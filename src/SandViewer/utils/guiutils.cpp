#include <imgui.h>
#include <imgui_internal.h>
#include "guiutils.h"

void BeginDisable(bool disable) {
	if (disable) {
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
}

void EndDisable(bool disable) {
	if (disable) {
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
}

