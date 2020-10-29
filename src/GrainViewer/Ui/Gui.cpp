/**
 * This file is part of GrainViewer
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

#include <OpenGL>

#include "Logger.h"
#include "Gui.h"
#include "Window.h"
#include "Scene.h"
#include "Dialog.h"
#include "RuntimeObject.h"
#include "ShaderPool.h"
#include "GlobalTimer.h"
#include "Ui/SceneDialog.h"
#include "Ui/DeferredShadingDialog.h"
#include "Ui/WorldDialog.h"
#include "Ui/GlobalTimerDialog.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>

using namespace std;

// TODO: Find a way to avoid this function
#include "LightGizmoDialog.h"
#include "FarGrainRendererDialog.h"
#include "TransformDialog.h"
#include "PointCloudSplitterDialog.h"
#include "InstanceGrainRendererDialog.h"
#include "ImpostorGrainRendererDialog.h"
#include "GrainBehaviorDialog.h"
#include "MeshRendererDialog.h"
#include "QuadMeshDataDialog.h"
static std::shared_ptr<Dialog> makeComponentDialog(std::string type, std::shared_ptr<Behavior> component) {
#define handleBehavior(T) \
	if (type == BehaviorRegistryEntry<T>::Name()) { \
		auto dialog = DialogFactory<T>().MakeShared(); \
		dialog->setControlledBehavior(std::dynamic_pointer_cast<T>(component)); \
		return std::dynamic_pointer_cast<Dialog>(dialog); \
	}
	handleBehavior(LightGizmo);
	handleBehavior(FarGrainRenderer);
	handleBehavior(TransformBehavior);
	handleBehavior(PointCloudSplitter);
	handleBehavior(InstanceGrainRenderer);
	handleBehavior(ImpostorGrainRenderer);
	handleBehavior(GrainBehavior);
	handleBehavior(MeshRenderer);
	handleBehavior(QuadMeshData);
	return nullptr;
#undef handleType
}

//-----------------------------------------------------------------------------

void Gui::Init(const Window & window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForOpenGL(window.glfw(), true);
	ImGui_ImplOpenGL3_Init("#version 150");
	ImGui::GetStyle().WindowRounding = 0.0f;
}

void Gui::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Gui::DrawFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::Shutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

//-----------------------------------------------------------------------------

static void printUsage() {
	cerr
		<< "GrainViewer -- real time grain viewer" << endl
		<< "Author : Élie Michel (http://perso.telecom-paristech.fr/~emichel)" << endl
		<< endl
		<< "Keyboard commands" << endl
		<< "------------------" << endl
		<< " r: Reload shaders" << endl
		<< " ctrl+r: Reload scene" << endl
		<< " p: Toggle panel" << endl
		<< " a: Tilt camera left" << endl
		<< " e: Tilt camera right" << endl
		<< " d: Switch deferred shading on/off" << endl
		<< " c: Print camera matrix (row major)" << endl
		<< " <space>: Pause physics" << endl
		<< " ?: Print help" << endl
		<< " q, <esc>: Quit" << endl
		<< endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	Gui* gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
	gui->onKey(key, scancode, action, mode);
}

static void cursor_pos_callback(GLFWwindow* window, double x, double y) {
	Gui* gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
	gui->onCursorPosition(x, y);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	Gui* gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
	gui->onMouseButton(button, action, mods);
}

static void window_size_callback(GLFWwindow* window, int width, int height) {
	Gui* gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
	gui->onResize(width, height);
}

void Gui::setupCallbacks()
{
	if (auto window = m_window.lock()) {
		glfwSetWindowUserPointer(window->glfw(), static_cast<void*>(this));
		glfwSetKeyCallback(window->glfw(), key_callback);
		glfwSetCursorPosCallback(window->glfw(), cursor_pos_callback);
		glfwSetMouseButtonCallback(window->glfw(), mouse_button_callback);
		glfwSetWindowSizeCallback(window->glfw(), window_size_callback);
	}
}

//-----------------------------------------------------------------------------

Gui::Gui(std::shared_ptr<Window> window)
	: m_window(window)
	, m_scene(nullptr)
{
	setupCallbacks();

	Init(*window);

	int width, height;
	glfwGetFramebufferSize(window->glfw(), &width, &height);
	onResize(width, height);
}

Gui::~Gui() {
	Shutdown();
}

void Gui::setScene(std::shared_ptr<Scene> scene)
{
	m_scene = scene;
	setupDialogs();
}

void Gui::beforeLoading()
{
	NewFrame();
	ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::Begin("Reload", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);
	ImGui::Text(" ");
	ImGui::Text(" ");
	ImGui::Text("Loading scene...");
	ImGui::End();
	DrawFrame();
	if (auto window = m_window.lock()) {
		glfwSwapBuffers(window->glfw());
	}
}

void Gui::afterLoading()
{
	if (auto window = m_window.lock()) {
		int width, height;
		glfwGetFramebufferSize(window->glfw(), &width, &height);
		onResize(width, height);
	}

	m_startTime = static_cast<float>(glfwGetTime());
	setupDialogs();
}

void Gui::setupDialogs()
{
	m_dialogGroups.clear();
	if (m_scene) {
		addDialogGroup<DeferredShadingDialog>("Deferred Shading", m_scene->deferredShader());
		addDialogGroup<WorldDialog>("World", m_scene->world());
		addDialogGroup<GlobalTimerDialog>("Timers", GlobalTimer::GetInstance());
		addDialogGroup<SceneDialog>("Scene:", m_scene);

		for (const auto& obj : m_scene->objects()) {
			DialogGroup group;
			group.title = " - " + obj->name;
			IBehaviorHolder::ConstBehaviorIterator it, end;
			for (it = obj->cbeginBehaviors(), end = obj->cendBehaviors(); it != end;  ++it) {
				auto dialog = makeComponentDialog(it->first, it->second);
				if (dialog) {
					group.dialogs.push_back(dialog);
				}
			}
			m_dialogGroups.push_back(group);
		}
	}
}

void Gui::update() {
	updateImGui();
	if (m_scene) {
		m_scene->update(static_cast<float>(glfwGetTime()) - m_startTime);
		if (auto window = m_window.lock()) {
			if (m_scene->mustQuit()) {
				glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
			}
		}
	}
}

void Gui::updateImGui() {
	NewFrame();
	ImVec4 clear_color = ImColor(114, 144, 154);
	static float f = 0.0f;
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::SetNextWindowSize(ImVec2(420.f, 0.f));
	ImGui::Begin("Framerate", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);
	int frame = m_scene ? m_scene->frame() : 0;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS) | #%d", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, frame);
	ImGui::End();

	// Handles
	int dialogId = 0;
	for (auto & dg : m_dialogGroups) {
		ImGui::PushID(dialogId++);
		for (auto d : dg.dialogs) {
			d->drawHandles(0, 0, m_windowWidth, m_windowHeight);
		}
		ImGui::PopID();
	}

	if (m_showPanel) {
		ImGui::SetNextWindowSize(ImVec2(m_panelWidth, m_windowHeight));
		ImGui::SetNextWindowPos(ImVec2(m_windowWidth - m_panelWidth, 0.f));
		ImGui::Begin("Dialogs", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoTitleBar);

		// Outliner
		ImGui::PushID(0);

		int dialogId = 0;
		bool pressed;
		for (auto & dg : m_dialogGroups) {
			ImGui::PushID(dialogId);
			dialogId += static_cast<int>(dg.dialogs.size());
			pressed = ImGui::Selectable(dg.title.c_str(), &dg.enabled);
			if (pressed && m_isControlPressed == 0) {
				// Select exclusive
				for (auto & other : m_dialogGroups) {
					other.enabled = &other == &dg;
				}
			}
			ImGui::PopID();
		}

		ImGui::PopID();
		
		// Visible dialogs
		ImGui::PushID(1);

		dialogId = 0;
		for (auto & dg : m_dialogGroups) {
			if (dg.enabled) {
				for (auto d : dg.dialogs) {
					ImGui::PushID(dialogId++);
					d->draw();
					ImGui::PopID();
				}
			}
			else {
				dialogId += static_cast<int>(dg.dialogs.size());
			}
		}

		ImGui::PopID();

		ImGui::End();
	}
}

void Gui::render() {
	if (m_scene) {
		m_scene->render();
	}

	if (!m_scene || m_scene->properties().ui) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		DrawFrame();
	}

	if (m_scene) {
		m_scene->onPostRender(static_cast<float>(glfwGetTime()) - m_startTime);
	}
}

void Gui::onResize(int width, int height) {
	m_windowWidth = static_cast<float>(width);
	m_windowHeight = static_cast<float>(height);

	if (auto window = m_window.lock()) {
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window->glfw(), &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);
		if (m_scene) {
			m_scene->setResolution(fbWidth, fbHeight);
		}
	}
}

#define forAllCameras(method) \
for (auto& camera : m_scene->cameras()) { \
	if (camera->properties().controlInViewport) { \
		camera->method(); \
	} \
}

void Gui::onMouseButton(int button, int action, int mods) {
	if (m_imguiFocus || !m_scene) {
		return;
	}

	m_isMouseMoveStarted = action == GLFW_PRESS;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (action == GLFW_PRESS) {
			forAllCameras(startMouseRotation);
		}
		else {
			forAllCameras(stopMouseRotation);
		}
		break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		if (action == GLFW_PRESS) {
			forAllCameras(startMouseZoom);
		}
		else {
			forAllCameras(stopMouseZoom);
		}
		break;

	case GLFW_MOUSE_BUTTON_RIGHT:
		if (action == GLFW_PRESS) {
			forAllCameras(startMousePanning);
		}
		else {
			forAllCameras(stopMousePanning);
		}
		break;
	}
}

void Gui::onCursorPosition(double x, double y) {
	double limit = static_cast<double>(m_windowWidth) - static_cast<double>(m_panelWidth);
	m_imguiFocus = x >= limit && m_showPanel && !m_isMouseMoveStarted;

	if (m_imguiFocus) {
		return;
	}

	if (m_scene) {
		for (auto& camera : m_scene->cameras()) {
			if (camera->properties().controlInViewport) {
				camera->updateMousePosition(static_cast<float>(x), static_cast<float>(y));
			}
		}
	}
}

void Gui::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			if (auto window = getWindow().lock()) {
				glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
			}
			break;

		case GLFW_KEY_LEFT_CONTROL:
		case GLFW_KEY_RIGHT_CONTROL:
			++m_isControlPressed;
			break;
		}
	}

	if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_LEFT_CONTROL:
		case GLFW_KEY_RIGHT_CONTROL:
			//--m_isControlPressed;
			m_isControlPressed = 0;
			break;
		}
	}

	if (action == GLFW_PRESS && !m_imguiFocus) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			if (auto window = m_window.lock()) {
				glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
			}
			break;

		case GLFW_KEY_P:
			m_showPanel = !m_showPanel;
			break;

		case GLFW_KEY_LEFT_CONTROL:
		case GLFW_KEY_RIGHT_CONTROL:
			++m_isControlPressed;
			break;
		}
	}

	if (action == GLFW_PRESS && !m_imguiFocus && m_scene) {
		switch (key) {
		case GLFW_KEY_R:
			if (mods & GLFW_MOD_CONTROL) {
				beforeLoading();
				ShaderPool::Clear();
				m_scene->load(m_scene->filename());
				afterLoading();
			}
			else {
				m_scene->reloadShaders();
			}
			break;

		case GLFW_KEY_U:
			m_scene->properties().ui = !m_scene->properties().ui;
			break;

		case GLFW_KEY_A:
			forAllCameras(tiltLeft);
			break;

		case GLFW_KEY_E:
			forAllCameras(tiltRight);
			break;

		case GLFW_KEY_C:
			if (mods & GLFW_MOD_CONTROL) {
				ShaderPool::Clear();
				m_scene->clear();
			} else {
				if (m_scene->viewportCamera()) {
					glm::mat4 m = m_scene->viewportCamera()->viewMatrix();
					DEBUG_LOG << "View Matrix: \n"
						<< "[" << m[0][0] << "," << m[0][1] << "," << m[0][2] << "," << m[0][3]
						<< "," << m[1][0] << "," << m[1][1] << "," << m[1][2] << "," << m[1][3]
						<< "," << m[2][0] << "," << m[2][1] << "," << m[2][2] << "," << m[2][3]
						<< "," << m[3][0] << "," << m[3][1] << "," << m[3][2] << "," << m[3][3] << "]";

					std::ostringstream ss;
					m_scene->viewportCamera()->serialize(ss);
					DEBUG_LOG << ss.str();
				}
			}
			break;

		case GLFW_KEY_SPACE:
			m_scene->togglePause();
			break;
		}
	}
}

#undef forAllCameras
