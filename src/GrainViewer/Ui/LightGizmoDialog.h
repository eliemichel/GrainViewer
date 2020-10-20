#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/LightGizmo.h"

class LightGizmoDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<LightGizmo> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<LightGizmo> m_cont;
};

registerDialogForBehavior(LightGizmoDialog, LightGizmo)
