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

#include "PointCloud.h"
#include "filterPointToPointDistance.h"

#include "utils/strutils.h"
#include "Logger.h"

#include <cstdlib>
#include <string>

#define XMIN -151
#define XMAX 151
#define YMIN -1000
#define YMAX 1000
#define ZMIN -151
#define ZMAX 151

/**
 * Convert .xyz point cloud to .bin ad-hoc file for faster loading
 */
int main(int argc, char *argv[]) {
	const char *title = "Bounding Light Field -- Copyright (c) 2019 -- CG Group @ Telecom Paris";

	std::string inputFilename;
	std::string outputFilename;
	if (argc >= 3) {
		inputFilename = std::string(argv[1]);
		outputFilename = std::string(argv[2]);
	}
	else {
		ERR_LOG << "Usage: PointCloudConvert <inputFilename> <outputFilename>";
		return EXIT_FAILURE;
	}

	if (argc >= 4 && std::string(argv[3]) == "point-to-point-filter") {
		bool success = filterPointToPointDistance(inputFilename, outputFilename);
		return success ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	if (!endsWith(outputFilename, ".bin")) {
		outputFilename += ".bin";
	}

	PointCloud pointCloud;

	pointCloud.load(inputFilename);

	if (argc >= 4 && std::string(argv[3]) == "bbox-filter") {
		PointCloud filteredPointCloud;
		filteredPointCloud.data().reserve(pointCloud.data().size());
		for (const auto& p : pointCloud.data()) {
			if (p.x >= XMIN && p.x < XMAX && p.y >= YMIN && p.y < YMAX && p.z >= ZMIN && p.z < ZMAX) {
				filteredPointCloud.data().push_back(p);
			}
		}
		LOG << "Filtered point cloud down to " << filteredPointCloud.data().size() << " points";
		filteredPointCloud.saveBin(outputFilename);
	}
	else {
		pointCloud.saveBin(outputFilename);
	}

	return EXIT_SUCCESS;
}
