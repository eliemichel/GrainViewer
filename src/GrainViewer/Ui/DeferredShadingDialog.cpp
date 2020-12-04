/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "DeferredShadingDialog.h"
#include "utils/guiutils.h"
#include "utils/behaviorutils.h"
#include "GlTexture.h"

#include <glm/gtc/type_ptr.hpp>

#include <limits>
#include <imgui.h>

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
