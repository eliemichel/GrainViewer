
#include "QuadMeshDataDialog.h"
#include "utils/guiutils.h"
#include "utils/behaviorutils.h"
#include <imgui.h>

void QuadMeshDataDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("QuadMeshData", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			//
			EndDisable(!enabled);
		}
	}
}
