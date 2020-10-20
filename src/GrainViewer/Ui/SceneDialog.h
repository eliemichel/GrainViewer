#pragma once

#include <memory>
#include "Dialog.h"
#include "Scene.h"

class SceneDialog : public Dialog {
public:
	void draw() override;
	void setController(std::weak_ptr<Scene> scene) { m_cont = scene; }

private:
	std::weak_ptr<Scene> m_cont;
};
