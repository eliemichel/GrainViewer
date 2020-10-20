#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/FarSandRenderer.h"

class FarSandRendererDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<FarSandRenderer> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<FarSandRenderer> m_cont;
};

registerDialogForBehavior(FarSandRendererDialog, FarSandRenderer)
