#include <limits>

#include <imgui.h>
#include <imgui_internal.h>

#include "SandRendererDialog.h"

void BeginDisable(bool disable) {
	if (disable) {
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
}
void EndDisable(bool disable) {
	if (disable) {
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
}

void SandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("SandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);
			BeginDisable(!enabled);

			SandRenderer::Properties & props = cont->properties();

			ImGui::SliderFloat("Grain Radius", &props.grainRadius, 0.0f, 0.1f, "radius = %.5f");
			ImGui::SliderFloat("Inner Radius", &props.grainInnerRadiusRatio, 0.0f, 1.0f, "ratio = %.5f");

			ImGui::SliderFloat("Grain Mesh Scale", &props.grainMeshScale, 0.0f, 10.0f, "scale = %.3f");
			
			// Instance limit distance
			static bool onlyInstances = false;
			static bool onlyImpostors = false;
			static float instanceLimit = 0.0f;
			bool wasOnlyInstances = onlyInstances;
			bool wasOnlyImpostors = onlyImpostors;
			ImGui::Checkbox("Only Instances", &onlyInstances);
			ImGui::Checkbox("Only Impostors", &onlyImpostors);
			if (!onlyInstances && !wasOnlyInstances && !onlyImpostors && !wasOnlyImpostors) {
				instanceLimit = props.instanceLimit;
			}

			BeginDisable(onlyInstances || onlyImpostors);
			ImGui::SliderFloat("Instance Limit Distance", &instanceLimit, 1.0f, 5.0f, "distance = %.2f");
			EndDisable(onlyInstances || onlyImpostors);

			props.instanceLimit = onlyInstances ? std::numeric_limits<float>::infinity() : (onlyImpostors ? 0 : instanceLimit);

			ImGui::Checkbox("Disable Impostors", &props.disableImpostors);
			ImGui::Checkbox("Disable Instances", &props.disableInstances);

			ImGui::Checkbox("Occlusion Culling", &props.enableOcclusionCulling);
			ImGui::Checkbox("Frustum Culling", &props.enableFrustumCulling);
			ImGui::Checkbox("Distance Culling", &props.enableDistanceCulling);

			const SandRenderer::RenderInfo & info = cont->renderInfo();
			ImGui::LabelText("Impostors", "%d", info.impostorCount);
			ImGui::LabelText("Instances", "%d", info.instanceCount);

			EndDisable(!enabled);
		}
	}
}
