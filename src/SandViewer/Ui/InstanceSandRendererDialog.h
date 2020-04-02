
#pragma once

#include "Behavior/InstanceSandRenderer.h"
#include "Dialog.h"
#include <memory>

class InstanceSandRendererDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<InstanceSandRenderer> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<InstanceSandRenderer> m_cont;
};

registerDialogForBehavior(InstanceSandRendererDialog, InstanceSandRenderer)
