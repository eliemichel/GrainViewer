// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <glad/glad.h>

#include <iostream>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Logger.h"
#include "Gui.h"
#include "Window.h"
#include "Scene.h"
#include "Dialog.h"
#include "RuntimeObject.h"

using namespace std;

// TODO: Find a way to avoid this function
#include "SandRendererDialog.h"
static std::shared_ptr<Dialog> makeComponentDialog(std::string type, std::shared_ptr<Behavior> component) {
#define handleType(T) \
	if (type == TypeName<T>().Get()) { \
		auto dialog = DialogFactory<T>().MakeShared(); \
		dialog->setControlledBehavior(std::dynamic_pointer_cast<T>(component)); \
		return std::dynamic_pointer_cast<Dialog>(dialog); \
	}
	handleType(SandRenderer);
	return nullptr;
#undef handleType
}

static void printUsage() {
	cerr
		<< "Augen -- real time viewer" << endl
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

Gui::Gui(std::shared_ptr<Window> window)
	: m_window(window)
	, m_scene(nullptr)
{
	setupCallbacks();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForOpenGL(window->glfw(), true);
	ImGui_ImplOpenGL3_Init("#version 150");
	ImGui::GetStyle().WindowRounding = 0.0f;

	int width, height;
	glfwGetFramebufferSize(window->glfw(), &width, &height);
	onResize(width, height);
}

Gui::~Gui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Gui::setScene(std::shared_ptr<Scene> scene)
{
	m_scene = scene;
	setupDialogs();
}

void Gui::beforeLoading()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::Begin("Reload", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);
	ImGui::Text("");
	ImGui::Text("");
	ImGui::Text("Loading scene...");
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
	m_dialogs.clear();
	if (m_scene) {
		for (const auto& obj : m_scene->objects()) {
			IBehaviorHolder::ConstBehaviorIterator it, end;
			for (it = obj->cbeginBehaviors(), end = obj->cendBehaviors(); it != end;  ++it) {
				auto dialog = makeComponentDialog(it->first, it->second);
				if (dialog) {
					m_dialogs.push_back(dialog);
				}
			}
		}
	}
}

void Gui::update() {
	updateImGui();
	if (m_scene) {
		m_scene->update(static_cast<float>(glfwGetTime()) - m_startTime);
	}
}

void Gui::updateImGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImVec4 clear_color = ImColor(114, 144, 154);
	static float f = 0.0f;
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::Begin("Framerate", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	if (m_showPanel) {
		ImGui::SetNextWindowSize(ImVec2(m_panelWidth, m_windowWidth));
		ImGui::SetNextWindowPos(ImVec2(m_windowWidth - m_panelWidth, 0.f));
		ImGui::Begin("Dialogs", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoTitleBar);
		for (auto d : m_dialogs) {
			d->draw();
		}
		ImGui::End();
	}
}

void Gui::render() {
	if (m_scene) {
		m_scene->render();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::onResize(int width, int height) {
	m_windowWidth = static_cast<float>(width);
	m_windowHeight = static_cast<float>(height);

	if (auto window = m_window.lock()) {
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window->glfw(), &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);
		if (m_scene) {
			m_scene->viewportCamera().setResolution(glm::vec2(fbWidth, fbHeight));
		}
	}
}

void Gui::onMouseButton(int button, int action, int mods) {
	if (m_imguiFocus || !m_scene) {
		return;
	}

	m_isMouseMoveStarted = action == GLFW_PRESS;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (action == GLFW_PRESS) {
			m_scene->viewportCamera().startMouseRotation();
		}
		else {
			m_scene->viewportCamera().stopMouseRotation();
		}
		break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		if (action == GLFW_PRESS) {
			m_scene->viewportCamera().startMouseZoom();
		}
		else {
			m_scene->viewportCamera().stopMouseZoom();
		}
		break;

	case GLFW_MOUSE_BUTTON_RIGHT:
		if (action == GLFW_PRESS) {
			m_scene->viewportCamera().startMousePanning();
		}
		else {
			m_scene->viewportCamera().stopMousePanning();
		}
		break;
	}
}

void Gui::onCursorPosition(double x, double y) {
	m_imguiFocus = x >= m_windowWidth - m_panelWidth && m_showPanel && !m_isMouseMoveStarted;

	if (m_imguiFocus) {
		return;
	}

	if (m_scene) {
		m_scene->viewportCamera().updateMousePosition(static_cast<float>(x), static_cast<float>(y));
	}
}

void Gui::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		if (auto window = m_window.lock()) {
			glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
		}
	}

	if (m_imguiFocus) {
		return;
	}

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			if (auto window = m_window.lock()) {
				glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
			}
			break;

		case GLFW_KEY_P:
			m_showPanel = !m_showPanel;
			break;
		}
	}

	if (action == GLFW_PRESS && m_scene) {
		switch (key) {
		case GLFW_KEY_R:
			if (mods & GLFW_MOD_CONTROL) {
				beforeLoading();
				m_scene->load(m_scene->filename());
				afterLoading();
			}
			else {
				m_scene->reloadShaders();
			}
			break;

		case GLFW_KEY_A:
			m_scene->viewportCamera().tiltLeft();
			break;

		case GLFW_KEY_E:
			m_scene->viewportCamera().tiltRight();
			break;
		}
	}
}

