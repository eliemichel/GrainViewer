#include <cstdlib>
#include <string>
#include "Logger.h"
#include "utils/strutils.h"
#include "PointCloud.h"

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

	if (!endsWith(outputFilename, ".bin")) {
		outputFilename += ".bin";
	}

	PointCloud pointCloud;

	if (endsWith(inputFilename, ".bin")) {
		pointCloud.loadBin(inputFilename);
	}
	else {
		pointCloud.loadXYZ(inputFilename);
	}
	

	if (argc >= 4 && std::string(argv[3]) == "filter") {
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
