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
#include "TestGui.h"
#include "Window.h"
#include "Widgets.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>

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

