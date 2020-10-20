#include <limits>

#include <imgui.h>

#include "Logger.h"
#include "utils/guiutils.h"

#include "LightGizmoDialog.h"

void LightGizmoDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Light Gizmo", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			//BeginDisable(!enabled);
			//EndDisable(!enabled);
		}
	}
}
