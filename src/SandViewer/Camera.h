// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <OpenGL>

#include "EnvironmentVariables.h"

#include <refl.hpp>
#include <rapidjson/document.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>
#include <vector>

class Framebuffer;
class AnimationManager;

class Camera {
public:
	enum ProjectionType {
		PerspectiveProjection,
		OrthographicProjection,
	};
	struct OutputSettings {
		bool autoOutputResolution = true;
		int width;
		int height;
		std::string outputFrameBase;
		bool isRecordEnabled = false;
		bool saveOnDisc = true; // if false, get pixel data from gpu but don't actually write them to disc. They can be used to measure stats
	};
	enum class ExtraFramebufferOption {
		Rgba32fDepth = 0,
		TwoRgba32fDepth = 1,
		Depth = 2,
		GBufferDepth = 3, // attachements to hold a g-buffer (see gbuffer.inc.glsl plus a depth buffer
		LinearGBufferDepth = 4, // same with linear g-buffer
		LeanLinearGBufferDepth = 5, // same with linear g-buffer + (pseudo) lean maps
		_Count,
	};

public:
	Camera();
	~Camera();
	virtual void update(float time);

	void initUbo();
	void updateUbo();
	GLuint ubo() const { return m_ubo; }

	inline glm::vec3 position() const { return m_position; }

	inline glm::mat4 viewMatrix() const { return m_uniforms.viewMatrix; }
	inline void setViewMatrix(glm::mat4 matrix) { m_uniforms.viewMatrix = matrix; }  // use with caution

	inline glm::mat4 projectionMatrix() const { return m_uniforms.projectionMatrix; }
	inline void setProjectionMatrix(glm::mat4 matrix) { m_uniforms.projectionMatrix = matrix; }  // use with caution
	void setProjectionType(ProjectionType projectionType);

	inline glm::vec2 resolution() const { return m_uniforms.resolution; }
	void setResolution(glm::vec2 resolution);
	inline void setResolution(int w, int h) { setResolution(glm::vec2(static_cast<float>(w), static_cast<float>(h))); }
	void setFreezeResolution(bool freeze);

	float fov() const { return m_fov; }
	float focalLength() const;
	void setFov(float fov);

	float nearDistance() const { return m_uniforms.uNear; }
	void setNearDistance(float distance);

	float farDistance() const { return m_uniforms.uFar; }
	void setFarDistance(float distance);

	void setOrthographicScale(float orthographicScale);
	float orthographicScale() const { return m_orthographicScale; }

	OutputSettings & outputSettings() { return m_outputSettings; }
	const OutputSettings & outputSettings() const { return m_outputSettings; }

	std::shared_ptr<Framebuffer> targetFramebuffer() const { return m_targetFramebuffer; }

	inline void startMouseRotation() { m_isMouseRotationStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMouseRotation() { m_isMouseRotationStarted = false; }

	inline void startMouseZoom() { m_isMouseZoomStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMouseZoom() { m_isMouseZoomStarted = false; }
	
	inline void startMousePanning() { m_isMousePanningStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMousePanning() { m_isMousePanningStarted = false; }

	inline void tiltLeft() { tilt(-0.1f); }
	inline void tiltRight() { tilt(0.1f); }

	void updateMousePosition(float x, float y);

	virtual void deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations);
	virtual std::ostream & serialize(std::ostream & out);

	/**
	 * Get a framebuffer that has the same resolution as the camera, to be used
	 * as intermediate step in render. Once you're done with it, release the
	 * framebuffer with releaseExtraFramebuffer(). It is safe to call this
	 * every frame.
	 * TODO: implement a proper dynamic framebuffer pool mechanism
	 */
	std::shared_ptr<Framebuffer> getExtraFramebuffer(ExtraFramebufferOption option = ExtraFramebufferOption::Rgba32fDepth) const;
	void releaseExtraFramebuffer(std::shared_ptr<Framebuffer>) const;

	/**
	 * Bounding circle of the projected sphere (which is an ellipsis).
	 * xy is the center, z is the radius, all in pixels
	 */
	glm::vec3 projectSphere(glm::vec3 center, float radius) const;

	/**
	 * Convert depth value from Z-buffer to a linear depth in camera space units
	 */
	float linearDepth(float zbufferDepth) const;

	public:
		struct Properties {
			// xy: offset, relative to screen size
			// zw: size, relative to screen size
			// e.g. for fullscreen: (0, 0, 1, 1)
			glm::vec4 viewRect = glm::vec4(0, 0, 1, 1);

			bool displayInViewport = true;
			bool controlInViewport = true; // receive mouse input
		};
		const Properties& properties() const { return m_properties; }
		Properties & properties(){ return m_properties; }

protected:
	/**
	* Called when mouse moves and rotation has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {}

	/**
	* Called when mouse moves and zoom has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {}

	/**
	* Called when mouse moves and panning has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMousePanning(float x1, float y1, float x2, float y2) {}

	virtual void tilt(float theta) {}

private:
	// Memory layout on GPU, matches shaders/include/uniform/camera.inc.glsl
	struct CameraUbo {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
		glm::mat4 inverseViewMatrix;
		glm::vec2 resolution;
		float uNear = 0.001f;
		float uFar = 1000.f;
		float uLeft;
		float uRight;
		float uTop;
		float uBottom;
	};

private:
	void updateProjectionMatrix();

protected:
	// Core data
	Properties m_properties;
	CameraUbo m_uniforms;
	GLuint m_ubo;

	// Other data, used to build matrices but not shared with gpu
	glm::vec3 m_position;
	float m_fov;
	float m_orthographicScale;

	// Move related attributes
	bool m_isMouseRotationStarted, m_isMouseZoomStarted, m_isMousePanningStarted;
	bool m_isLastMouseUpToDate;
	float m_lastMouseX, m_lastMouseY;

	bool m_freezeResolution;

	// When resolution is freezed, this target framebuffer is allocated at the
	// fixed resolution and can be bound by the render pipeline
	std::shared_ptr<Framebuffer> m_targetFramebuffer;

	mutable std::vector<std::shared_ptr<Framebuffer>> m_extraFramebuffers; // lazy initialized

	OutputSettings m_outputSettings;
	ProjectionType m_projectionType;
};

REFL_TYPE(Camera::Properties)
REFL_FIELD(viewRect)
REFL_FIELD(displayInViewport)
REFL_FIELD(controlInViewport)
REFL_END
