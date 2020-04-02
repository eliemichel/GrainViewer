
#pragma once

#include "Behavior/ImpostorSandRenderer.h"
#include "Dialog.h"
#include <memory>

class ImpostorSandRendererDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<ImpostorSandRenderer> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<ImpostorSandRenderer> m_cont;
};

registerDialogForBehavior(ImpostorSandRendererDialog, ImpostorSandRenderer)
