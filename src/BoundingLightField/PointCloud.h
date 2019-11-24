// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>

#include "Logger.h"

using glm::vec3;

class PointCloud {
public:
	PointCloud() {}
	PointCloud(const std::string & filename) { loadXYZ(filename); }

	bool loadXYZ(const std::string & filename) {
		std::ifstream in(filename);
		if (!in.is_open()) {
			ERR_LOG << filename << " is not a valid XYZ file.";
			return false;
		}

		float x, y, z;
		while (in >> x >> y >> z) {
			m_data.push_back(vec3(x, y, z));
		}

		in.close();
		LOG << "Loaded cloud of " << m_data.size() << " points from " << filename;
		return true;
	}

	bool loadBin(const std::string & filename) {
		std::ifstream in(filename, std::ios::binary);
		if (!in.is_open()) {
			ERR_LOG << filename << " is not a valid file.";
			return false;
		}

		float header[2];
		if (!in.read(reinterpret_cast<char*>(header), 2 * sizeof(float))) {
			ERR_LOG << "Could not read point buffer from file: " << filename;
			in.close();
			return false;
		}
		size_t point_count = static_cast<size_t>(header[0]);
		m_frame_count = static_cast<size_t>(header[1]);
		size_t size = point_count * m_frame_count;
		
		m_data.resize(size);
		if (!in.read(reinterpret_cast<char*>(m_data.data()), size * sizeof(float) * 3)) {
			ERR_LOG << "Could not read point buffer from file: " << filename;
			in.close();
			return false;
		}

		in.close();
		LOG << "Loaded cloud of " << size << " points from " << filename << " (" << m_frame_count << " frames of " << point_count << " points)";
		return true;
	}

	size_t frameCount() const { return m_frame_count; }
	const std::vector<vec3> & data() const { return m_data; }

private:
	std::vector<vec3> m_data;
	size_t m_frame_count = 1;
};
