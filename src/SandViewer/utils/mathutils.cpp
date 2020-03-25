#include "utils/mathutils.h"

int ilog2(int x) {
	int log = 0;
	while (x >>= 1) ++log;
	return log;
}
