#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "utils/guiutils.h"
#include "TransformDialog.h"

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
