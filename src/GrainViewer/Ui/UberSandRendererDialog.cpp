#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "utils/guiutils.h"
#include "UberSandRendererDialog.h"
#include "utils/behaviorutils.h"

void UberSandRendererDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("UberSandRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			autoUi(cont->properties());
			EndDisable(!enabled);
		}
	}
}
