#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "utils/guiutils.h"
#include "utils/behaviorutils.h"
#include "DeferredShadingDialog.h"
#include "GlTexture.h"

void DeferredShadingDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Deferred Shading", ImGuiTreeNodeFlags_DefaultOpen)) {
			GlDeferredShader::Properties & props = cont->properties();

			autoUi(props);
			ImGui::Text("Shadow Map Bias: %f", props.ShadowMapBias());
		}
	}
}

void DeferredShadingDialog::drawHandles(float x, float y, float w, float h)
{
	if (auto cont = m_cont.lock()) {
		GlDeferredShader::Properties & props = cont->properties();

		if (props.showSampleCount) {
			ImGui::SetNextWindowPos(ImVec2(x, y + h - 60));
			ImGui::SetNextWindowSize(ImVec2(540, 60));
			ImGui::Begin("DeferredShadingDialog Handle", nullptr,
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoTitleBar);
			
			if (cont->hasColorMap()) {
				const float spacing = ImGui::GetStyle().ItemSpacing.x;

				GLuint tex = cont->colormap().raw();
				ImGui::Image((void*)(intptr_t)tex, ImVec2(540 - 2 * spacing, 20));
				ImGui::Text("0");
				
				static float inputWidth = 30.0f;
				ImGui::SameLine(ImGui::GetWindowWidth() - spacing - inputWidth);
				ImGui::PushItemWidth(inputWidth);
				ImGui::InputFloat("##maxSampleCount", &props.maxSampleCount, 0, 0, "%.0f");
				inputWidth = ImGui::GetItemRectSize().x;
			}

			ImGui::End();
		}
	}
}
