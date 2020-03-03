#pragma once

#include <memory>
#include "GlTexture.h"

struct LeanTexture {
	GlTexture lean1;
	GlTexture lean2;

	LeanTexture(GLenum target) : lean1(target), lean2(target) {}
};

class Filtering {
public:
	/*
	enum Type {
		StandardFiltering,
		LeanFiltering,
	};
	*/

	static std::unique_ptr<LeanTexture> CreateLeanTexture(const GlTexture & sourceTexture);
};
