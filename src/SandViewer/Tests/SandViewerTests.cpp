#include <cstdlib>

#include "testHzb.h"
#include "testSplineBlur.h"

int main(int argc, char** argv) {
	bool success = true;
	success = success && testHzb();
	//success = success && testSplineBlur();
	return EXIT_SUCCESS;
}
