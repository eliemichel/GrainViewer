#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/SandRenderer.h"

class SandRendererDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<SandRenderer> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<SandRenderer> m_cont;
};

registerDialogForBehavior(SandRendererDialog, SandRenderer)
