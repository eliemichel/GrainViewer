#pragma once

#include <memory>
#include "Dialog.h"
#include "Behavior/PointCloudSplitter.h"

class PointCloudSplitterDialog : public Dialog {
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<PointCloudSplitter> behavior) { m_cont = behavior; }

private:
	std::weak_ptr<PointCloudSplitter> m_cont;
};

registerDialogForBehavior(PointCloudSplitterDialog, PointCloudSplitter)
