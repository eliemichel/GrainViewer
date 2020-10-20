/**
 * This file is part of GrainViewer
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

#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

/**
 * Class handling point cloud I/O
 */
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
