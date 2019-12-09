#include <limits>
#include <imgui.h>
#include "utils/guiutils.h"
#include "FarSandRendererDialog.h"

void FarSandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("FarSandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			FarSandRenderer::Properties & props = cont->properties();
			ImGui::SliderFloat("Radius", &props.radius, 0.0f, 0.1f, "radius = %.5f");
			EndDisable(!enabled);
		}
	}
}
