#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "utils/guiutils.h"
#include "Sand6DataDialog.h"
#include "utils/behaviorutils.h"

void Sand6DataDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("Sand6Data", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			autoUi(cont->properties());
			EndDisable(!enabled);
		}
	}
}
