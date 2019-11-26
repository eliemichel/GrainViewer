#include <limits>

#include <imgui.h>
#include <imgui_internal.h>

#include "SandRendererDialog.h"

void SandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("SandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			SandRenderer::Properties & props = cont->properties();

			ImGui::SliderFloat("Grain Radius", &props.grainRadius, 0.00f, 0.1f, "radius = %.5f");
			
			ImGui::SliderFloat("Grain Mesh Scale", &props.grainMeshScale, 0.0f, 10.0f, "scale = %.3f");
			
			// Instance limit distance
			static bool onlyInstances = false;
			static float instanceLimit = 0.0f;
			bool wasOnlyInstances = onlyInstances;
			ImGui::Checkbox("Only Instances", &onlyInstances);
			if (onlyInstances) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			} else if (!wasOnlyInstances) {
				instanceLimit = props.instanceLimit;
			}
			ImGui::SliderFloat("Instance Limit Distance", &instanceLimit, 1.0f, 5.0f, "distance = %.2f");
			if (onlyInstances) {
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
			props.instanceLimit = onlyInstances ? std::numeric_limits<float>::infinity() : instanceLimit;

			ImGui::Checkbox("Disable Impostors", &props.disableImpostors);
			ImGui::Checkbox("Disable Instances", &props.disableInstances);
		}
	}
}
