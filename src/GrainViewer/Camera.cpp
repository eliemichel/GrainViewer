/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "Camera.h"
#include "Logger.h"
#include "Framebuffer.h"
#include "utils/behaviorutils.h"

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <algorithm>

Camera::Camera()
	: m_isMouseRotationStarted(false)
	, m_isMouseZoomStarted(false)
	, m_isMousePanningStarted(false)
	, m_isLastMouseUpToDate(false)
	, m_fov(45.0f)
	, m_orthographicScale(1.0f)
	, m_freezeResolution(false)
	, m_projectionType(PerspectiveProjection)
	, m_extraFramebuffers(static_cast<int>(ExtraFramebufferOption::_Count))
{
	initUbo();
	setResolution(800, 600);
}

Camera::~Camera() {
	glDeleteBuffers(1, &m_ubo);
}

void Camera::update(float time) {
	m_position = glm::vec3(3.f * cos(time), 3.f * sin(time), 0.f);
	m_uniforms.viewMatrix = glm::lookAt(
		m_position,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
}

void Camera::initUbo() {
	glCreateBuffers(1, &m_ubo);
	glNamedBufferStorage(m_ubo, static_cast<GLsizeiptr>(sizeof(CameraUbo)), NULL, GL_DYNAMIC_STORAGE_BIT);
}
void Camera::updateUbo() {
	
	m_uniforms.inverseViewMatrix = inverse(m_uniforms.viewMatrix);
	m_uniforms.resolution = resolution();
	glNamedBufferSubData(m_ubo, 0, static_cast<GLsizeiptr>(sizeof(CameraUbo)), &m_uniforms);
}

void Camera::setResolution(glm::vec2 resolution)
{
	if (!m_freezeResolution) {
		glm::vec4 rect = properties().viewRect;
		m_uniforms.resolution = resolution * glm::vec2(rect.z, rect.w);
	}

	updateProjectionMatrix();

	// Resize extra framebuffers
	size_t width = static_cast<size_t>(m_uniforms.resolution.x);
	size_t height = static_cast<size_t>(m_uniforms.resolution.y);
	for (auto& fbo : m_extraFramebuffers) {
		if (fbo) fbo->setResolution(width, height);
	}

	updateUbo();
}

void Camera::setFreezeResolution(bool freeze)
{
	m_freezeResolution = freeze;
	if (m_freezeResolution) {
		size_t width = static_cast<size_t>(m_uniforms.resolution.x);
		size_t height = static_cast<size_t>(m_uniforms.resolution.y);
		const std::vector<ColorLayerInfo> colorLayerInfos = { { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
		m_targetFramebuffer = std::make_shared<Framebuffer>(width, height, colorLayerInfos);
	} else {
		m_targetFramebuffer = nullptr;
	}
}

void Camera::setFov(float fov)
{
	m_fov = fov;
	updateProjectionMatrix();
}

float Camera::focalLength() const
{
	return 1.0f / (2.0f * glm::tan(glm::radians(fov()) / 2.0f));
}

void Camera::setNearDistance(float distance)
{
	m_uniforms.uNear = distance;
	updateProjectionMatrix();
}

void Camera::setFarDistance(float distance)
{
	m_uniforms.uFar = distance;
	updateProjectionMatrix();
}


void Camera::setOrthographicScale(float orthographicScale)
{
	m_orthographicScale = orthographicScale;
	updateProjectionMatrix();
}

void Camera::setProjectionType(ProjectionType projectionType)
{
	m_projectionType = projectionType;
	updateProjectionMatrix();
}

void Camera::updateMousePosition(float x, float y)
{
	if (m_isLastMouseUpToDate) {
		if (m_isMouseRotationStarted) {
			updateDeltaMouseRotation(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMouseZoomStarted) {
			updateDeltaMouseZoom(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMousePanningStarted) {
			updateDeltaMousePanning(m_lastMouseX, m_lastMouseY, x, y);
		}
	}
	m_lastMouseX = x;
	m_lastMouseY = y;
	m_isLastMouseUpToDate = true;
}

void Camera::updateProjectionMatrix()
{
	switch (m_projectionType) {
	case PerspectiveProjection:
		if (m_uniforms.resolution.x > 0.0f && m_uniforms.resolution.y > 0.0f) {
			m_uniforms.projectionMatrix = glm::perspectiveFov(glm::radians(m_fov), m_uniforms.resolution.x, m_uniforms.resolution.y, m_uniforms.uNear, m_uniforms.uFar);
			m_uniforms.uRight = m_uniforms.uNear / m_uniforms.projectionMatrix[0][0];
			m_uniforms.uLeft = -m_uniforms.uRight;
			m_uniforms.uTop = m_uniforms.uNear / m_uniforms.projectionMatrix[1][1];
			m_uniforms.uBottom = -m_uniforms.uTop;
		}
		break;
	case OrthographicProjection:
	{
		float ratio = m_uniforms.resolution.x > 0.0f ? m_uniforms.resolution.y / m_uniforms.resolution.x : 1.0f;
		m_uniforms.projectionMatrix = glm::ortho(-m_orthographicScale, m_orthographicScale, -m_orthographicScale * ratio, m_orthographicScale * ratio, m_uniforms.uNear, m_uniforms.uFar);
		m_uniforms.uRight = 1.0f / m_uniforms.projectionMatrix[0][0];
		m_uniforms.uLeft = -m_uniforms.uRight;
		m_uniforms.uTop = 1.0f / m_uniforms.projectionMatrix[1][1];
		m_uniforms.uBottom = -m_uniforms.uTop;
		break;
	}
	}
}

glm::vec3 Camera::projectSphere(glm::vec3 center, float radius) const
{
	// http://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm
	glm::vec3 o = (viewMatrix() * glm::vec4(center, 1.0f));

	glm::mat2 R = glm::mat2(o.x, o.y, -o.y, o.x);

	float r2 = radius * radius;
	float fl = focalLength();
	float ox2 = o.x * o.x;
	float oy2 = o.y * o.y;
	float oz2 = o.z * o.z;
	float fp = fl * fl * r2 * (ox2 + oy2 + oz2 - r2) / (oz2 - r2);
	float outerRadius = sqrt(abs(fp / (r2 - oz2)));

	glm::vec2 circleCenter = glm::vec2(o.x, o.y) * o.z * fl / (oz2 - r2);

	float pixelRadius = outerRadius * m_uniforms.resolution.y;
	glm::vec2 pixelCenter = circleCenter * m_uniforms.resolution.y * glm::vec2(-1.0f, 1.0f) + m_uniforms.resolution * 0.5f;

	return glm::vec3(pixelCenter, pixelRadius);
}

float Camera::linearDepth(float zbufferDepth) const
{
	return (2.0f * m_uniforms.uNear * m_uniforms.uFar) / (m_uniforms.uFar + m_uniforms.uNear - (zbufferDepth * 2.0f - 1.0f) * (m_uniforms.uFar - m_uniforms.uNear));
}

///////////////////////////////////////////////////////////////////////////////
// Serialization
///////////////////////////////////////////////////////////////////////////////

#include <regex>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
#define _USE_MATH_DEFINES
#include <math.h>

#include "ResourceManager.h"
#include "AnimationManager.h"
#include "utils/jsonutils.h"

void Camera::deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations)
{
	if (json.HasMember("resolution")) {
		auto& resolutionJson = json["resolution"];
		if (resolutionJson.IsString()) {
			std::string resolution = resolutionJson.GetString();
			if (resolution != "auto") {
				ERR_LOG << "Invalid resolution '" << resolution << "'. Resolution must either be an array of two int elements or the string 'auto'";
			}
			else {
				setFreezeResolution(false);
			}
		}
		else if (resolutionJson.IsArray()) {
			if (resolutionJson.Size() != 2 || !resolutionJson[0].IsInt() || !resolutionJson[1].IsInt()) {
				ERR_LOG << "Invalid resolution. Resolution must either be an array of two int elements or the string 'auto'";
			}
			else {
				int width = resolutionJson[0].GetInt();
				int height = resolutionJson[1].GetInt();
				LOG << "Freeze camera resolution to " << width << "x" << height;
				setResolution(width, height);
				setFreezeResolution(true);
			}
		}
		else {
			ERR_LOG << "Invalid resolution '" << resolutionJson.GetString() << "'. Resolution must either be an array of two int elements or the string 'auto'";
		}
	}

	if (json.HasMember("outputResolution")) {
		auto& resolutionJson = json["outputResolution"];
		if (resolutionJson.IsArray()) {
			if (resolutionJson.IsString()) {
				std::string resolution = resolutionJson.GetString();
				if (resolution != "auto") {
					ERR_LOG << "Invalid output resolution '" << resolution << "'. Output resolution must either be an array of two int elements or the string 'auto'";
				}
				else {
					outputSettings().autoOutputResolution = true;
				}
			}
			else if (resolutionJson.Size() != 2 || !resolutionJson[0].IsInt() || !resolutionJson[1].IsInt()) {
				ERR_LOG << "Invalid output resolution. Output resolution must either be an array of two int elements or the string 'auto'";
			}
			else {
				auto& s = outputSettings();
				s.width = resolutionJson[0].GetInt();
				s.height = resolutionJson[1].GetInt();
				s.autoOutputResolution = false;
			}
		}
		else {
			ERR_LOG << "Invalid output resolution '" << resolutionJson.GetString() << "'. Output resolution must either be an array of two int elements or the string 'auto'";
		}
	}

	if (json.HasMember("isRecordEnabled")) {
		if (json["isRecordEnabled"].IsBool()) {
			outputSettings().isRecordEnabled = json["isRecordEnabled"].GetBool();
		} else if (json["isRecordEnabled"].IsObject() && animations) {
			auto& anim = json["isRecordEnabled"];
			if (anim.HasMember("start") && anim.HasMember("end") && anim["start"].IsInt() && anim["end"].IsInt()) {
				int start = anim["start"].GetInt();
				int end = anim["end"].GetInt();
				animations->addAnimation([start, end, this](float time, int frame) {
					outputSettings().isRecordEnabled = frame >= start && frame <= end;
				});
			}
		}
	}

	jrOption(json, "saveOnDisc", outputSettings().saveOnDisc, outputSettings().saveOnDisc);

	std::string outputFrameBase;
	if (jrOption(json, "outputFrameBase", outputFrameBase)) {
		outputFrameBase = std::regex_replace(outputFrameBase, std::regex("\\$BASEFILE"), env.baseFile);
		outputFrameBase = ResourceManager::resolveResourcePath(outputFrameBase);
		outputSettings().outputFrameBase = outputFrameBase;
	}

	if (json.HasMember("orthographicScale") && json["orthographicScale"].IsFloat()) {
		float orthographicScale = json["orthographicScale"].GetFloat();
		setOrthographicScale(orthographicScale);
	}

	if (json.HasMember("projection") && json["projection"].IsString()) {
		std::string projection = json["projection"].GetString();
		if (projection == "orthographic") {
			setProjectionType(Camera::OrthographicProjection);
		}
		else if (projection == "perspective") {
			setProjectionType(Camera::PerspectiveProjection);
		}
		else {
			ERR_LOG << "Invalid projection type: '" << projection << "'";
		}
	}

	if (json.HasMember("position") || json.HasMember("rotationEuler")) {
		glm::vec3 pos = position();
		glm::vec3 euler = glm::vec3(0.0f, 0.0f, 0.0f);

		if (json.HasMember("position")) {
			auto& p = json["position"];
			bool valid = p.IsArray() && p.Size() == 3 && p[0].IsNumber() && p[1].IsNumber() && p[2].IsNumber();
			if (!valid) {
				ERR_LOG << "camera position must be an array of 3 numbers";
			}
			else {
				pos = glm::vec3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat());
			}
		}

		if (json.HasMember("rotationEuler")) {
			auto& p = json["rotationEuler"];
			bool valid = p.IsArray() && p.Size() == 3 && p[0].IsNumber() && p[1].IsNumber() && p[2].IsNumber();
			if (!valid) {
				ERR_LOG << "camera rotation_euler must be an array of 3 numbers";
			}
			else {
				euler = glm::vec3(p[0].GetFloat() * M_PI / 180.0f, p[1].GetFloat() * M_PI / 180.0f, p[2].GetFloat() * M_PI / 180.0f);
			}
		}

		glm::mat4 viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::translate(viewMatrix, pos);
		viewMatrix = viewMatrix * glm::eulerAngleXYZ(euler.x, euler.y, euler.z);
		setViewMatrix(viewMatrix);
	}

	if (json.HasMember("viewMatrix")) {
		auto& mat = json["viewMatrix"];
		if (mat.IsArray()) {
			bool valid = true;
			for (int i = 0; i < 16 && valid; ++i) valid = valid && mat[i].IsNumber();
			if (!valid) {
				ERR_LOG << "viewMatrix must be either a 16-float array or an object";
			} else {
				float data[16];
				for (int i = 0; i < 16; ++i) data[i] = mat[i].GetFloat();
				glm::mat4 viewMatrix = glm::make_mat4(data);
				setViewMatrix(viewMatrix);
			}
		}
		else if (mat.IsObject()) {
			bool valid = mat.HasMember("buffer") && mat.HasMember("startFrame") && mat["buffer"].IsString() && mat["startFrame"].IsNumber();
			if (!valid) {
				ERR_LOG << "viewMatrix object must provide 'buffer' (string) and 'startFrame' (number) fields";
			}
			else if (animations) {
				std::string path = ResourceManager::resolveResourcePath(mat["buffer"].GetString());
				LOG << "Loading camera movement from " << path << "...";

				std::ifstream file(path, std::ios::binary | std::ios::ate);
				if (!file.is_open()) {
					ERR_LOG << "Could not open viewMatrix buffer file: " << path;
					return;
				}
				std::streamsize size = file.tellg() / sizeof(float);
				if (size % 16 != 0) {
					ERR_LOG << "viewMatrix buffer size must be a multiple of 16 (in file " << path << ")";
					return;
				}
				file.seekg(0, std::ios::beg);
				std::shared_ptr<float[]> buffer(new float[size]);
				if (!file.read(reinterpret_cast<char*>(buffer.get()), size * sizeof(float))) {
					ERR_LOG << "Could not read viewMatrix buffer from file: " << path;
					return;
				}

				int startFrame = mat["startFrame"].GetInt();
				int endFrame = startFrame + static_cast<int>(size) / 16 - 1;
				animations->addAnimation([startFrame, endFrame, buffer, this](float time, int frame) {
					if (frame >= startFrame && frame < endFrame) {
						glm::mat4 viewMatrix = glm::make_mat4(buffer.get() + 16 * (frame - startFrame));
						setViewMatrix(viewMatrix);
						updateUbo();
					}
				});
			}
		}
	}

	float fov;
	if (jrOption(json, "fov", fov)) setFov(fov);

	float dist;
	if (jrOption(json, "near", dist)) setNearDistance(dist);
	if (jrOption(json, "far", dist)) setFarDistance(dist);

	autoDeserialize(json, m_properties);
}

std::ostream& Camera::serialize(std::ostream& out)
{
	out << "Camera {}";
	return out;
}

///////////////////////////////////////////////////////////////////////////////
// Framebuffer pool
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Framebuffer> Camera::getExtraFramebuffer(ExtraFramebufferOption option) const
{
	// TODO: add mutex
	auto& fbo = m_extraFramebuffers[static_cast<int>(option)];
	// Lazy construction
	if (!fbo) {
		int width = static_cast<int>(m_uniforms.resolution.x);
		int height = static_cast<int>(m_uniforms.resolution.y);
		std::vector<ColorLayerInfo> colorLayerInfos;
		using Opt = ExtraFramebufferOption;
		switch (option)
		{
		case Opt::Rgba32fDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{ { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
			break;
		case Opt::TwoRgba32fDepth:
		case Opt::LinearGBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT1 }
			};
			break;
		case Opt::LeanLinearGBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT1 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT2 }
			};
			break;
		case Opt::Depth:
			colorLayerInfos = std::vector<ColorLayerInfo>{};
			break;
		case Opt::GBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32UI,  GL_COLOR_ATTACHMENT1 },
				{ GL_RGBA32UI,  GL_COLOR_ATTACHMENT2 }
			};
			break;
		}
		fbo = std::make_shared<Framebuffer>(width, height, colorLayerInfos);
	}
	return fbo;
}

void Camera::releaseExtraFramebuffer(std::shared_ptr<Framebuffer>) const
{
	// TODO: release mutexes
}
