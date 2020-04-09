#include "GlobalTimerDialog.h"
#include "utils/guiutils.h"
#include "utils/strutils.h"
#include <imgui.h>

void GlobalTimerDialog::draw()
{
	auto cont = m_cont.lock();
	if (!cont) return;
	if (ImGui::CollapsingHeader("Timers", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Reset All")) {
			cont->resetAllStats();
		}
		for (const auto& s : cont->stats()) {
			int n = s.second.sampleCount;
			double avg = s.second.cumulatedTime / static_cast<double>(n);
			ImGui::Text(MAKE_STR(s.first << ": " << avg << " (" << n << " samples)").c_str());
		}
	}
}
