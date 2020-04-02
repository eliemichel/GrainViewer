
#pragma once

#include "Behavior/SandBehavior.h"
#include "Dialog.h"
#include <memory>

class SandBehaviorDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<SandBehavior> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<SandBehavior> m_cont;
};

registerDialogForBehavior(SandBehaviorDialog, SandBehavior)
