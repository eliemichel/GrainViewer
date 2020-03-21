#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/TransformBehavior.h"

class TransformDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<TransformBehavior> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<TransformBehavior> m_cont;
};

registerDialogForBehavior(TransformDialog, TransformBehavior)
