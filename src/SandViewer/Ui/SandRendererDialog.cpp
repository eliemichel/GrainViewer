#include <limits>

#include <imgui.h>

#include "utils/guiutils.h"

#include "SandRendererDialog.h"

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

			ImGui::SliderFloat("Grain Instance Scale", &props.grainMeshScale, 0.0f, 10.0f, "scale = %.3f");
			
			{
				ImGui::Text("\nModel Selection");

				static float instanceLimit = 0.0f;

				static int modelType = 2;
				int prevModelType = modelType;
				ImGui::RadioButton("Only Instances", &modelType, 0);
				ImGui::RadioButton("Only Impostors", &modelType, 1);
				ImGui::RadioButton("Use Limit Distance", &modelType, 2);

				if (modelType == 2 && prevModelType == 2) {
					instanceLimit = props.instanceLimit;
				}

				ImGui::SameLine();

				BeginDisable(modelType != 2);
				ImGui::SliderFloat("Instance Limit Distance", &instanceLimit, 1.0f, 5.0f, "distance = %.2f");
				EndDisable(modelType != 2);

				props.instanceLimit = modelType == 0 ? std::numeric_limits<float>::infinity() : (modelType == 1 ? 0 : instanceLimit);
			}

			{
				ImGui::Text("\nCulling");
				ImGui::SliderFloat("Inner Radius", &props.grainInnerRadiusRatio, 0.0f, .9999f, "ratio = %.5f");
				ImGui::Checkbox("Occlusion Culling", &props.enableOcclusionCulling);
				ImGui::Checkbox("Frustum Culling", &props.enableFrustumCulling);
				ImGui::Checkbox("Distance Culling", &props.enableDistanceCulling);
			}

			{
				ImGui::Text("\nDebug");
				ImGui::Checkbox("Hide Impostors", &props.disableImpostors);
				ImGui::Checkbox("Hide Instances", &props.disableInstances);
				ImGui::Checkbox("Render Additive (for debug)", &props.renderAdditive);
				ImGui::Checkbox("Has Metallic Roughness Map", &props.hasMetallicRoughnessMap);

				const SandRenderer::RenderInfo & info = cont->renderInfo();
				ImGui::LabelText("Impostors", "%d", info.impostorCount);
				ImGui::LabelText("Instances", "%d", info.instanceCount);
			}

			EndDisable(!enabled);
		}
	}
}
