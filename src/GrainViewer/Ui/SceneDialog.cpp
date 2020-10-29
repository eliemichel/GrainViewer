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

#include "SceneDialog.h"
#include "Light.h"
#include "utils/guiutils.h"
#include "utils/behaviorutils.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

void SceneDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Take Screenshot")) {
				cont->takeScreenshot();
			}

			if (cont->isPaused()) {
				if (ImGui::Button("Play")) {
					cont->play();
				}
			}
			else {
				if (ImGui::Button("Pause")) {
					cont->pause();
				}
			}

			autoUi(cont->properties());
		}
	}
}
