#include "SandRendererDialog.h"
#include <imgui.h>

void SandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("SandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			float grainRadius = cont->grainRadius();
			ImGui::SliderFloat("Grain Radius", &grainRadius, 0.00f, 0.1f, "radius = %.5f");
			cont->setGrainRadius(grainRadius);

			float grainMeshScale = cont->grainMeshScale();
			ImGui::SliderFloat("Grain Mesh Scale", &grainMeshScale, 0.0f, 10.0f, "scale = %.3f");
			cont->setGrainMeshScale(grainMeshScale);
		}
	}
}

void SandRendererDialog::setControlledBehavior(std::weak_ptr<SandRenderer> behavior)
{
	m_cont = behavior;
}
