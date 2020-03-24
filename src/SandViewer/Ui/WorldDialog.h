#pragma once

#include <memory>
#include "Dialog.h"
#include "World.h"

class WorldDialog : public Dialog {
public:
	void draw() override;
	void setController(std::weak_ptr<World> world) { m_cont = world; }

private:
	std::weak_ptr<World> m_cont;
};
