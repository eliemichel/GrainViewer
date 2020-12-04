/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "Logger.h"
#include "PointCloud.h"
#include "utils/strutils.h"

#include <iostream>
#include <fstream>

bool PointCloud::load(const std::string & filename)
{
	if (endsWith(filename, ".bin")) {
		return loadBin(filename);
	}
	else if (endsWith(filename, ".raw")) {
		return loadMomentRaw(filename);
	}
	else {
		return loadXYZ(filename);
	}
}

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

#define READ(in, value) in.read(reinterpret_cast<char*>(&(value)), sizeof(value) / sizeof(char))

bool PointCloud::loadMomentRaw(const std::string & filename, float threshold)
{
	std::ifstream in(filename, std::ios::binary);
	if (!in.is_open()) {
		ERR_LOG << "Could not open raw file '" << filename << "'";
		return false;
	}

	uint32_t version, nChannel, nDimensions, shape[3], value;
	READ(in, version);
	READ(in, nChannel);
	READ(in, nDimensions);
	if (nChannel != 1 || nDimensions != 3) {
		ERR_LOG << "Only single channel 3-dimensional raw files can be loaded as point clouds";
		return false;
	}
	READ(in, shape[0]);
	READ(in, shape[1]);
	READ(in, shape[2]);
	size_t voxelCount = shape[0] * shape[1] * shape[2];

	m_frame_count = 1;
	m_data.resize(static_cast<size_t>(0.1f * voxelCount)); // first size guess

	int k = 0;
	uint32_t actualThreshold = static_cast<uint32_t>(threshold * voxelCount);
	for (size_t i = 0; i < voxelCount; ++i) {
		if (k >= m_data.size()) {
			m_data.resize(2 * m_data.size());
		}

		READ(in, value);
		if (value < actualThreshold) {
			size_t u, v, w;
			w = i % shape[1];
			v = (i / shape[1]) % shape[0];
			u = i / (shape[0] * shape[1]);
			float x = static_cast<float>(u) / static_cast<float>(shape[0] - 1) - 0.5f;
			float y = static_cast<float>(v) / static_cast<float>(shape[1] - 1) - 0.5f;
			float z = static_cast<float>(w) / static_cast<float>(shape[2] - 1) - 0.5f;
			m_data[k] = glm::vec3(x, y, z);
			++k;
		}
	}


	in.close();
	LOG << "Loaded cloud of " << m_data.size() << " points from " << filename;
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
