
#pragma once

#include "Behavior/QuadMeshData.h"
#include "Dialog.h"
#include <memory>

class QuadMeshDataDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<QuadMeshData> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<QuadMeshData> m_cont;
};

registerDialogForBehavior(QuadMeshDataDialog, QuadMeshData)
