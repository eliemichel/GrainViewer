#pragma once

#include <string>

/**
 * Environement variables are used for substitution of patterns like $BASEFILE in input strings
 */
struct EnvironmentVariables {
	std::string baseFile;
};
