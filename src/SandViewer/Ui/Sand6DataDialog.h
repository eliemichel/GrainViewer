#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/Sand6Data.h"

class Sand6DataDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<Sand6Data> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<Sand6Data> m_cont;
};

registerDialogForBehavior(Sand6DataDialog, Sand6Data)
