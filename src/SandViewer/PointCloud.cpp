// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <iostream>
#include <fstream>

#include "Logger.h"
#include "PointCloud.h"

bool PointCloud::loadXYZ(const std::string & filename) {
	std::ifstream in(filename);
	if (!in.is_open()) {
		ERR_LOG << filename << " is not a valid XYZ file.";
		return false;
	}

	float x, y, z;
	while (in >> x >> y >> z) {
		m_data.push_back(glm::vec3(x, y, z));
	}

	in.close();
	LOG << "Loaded cloud of " << m_data.size() << " points from " << filename;
	return true;
}

bool PointCloud::loadBin(const std::string & filename) {
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

bool PointCloud::saveBin(const std::string & filename) {
	std::ofstream out(filename, std::ios::binary);
	if (!out.is_open()) {
		ERR_LOG << filename << " is not a writable file.";
		return false;
	}

	float header[2];
	header[0] = static_cast<float>(m_data.size() / m_frame_count);
	header[1] = static_cast<float>(m_frame_count);
	if (!out.write(reinterpret_cast<const char*>(header), 2 * sizeof(float))) {
		ERR_LOG << "Could not write point buffer in file: " << filename;
		out.close();
		return false;
	}

	if (!out.write(reinterpret_cast<const char*>(m_data.data()), m_data.size() * sizeof(float) * 3)) {
		ERR_LOG << "Could not write point buffer in file: " << filename;
		out.close();
		return false;
	}

	out.close();
	LOG << "Saved cloud of " << m_data.size() << " points to " << filename << " (" << m_frame_count << " frames of " << (m_data.size() / m_frame_count) << " points)";
	return true;
}
