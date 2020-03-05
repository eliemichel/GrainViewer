#include "GlBuffer.h"
#include "ShaderPool.h"
#include "prefixSum.h"
#include "PrefixSumElementBufferBuilder.h"

// Disable debug logs
#undef DEBUG_LOG
#define DEBUG_LOG if (false) Logger("", "", 0, Logger::LDEBUG).stream()

// For debug, assume that GlBuffer is filled with GLuint
static std::ostream & operator<<(std::ostream & out, const GlBuffer & buffer) {
	out << "[";
	buffer.readBlock<GLuint>(0, [&out](GLuint *data, size_t size) {
		for (int i = 0; i < size; ++i) {
			if (i != 0) out << ", ";
			out << data[i];
		}
	});
	out << "]";
	return out;
}

//-----------------------------------------------------------------------------

bool PrefixSumElementBufferBuilder::load(const Settings & settings)
{
	m_xWorkGroups = (settings.pointCount + (settings.local_size_x - 1)) / settings.local_size_x;

	LOG << "Loading shaders...";
	m_cullingShaders = LoadCullingShaders(settings.shaderName);
	if (m_cullingShaders.empty()) return false;
	m_prefixSumShader = ShaderPool::GetShader("PrefixSum");
	if (!m_prefixSumShader->isValid()) return false;

	LOG << "Allocate element buffer section info SSBO...";
	m_sectionInfoSsbo.addBlock<ElementBufferSectionInfo>(_RenderModelCount);
	m_sectionInfoSsbo.alloc();

	LOG << "Generating 3 element buffers...";
	LOG << "(One element buffer will be the output, the others are used as intermediary steps for prefix sum)";
	for (int i = 0; i < 3; ++i) {
		m_elementBuffers[i].addBlock<GLuint>(settings.pointCount);
		m_elementBuffers[i].alloc();
	}

	m_renderTypeSsbo.importBlock(renderTypeData());

	return true;
}

void PrefixSumElementBufferBuilder::build() {
	DEBUG_LOG << "RenderTypeSsbo: " << m_renderTypeSsbo;
	for (int i = 0; i < _RenderModelCount; ++i) {
		RenderModel renderModel = static_cast<RenderModel>(i);
		DEBUG_LOG << (i + 1) << ". " << renderModelNames[renderModel];
		DEBUG_LOG << " - Mark";
		mark(renderModel, m_renderTypeSsbo, m_elementBuffers[1]);
		DEBUG_LOG << "     Mark buffer: " << m_elementBuffers[1];
		DEBUG_LOG << " - Prefix sum";
		int resultIndex = prefixSum(m_elementBuffers[1], m_elementBuffers[2], *m_prefixSumShader, pointCount());
		DEBUG_LOG << "     Summed marks: " << m_elementBuffers[1 + resultIndex];
		DEBUG_LOG << " - Group";
		group(renderModel, m_elementBuffers[1 + resultIndex], m_elementBuffers[0]);
		DEBUG_LOG << "     Current Element Buffer: " << m_elementBuffers[0];
	}
}

//-----------------------------------------------------------------------------

void PrefixSumElementBufferBuilder::mark(RenderModel renderModel, const GlBuffer & reference, const GlBuffer & output)
{
	const ShaderProgram & shader = *m_cullingShaders[MarkCulling];
	shader.use();
	shader.setUniform("uType", static_cast<GLuint>(renderModel));
	shader.setUniform("uPointCount", pointCount());
	m_sectionInfoSsbo.bindSsbo(0);
	reference.bindSsbo(1);
	output.bindSsbo(2);

	glDispatchCompute(static_cast<GLuint>(m_xWorkGroups), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void PrefixSumElementBufferBuilder::group(RenderModel renderModel, const GlBuffer & input, const GlBuffer & output)
{
	const ShaderProgram & shader = *m_cullingShaders[Group];
	shader.use();
	shader.setUniform("uType", static_cast<GLuint>(renderModel));
	shader.setUniform("uPointCount", pointCount());
	m_sectionInfoSsbo.bindSsbo(0);
	input.bindSsbo(1);
	output.bindSsbo(2);

	glDispatchCompute(static_cast<GLuint>(m_xWorkGroups), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//-----------------------------------------------------------------------------

static const std::vector<std::string> prefixSumCullingSteps = {
	"STEP_MARK_CULLING",
	"STEP_GROUP",
	"STEP_BUILD_COMMAND_BUFFER",
};

std::vector<ShaderPtr> PrefixSumElementBufferBuilder::LoadCullingShaders(const std::string & baseFile)
{
	std::string baseName = "Culling";
	ShaderPool::AddShader(baseName, baseFile, ShaderProgram::ComputeShader, {});

	std::vector<ShaderPtr> shaders;
	for (const auto & step : prefixSumCullingSteps) {
		std::string name = baseName + "_STEP_" + step;
		ShaderPool::AddShaderVariant(name, baseName, step);
		shaders.push_back(ShaderPool::GetShader(name));
		if (!shaders.back()->isValid()) {
			ERR_LOG << "Invalid shader";
			return {};
		}
	}

	return shaders;
}

void PrefixSumElementBufferBuilder::DisplayInfoBuffers(const GlBuffer & sectionInfoSsbo)
{
	ElementBufferSectionInfo sectionInfo[_RenderModelCount];
	sectionInfoSsbo.readBlock<ElementBufferSectionInfo>(0, [&sectionInfo](ElementBufferSectionInfo *data, size_t size) {
		for (int i = 0; i < size; ++i) {
			sectionInfo[i] = data[i];
		}
	});

	std::cout
		<< "SectionInfo [\n";
	for (int i = 0; i < _RenderModelCount; ++i) {
		std::cout
			<< "  " << renderModelNames[i] << ": {"
			<< "    count: " << sectionInfo[i].count << "\n"
			<< "    offset: " << sectionInfo[i].offset << "\n"
			<< "    isLastPointActive: " << sectionInfo[i].isLastPointActive << "\n"
			<< "  }\n";
	}
	std::cout
		<< "  ]\n"
		<< "}\n";
}

