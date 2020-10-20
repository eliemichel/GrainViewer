#pragma once

#include "Behavior/MeshRenderer.h"
#include "Dialog.h"

#include <memory>

class MeshRendererDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<MeshRenderer> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<MeshRenderer> m_cont;
};

registerDialogForBehavior(MeshRendererDialog, MeshRenderer)
