#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "utils/guiutils.h"
#include "DeferredShadingDialog.h"
#include "GlTexture.h"

void DeferredShadingDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Deferred Shading", ImGuiTreeNodeFlags_DefaultOpen)) {
			GlDeferredShader::Properties & props = cont->properties();

			int mode = props.shadingMode;
			ImGui::Text("\nShading Mode");
			ImGui::RadioButton("Beauty", &mode, GlDeferredShader::BeautyPass);
			ImGui::RadioButton("Normal", &mode, GlDeferredShader::NormalPass);
			ImGui::RadioButton("Base Color", &mode, GlDeferredShader::BaseColorPass);
			ImGui::RadioButton("Metallic", &mode, GlDeferredShader::MetallicPass);
			ImGui::RadioButton("Roughness", &mode, GlDeferredShader::RoughnessPass);
			ImGui::RadioButton("World Position", &mode, GlDeferredShader::WorldPositionPass);
			ImGui::RadioButton("Depth", &mode, GlDeferredShader::DepthPass);
			ImGui::RadioButton("Raw GBuffer 0", &mode, GlDeferredShader::RawGBuffer0);
			ImGui::RadioButton("Raw GBuffer 1", &mode, GlDeferredShader::RawGBuffer1);
			ImGui::RadioButton("Raw GBuffer 2", &mode, GlDeferredShader::RawGBuffer2);
			props.shadingMode = static_cast<GlDeferredShader::ShadingMode>(mode);

			ImGui::Checkbox("Transparent film", &props.transparentFilm);
			ImGui::Checkbox("Show Sample Count", &props.showSampleCount);
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
