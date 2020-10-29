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

#pragma once

class Window;
class Scene;
class Dialog;

#include <string>
#include <vector>
#include <memory>

class Gui {
public:
	// For tools that don't use the main Gui
	static void Init(const Window & window);
	static void NewFrame();
	static void DrawFrame();
	static void Shutdown();
public:
	Gui(std::shared_ptr<Window> window);
	~Gui();

	void setScene(std::shared_ptr<Scene> scene);

	// Call these resp. before and after loading the scene
	void beforeLoading();
	void afterLoading();

	void update();
	void render();

	// event callbacks
	virtual void onResize(int width, int height);
	virtual void onMouseButton(int button, int action, int mods);
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void onCursorPosition(double x, double y);

	float width() const { return m_windowWidth; }
	float height() const { return m_windowHeight; }

protected:
	std::weak_ptr<Window> getWindow() const { return m_window; }

private:
	void setupCallbacks();
	void setupDialogs();
	void updateImGui();

private:
	struct DialogGroup {
		std::string title;
		std::vector<std::shared_ptr<Dialog>> dialogs;
		bool enabled = false;
	};

private:
	template<typename DialogType, typename ControllerType>
	void addDialogGroup(std::string title, ControllerType controller)
	{
		DialogGroup group;
		group.title = title;
		auto dialog = std::make_shared<DialogType>();
		dialog->setController(controller);
		group.dialogs.push_back(dialog);
		m_dialogGroups.push_back(group);
	}

private:
	std::weak_ptr<Window> m_window;
	std::shared_ptr<Scene> m_scene;
	std::vector<DialogGroup> m_dialogGroups;

	float m_startTime;

	float m_windowWidth;
	float m_windowHeight;
	bool m_showPanel = true;
	float m_panelWidth = 300.f;

	bool m_imguiFocus;
	bool m_isMouseMoveStarted;
	int m_isControlPressed = 0;
};
