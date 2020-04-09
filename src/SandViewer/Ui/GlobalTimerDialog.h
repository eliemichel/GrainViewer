#pragma once

#include <memory>
#include "Dialog.h"
#include "GlobalTimer.h"

class GlobalTimerDialog : public Dialog {
public:
	void draw() override;
	void setController(std::weak_ptr<GlobalTimer> timer) { m_cont = timer; }

private:
	std::weak_ptr<GlobalTimer> m_cont;
};
