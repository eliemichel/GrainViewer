
#include "ImpostorSandRendererDialog.h"
#include "utils/guiutils.h"
#include "utils/behaviorutils.h"
#include <imgui.h>

void ImpostorSandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("ImpostorSandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			autoUi(cont->properties());
			EndDisable(!enabled);
		}
	}
}
