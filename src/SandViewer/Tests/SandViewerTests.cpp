#include <cstdlib>

#include "testHzb.h"
#include "testSplineBlur.h"
#include "testPointcloudDistance.h"

int main(int argc, char** argv) {
	bool success = true;
	//success = success && testHzb();
	//success = success && testSplineBlur();
	success = success && testPointcloudDistance();
	return EXIT_SUCCESS;
}
