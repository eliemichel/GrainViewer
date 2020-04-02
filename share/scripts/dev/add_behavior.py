behavior_h_tpl = """
#pragma once

#include <OpenGL>
#include "Behavior.h"
#include <refl.hpp>
#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class TransformBehavior;

class {BehaviorName} : public Behavior {{
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

public:
	// Properties (serialized and displayed in UI)
	struct Properties {{
		bool someOption = true;
	}};
	Properties & properties() {{ return m_properties; }}
	const Properties& properties() const {{ return m_properties; }}

private:
	glm::mat4 modelMatrix() const;

private:
	Properties m_properties;

	std::string m_shaderName = "{DefaultShader}";
	std::shared_ptr<ShaderProgram> m_shader;

	std::weak_ptr<TransformBehavior> m_transform;
}};

REFL_TYPE({BehaviorName}::Properties)
REFL_FIELD(someOption)
REFL_END

registerBehaviorType({BehaviorName})
"""

behavior_cpp_tpl = """
#include "{BehaviorName}.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool {BehaviorName}::deserialize(const rapidjson::Value & json)
{{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	autoDeserialize(json, m_properties);
	return true;
}}

void {BehaviorName}::start()
{{
	m_transform = getComponent<TransformBehavior>();
	m_shader = ShaderPool::GetShader(m_shaderName);
}}

void {BehaviorName}::update(float time, int frame)
{{
	
}}

void {BehaviorName}::render(const Camera& camera, const World& world, RenderType target) const
{{
	const ShaderProgram & shader = *m_shader; // maybe switch shader depending on option/render target
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	shader.use();

	// some draw call here!
}}


//-----------------------------------------------------------------------------

glm::mat4 {BehaviorName}::modelMatrix() const
{{
	if (auto transform = m_transform.lock()) {{
		return transform->modelMatrix();
	}} else {{
		return glm::mat4(1.0f);
	}}
}}

"""

behavior_dialog_h_tpl = """
#pragma once

#include "Behavior/{BehaviorName}.h"
#include "Dialog.h"
#include <memory>

class {BehaviorName}Dialog : public Dialog {{
public:
	void draw() override;
	void setControlledBehavior(std::weak_ptr<{BehaviorName}> behavior) {{ m_cont = behavior; }}

private:
	std::weak_ptr<{BehaviorName}> m_cont;
}};

registerDialogForBehavior({BehaviorName}Dialog, {BehaviorName})
"""

behavior_dialog_cpp_tpl = """
#include "{BehaviorName}Dialog.h"
#include "utils/guiutils.h"
#include "utils/behaviorutils.h"
#include <imgui.h>

void {BehaviorName}Dialog::draw()
{{
	if (auto cont = m_cont.lock()) {{
		if (ImGui::CollapsingHeader("{BehaviorName}", ImGuiTreeNodeFlags_DefaultOpen)) {{
			bool enabled = cont->isEnabled();
			ImGui::Checkbox("Enabled", &enabled);
			cont->setEnabled(enabled);

			BeginDisable(!enabled);
			autoUi(cont->properties());
			EndDisable(!enabled);
		}}
	}}
}}
"""

from os import path

def dirname(p, n): # recursive dirname
	return dirname(path.dirname(p), n - 1) if n > 0 else p
ROOT = path.join(dirname(path.realpath(__file__), 4), "src", "SandViewer")

data = {
	"BehaviorName": "TestBehavior",
	"DefaultShader": "TestShader"
}

def f(fmt):
	return fmt.format(**data)

behavior_filename = path.join(ROOT, "Behavior", data["BehaviorName"])
behavior_dialog_filename = path.join(ROOT, "Ui", data["BehaviorName"] + "Dialog")
registry_cpp_filename = path.join(ROOT, "BehaviorRegistry.cpp")
gui_cpp_filename = path.join(ROOT, "Ui", "Gui.cpp")
cmakelists_filename = path.join(ROOT, "CMakeLists.txt")

print("Creating {}.h/cpp...".format(behavior_filename))
with open(behavior_filename + ".h", 'w', encoding="utf8") as file:
	file.write(f(behavior_h_tpl))

with open(behavior_filename + ".cpp", 'w', encoding="utf8") as file:
	file.write(f(behavior_cpp_tpl))

print("Creating {}.h/cpp...".format(behavior_dialog_filename))
with open(behavior_dialog_filename + ".h", 'w', encoding="utf8") as file:
	file.write(f(behavior_dialog_h_tpl))

with open(behavior_dialog_filename + ".cpp", 'w', encoding="utf8") as file:
	file.write(f(behavior_dialog_cpp_tpl))

print("Updating {}...".format(registry_cpp_filename))
with open(registry_cpp_filename, 'r', encoding="utf8") as file:
	new_lines = []
	for line in file:
		if line.strip().startswith("void BehaviorRegistry::addBehavior"):
			new_lines.insert(-1, f("#include \"Behavior/{BehaviorName}.h\"\n"))
		elif line.strip() == "#undef handleType":
			new_lines.append(f("	handleType({BehaviorName});\n"))
		new_lines.append(line)
with open(registry_cpp_filename, 'w', encoding="utf8") as file:
	for line in new_lines:
		file.write(line)

print("Updating {}...".format(gui_cpp_filename))
with open(gui_cpp_filename, 'r', encoding="utf8") as file:
	new_lines = []
	for line in file:
		if line.strip().startswith("#define handleBehavior(T)"):
			new_lines.insert(-1, f("#include \"{BehaviorName}Dialog.h\"\n"))
		elif line.strip() == "#undef handleType":
			new_lines.insert(-1, f("	handleBehavior({BehaviorName});\n"))
		new_lines.append(line)
with open(gui_cpp_filename, 'w', encoding="utf8") as file:
	for line in new_lines:
		file.write(line)

print("Updating {}...".format(cmakelists_filename))
with open(cmakelists_filename, 'r', encoding="utf8") as file:
	new_lines = []
	b_added = False
	u_added = False
	start_tag_seen = False
	for line in file:
		if line.strip().startswith("set(All_SRC"):
			start_tag_seen = True
		if start_tag_seen:
			if not b_added and line.strip() == "" and new_lines[-1].strip().startswith("Behavior"):
				new_lines.append(f("	Behavior/{BehaviorName}.h\n"))
				new_lines.append(f("	Behavior/{BehaviorName}.cpp\n"))
				b_added = True
			if not u_added and line.strip() == "" and new_lines[-1].strip().startswith("Ui"):
				new_lines.append(f("	Ui/{BehaviorName}Dialog.h\n"))
				new_lines.append(f("	Ui/{BehaviorName}Dialog.cpp\n"))
				u_added = True
		new_lines.append(line)
with open(cmakelists_filename, 'w', encoding="utf8") as file:
	for line in new_lines:
		file.write(line)

print("Done.")
