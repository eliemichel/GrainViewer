#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/UberSandRenderer.h"

class UberSandRendererDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<UberSandRenderer> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<UberSandRenderer> m_cont;
};

registerDialogForBehavior(UberSandRendererDialog, UberSandRenderer)
