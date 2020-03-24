#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "utils/guiutils.h"
#include "SceneDialog.h"
#include "Light.h"

void SceneDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
		}
	}
}
