// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

class Window;

#include <string>
#include <vector>

/**
 * Gui for tests, displaying a progress bar and some debug info
 */
class TestGui {
public:
	TestGui(std::shared_ptr<Window> window);
	~TestGui();

	void updateProgress(float progress);
	void addMessage(const std::string & msg);

	void render();

	// event callbacks
	void onResize(int width, int height);
	void onMouseButton(int button, int action, int mods);
	void onKey(int key, int scancode, int action, int mods);
	void onCursorPosition(double x, double y);

private:
	void setupCallbacks();

private:
	std::weak_ptr<Window> m_window;

	float m_windowWidth;
	float m_windowHeight;

	float m_progress = 0;
	std::vector<std::string> m_messages;

	bool m_isMouseMoveStarted;
};
