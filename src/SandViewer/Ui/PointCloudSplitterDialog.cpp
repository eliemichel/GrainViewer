#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "utils/guiutils.h"
#include "PointCloudSplitterDialog.h"
#include "utils/behaviorutils.h"
#include "utils/strutils.h"

void PointCloudSplitterDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("PointCloudSplitter", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			autoUi(cont->properties());
			EndDisable(!enabled);

			ImGui::Text("\nInfo");
			constexpr auto names = magic_enum::enum_names<PointCloudSplitter::RenderModel>();
			const auto& counters = cont->counters();
			for (int i = 0; i < counters.size(); ++i) {
				ImGui::Text(MAKE_STR(" - " << names[i] << ": " << counters[i].count).c_str());
			}
		}
	}
}
