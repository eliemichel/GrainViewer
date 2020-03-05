#pragma once

#include <memory>
#include "AbstractElementBufferBuilder.h"

class GlBuffer;
class ShaderProgram;

typedef std::shared_ptr<ShaderProgram> ShaderPtr;

class PrefixSumElementBufferBuilder : public AbstractElementBufferBuilder {
public:
	bool load(const Settings & settings) override;
	void build() override;
	const GlBuffer & elementBuffer() override { return m_elementBuffers[0]; };

private:
	enum PrefixSumCullingSteps {
		MarkCulling = 0,
		Group,
		BuildCommandBuffer
	};

	/**
	 * Info about a subsection of the output element buffer corresponding to one
	 * single render model.
	 */
	struct ElementBufferSectionInfo {
		// Number of elements rendered with this model
		GLuint count;
		// Cumulated number of elements rendered with previous models
		GLuint offset;
		// Keep culling flag for last elements, because the prefix sum discards them
		GLuint isLastPointActive;
		GLuint _pad;
	};

private:
	// Culling steps

	/**
	 * Fill the output buffer with zeros and ones to flag which elements must be
	 * rendered using the selected render model.
	 */
	void mark(RenderModel renderModel, const GlBuffer & reference, const GlBuffer & output);

	/**
	 * Group elements rendered using the same model tegether, at an offset in
	 * output buffer given by the prefixsuminfo SSBO. This must be called for each
	 * model in increasing order.
	 */
	void group(RenderModel renderModel, const GlBuffer & input, const GlBuffer & output);

private:
	static std::vector<ShaderPtr> LoadCullingShaders(const std::string & baseFile);
	static void DisplayInfoBuffers(const GlBuffer & sectionInfoSsbo);

private:
	std::vector<ShaderPtr> m_cullingShaders;
	ShaderPtr m_prefixSumShader;

	GlBuffer m_sectionInfoSsbo = GL_SHADER_STORAGE_BUFFER;
	GlBuffer m_renderTypeSsbo = GL_SHADER_STORAGE_BUFFER;
	GlBuffer m_elementBuffers[3] = { GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER };
	int m_xWorkGroups;
};

