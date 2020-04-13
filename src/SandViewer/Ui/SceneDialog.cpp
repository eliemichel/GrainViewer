#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "utils/guiutils.h"
#include "utils/behaviorutils.h"
#include "SceneDialog.h"
#include "Light.h"

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
