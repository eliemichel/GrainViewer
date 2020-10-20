// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

class PointCloud {
public:
	PointCloud() {}
	PointCloud(const std::string & filename) { loadXYZ(filename); }

	// Guess codec using extension
	bool load(const std::string & filename);

	// Force XYZ codec
	bool loadXYZ(const std::string & filename);

	// Force Bin codec (basically some ad-hoc memory dump)
	bool loadBin(const std::string & filename);

	// Force Raw codec (as specified in BlueNoise.py - by Moment in Graphics)
	// threshold is used to determine the point density
	bool loadMomentRaw(const std::string & filename, float threshold = 0.1f);

	bool saveBin(const std::string & filename);

	size_t frameCount() const { return m_frame_count; }
	const std::vector<glm::vec3> & data() const { return m_data; }
	std::vector<glm::vec3> & data() { return m_data; }

private:
	std::vector<glm::vec3> m_data;
	size_t m_frame_count = 1;
};
