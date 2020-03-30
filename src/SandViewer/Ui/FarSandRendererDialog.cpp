#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "utils/guiutils.h"
#include "FarSandRendererDialog.h"

void FarSandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("FarSandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			int id = 0; // for disambiguation

			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			FarSandRenderer::Properties & props = cont->properties();
			ImGui::SliderFloat("Radius", &props.radius, 0.0f, 0.1f, "%.5f");

			int shape = props.debugShape;
			ImGui::Text("\nShape");
			ImGui::RadioButton("None", &shape, FarSandRenderer::DebugShapeNone);
			ImGui::RadioButton("Lit Sphere", &shape, FarSandRenderer::DebugShapeLitSphere);
			ImGui::RadioButton("Disc", &shape, FarSandRenderer::DebugShapeDisc);
			ImGui::RadioButton("Square", &shape, FarSandRenderer::DebugShapeSquare);
			ImGui::RadioButton("Normal Sphere", &shape, FarSandRenderer::DebugShapeNormalSphere);
			props.debugShape = static_cast<FarSandRenderer::DebugShape>(shape);

			ImGui::Spacing();
			ImGui::Checkbox("Use Shell Culling", &props.useShellCulling);
			BeginDisable(!props.useShellCulling);
			{
				ImGui::SliderFloat("Shell Thickness", &props.epsilonFactor, 0.01f, 20.0f, "%.5f");
				ImGui::Checkbox("Depth-based Falloff", &props.shellDepthFalloff);
				
				int weightMode = props.weightMode;
				ImGui::PushID(id++);
				ImGui::Text("\nWeight Mode");
				ImGui::RadioButton("None", &weightMode, FarSandRenderer::WeightModeNone);
				ImGui::RadioButton("Linear", &weightMode, FarSandRenderer::WeightModeLinear);
				ImGui::RadioButton("Quad", &weightMode, FarSandRenderer::WeightModeQuad);
				//ImGui::RadioButton("Gaussian", &weightMode, FarSandRenderer::WeightModeGaussian);
				props.weightMode = static_cast<FarSandRenderer::WeightMode>(weightMode);
				ImGui::PopID();


				int shellCullingStrategy = props.shellCullingStrategy;
				ImGui::Text("\nShell Culling Strategy");
				ImGui::RadioButton("Frag Depth", &shellCullingStrategy, FarSandRenderer::ShellCullingFragDepth);
				ImGui::RadioButton("Move Away (recommended)", &shellCullingStrategy, FarSandRenderer::ShellCullingMoveAway);
				ImGui::RadioButton("Depth Range", &shellCullingStrategy, FarSandRenderer::ShellCullingDepthRange);
				props.shellCullingStrategy = static_cast<FarSandRenderer::ShellCullingStrategy>(shellCullingStrategy);

				ImGui::Checkbox("Use Early Depth Test", &props.useEarlyDepthTest);
				ImGui::Checkbox("No Discard In Epsilon ZPass", &props.noDiscardInEpsilonZPass);
				ImGui::Checkbox("Use Extra Fbo", &props.extraFbo);
				BeginDisable(props.extraFbo);
				{
					ImGui::Checkbox("Extra Init Pass", &props.extraInitPass);
				}
				EndDisable(props.extraFbo);
			}
			EndDisable(!props.useShellCulling);

			ImGui::Text("\n");
			ImGui::Checkbox("Use Bbox Culling", &props.useBbox);
			BeginDisable(!props.useBbox);
			{
				ImGui::SliderFloat3("Minimum", glm::value_ptr(props.bboxMin), 0.001f, 1.0f, "%.5f");
				ImGui::SliderFloat3("Maximum", glm::value_ptr(props.bboxMax), 0.001f, 1.0f, "%.5f");
			}
			EndDisable(!props.useBbox);

			ImGui::Text("\nDebug");
			ImGui::Checkbox("Disable additive blend", &props.disableBlend);

			EndDisable(!enabled);
		}
	}
}
