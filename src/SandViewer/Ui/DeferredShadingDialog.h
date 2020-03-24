#pragma once

#include <memory>
#include "Dialog.h"
#include "GlDeferredShader.h"

class DeferredShadingDialog : public Dialog {
public:
	void draw() override;
	void drawHandles(float x, float y, float w, float h) override;
	void setController(std::weak_ptr<GlDeferredShader> shading) { m_cont = shading; }

private:
	std::weak_ptr<GlDeferredShader> m_cont;
};
