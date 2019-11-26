#include <cstdlib>
#include <string>
#include "Logger.h"
#include "utils/strutils.h"
#include "PointCloud.h"

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

	PointCloud pointCloud;

	if (endsWith(inputFilename, ".bin")) {
		pointCloud.loadBin(inputFilename);
	}
	else {
		pointCloud.loadXYZ(inputFilename);
	}

	if (!endsWith(outputFilename, ".bin")) {
		outputFilename += ".bin";
	}
	pointCloud.saveBin(outputFilename);

	return EXIT_SUCCESS;
}
