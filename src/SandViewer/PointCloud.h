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

	bool loadXYZ(const std::string & filename);

	bool loadBin(const std::string & filename);
	bool saveBin(const std::string & filename);

	size_t frameCount() const { return m_frame_count; }
	const std::vector<glm::vec3> & data() const { return m_data; }

private:
	std::vector<glm::vec3> m_data;
	size_t m_frame_count = 1;
};
