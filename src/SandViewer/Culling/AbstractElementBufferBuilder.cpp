#include <set>
#include "Logger.h"
#include "GlBuffer.h"
#include "AbstractElementBufferBuilder.h"

//#define EXTRA_DEBUG

const std::vector<std::string> AbstractElementBufferBuilder::renderModelNames = {
		"ImpostorModel",
		"InstanceModel",
		"PointModel",
		"NoModel",
};

bool AbstractElementBufferBuilder::init(const Settings & settings) {
	m_pointCount = settings.pointCount;

	LOG << "Generating reference buffer...";
	LOG << "(Reference buffer is a buffer with numbers from 0 to 3 telling which model to use. It mocks the actual culling procedures.)";
	auto randint = [](int upper) {
		int x = upper;
		while (x >= upper)
			x = std::rand() / ((RAND_MAX + 1u) / upper);
		return x;
	};
	m_renderTypeData.resize(pointCount());
	for (GLuint i = 0; i < m_renderTypeData.size(); ++i) {
		m_renderTypeData[i] = static_cast<GLuint>(randint(_RenderModelCount));
	}

#ifdef EXTRA_DEBUG
	std::ostringstream ss;
	for (GLuint i = 0; i < m_renderTypeData.size(); ++i) {
		if (i != 0) ss << ", ";
		ss << m_renderTypeData[i];
	}
	LOG << "input renderTypeData: [" << ss.str() << "]";
#endif // EXTRA_DEBUG

	return load(settings);
}

bool AbstractElementBufferBuilder::check() {
	return CheckElementBuffer(elementBuffer(), m_pointCount, m_renderTypeData);
}

//-----------------------------------------------------------------------------

bool AbstractElementBufferBuilder::CheckElementBuffer(const GlBuffer & elementBuffer, GLuint pointCount, const std::vector<GLuint> & renderTypeData)
{
	bool status = true;
	// string streams for display
	std::ostringstream elementSs, modelSs, visitedSs;

	std::vector<bool> visited(pointCount, false);

	// Check that all models are single contiguous blocks
	elementBuffer.readBlock<GLuint>(0, [&status, &renderTypeData, &elementSs, &modelSs, &visited](GLuint *data, size_t size) {
		DEBUG_LOG << "Reading output element buffer...";
		auto seenModels = std::set<GLuint>();
		GLuint currentModel = 999;
		for (size_t i = 0; i < size; ++i) {
			int model = renderTypeData[data[i]];
			if (currentModel != model) {
				if (seenModels.count(model) > 0) {
					ERR_LOG << "Invalid output element buffer at index #" << i << ": new block of model type " << model << " starts here.";
					status = false;
				}
				seenModels.insert(model);
				currentModel = model;
			}
			visited[data[i]] = true;

#ifdef EXTRA_DEBUG
			if (i != 0) {
				elementSs << ", ";
				modelSs << ", ";
			}
			elementSs << data[i];
			modelSs << model;
#endif // EXTRA_DEBUG
		}
	});

#ifdef EXTRA_DEBUG
	DEBUG_LOG << "Output Element Buffer: [" << elementSs.str() << "]";
	DEBUG_LOG << "Output Models: [" << modelSs.str() << "]";
#endif // EXTRA_DEBUG

	// Check that all elements are present in output
	for (size_t i = 0; i < visited.size(); ++i) {
		if (!visited[i]) {
			ERR_LOG << "Element #" << i << " is not present in output element buffer.";
			status = false;
		}
#ifdef EXTRA_DEBUG
		if (i != 0) visitedSs << ", ";
		visitedSs << (visited[i] ? 1 : 0);
#endif // EXTRA_DEBUG
	}

#ifdef EXTRA_DEBUG
	DEBUG_LOG << "Visited: [" << visitedSs.str() << "]";
#endif // EXTRA_DEBUG

	return status;
}
