// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <iostream>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Logger.h"
#include "TestGui.h"
#include "Window.h"
#include "Widgets.h"

void TestGui::setupCallbacks()
{
	if (auto window = m_window.lock()) {
		glfwSetWindowUserPointer(window->glfw(), static_cast<void*>(this));
		glfwSetKeyCallback(window->glfw(), [](GLFWwindow* window, int key, int scancode, int action, int mode) {
			static_cast<TestGui*>(glfwGetWindowUserPointer(window))->onKey(key, scancode, action, mode);
		});
		glfwSetCursorPosCallback(window->glfw(), [](GLFWwindow* window, double x, double y) {
			static_cast<TestGui*>(glfwGetWindowUserPointer(window))->onCursorPosition(x, y);
		});
		glfwSetMouseButtonCallback(window->glfw(), [](GLFWwindow* window, int button, int action, int mods) {
			static_cast<TestGui*>(glfwGetWindowUserPointer(window))->onMouseButton(button, action, mods);
		});
		glfwSetWindowSizeCallback(window->glfw(), [](GLFWwindow* window, int width, int height) {
			static_cast<TestGui*>(glfwGetWindowUserPointer(window))->onResize(width, height);
		});
	}
}

TestGui::TestGui(std::shared_ptr<Window> window)
	: m_window(window)
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

TestGui::~TestGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void TestGui::updateProgress(float progress)
{
	m_progress = progress;
	render();
	if (auto window = m_window.lock()) {
		window->swapBuffers();
	}
}

void TestGui::addMessage(const std::string & msg)
{
	LOG << msg;
	m_messages.push_back(msg);
}

void TestGui::render() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::Begin("Progress", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);

	ImGui::Spacing();
	ImGui::Text("Test progress:");
	const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
	const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);
	ProgressBar("Progress", m_progress, ImVec2(m_windowWidth - 7, 6), bg, col);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	ImGui::BeginChild("Messages", ImVec2(0, 0), false, ImGuiWindowFlags_NoTitleBar);
	for (const auto & msg : m_messages) {
		ImGui::Text(msg.c_str());
	}
	if (ImGui::GetScrollY() == ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHere();
	}
	ImGui::End();

	ImGui::End();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void TestGui::onResize(int width, int height) {
	m_windowWidth = static_cast<float>(width);
	m_windowHeight = static_cast<float>(height);

	if (auto window = m_window.lock()) {
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window->glfw(), &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);
	}
}

void TestGui::onMouseButton(int button, int action, int mods) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (action == GLFW_PRESS) {
		}
		break;
	}
}

void TestGui::onCursorPosition(double x, double y) {
}

void TestGui::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			if (auto window = m_window.lock()) {
				glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
			}
			break;
		}
	}
}

