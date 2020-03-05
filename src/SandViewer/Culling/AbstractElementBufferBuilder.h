#pragma once

#include <vector>
#include <glad/modernglad.h>

class GlBuffer;

struct Settings {
	GLuint pointCount = 20;
	GLuint instancedMeshPointCount = 314;
	int benchmarkRepeat = 1; // number of repetitions when benchmarking
	bool testResult = true; // when benchmarking, also test output

	// The next three must sum to at most 1
	float impostorProportion = 0.1f;
	float instanceProportion = 0.2f;
	float pointProportion = 0.5f;

	// Must be in sync with shader qualifiers
	int local_size_x = 128;

	// Here to avoid magic values in the code
	std::string prefixSumShaderName = "benchmark-prefixsum-culling";
	std::string globalAtomicShaderName = "benchmark-globalatomic-culling";
};

/**
 * Class responsible for building the grouped element buffer, where all indices
 * for one model are in a contibuous section of the element buffer.
 */
class AbstractElementBufferBuilder {
public:
	enum RenderModel {
		ImpostorModel = 0,
		InstanceModel,
		PointModel,
		NoModel,
		_RenderModelCount
	};
	static const std::vector<std::string> renderModelNames;

public:
	bool init(const Settings & settings);
	bool check();

	/**
	 * Load shaders and stuff, can call pointCount() and renderTypeData()
	 */
	virtual bool load(const Settings & settings) = 0;

	/**
	 * Core build function to be benchmarked
	 */
	virtual void build() = 0;

	/**
	 * Return element buffer once build() has been called
	 */
	virtual const GlBuffer & elementBuffer() = 0;

protected:
	GLuint pointCount() { return m_pointCount; }
	const std::vector<GLuint> & renderTypeData() { return m_renderTypeData; }

private:
	static bool CheckElementBuffer(const GlBuffer & elementBuffer, GLuint pointCount, const std::vector<GLuint> & renderTypeData);

private:
	GLuint m_pointCount;
	std::vector<GLuint> m_renderTypeData;
};
