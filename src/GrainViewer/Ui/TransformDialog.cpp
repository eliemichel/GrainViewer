/**
 * This file is part of GrainViewer
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

#include "TransformDialog.h"
#include "utils/guiutils.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

void TransformDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Post Transform
			glm::mat4 postTransform = cont->postTransform();
			glm::vec3 translate = postTransform[3];
			ImGui::SliderFloat3("Post Translate", glm::value_ptr(translate), -1.0f, 1.0f, "%.5f");
			for (int k = 0 ; k < 3 ; ++k) postTransform[3][k] = translate[k];
			cont->setPostTransform(postTransform);
		}
	}
}
