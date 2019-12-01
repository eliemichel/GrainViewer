#version 450 core
#include "sys:defines"

#pragma variant FLOAT_ELEMENTS // default is UINT_ELEMENTS
#ifdef FLOAT_ELEMENTS
#define ELEMENT_TYPE float
#else // FLOAT_ELEMENTS
#define ELEMENT_TYPE uint
#endif // FLOAT_ELEMENTS

uniform uint uElementCount;
uniform uint uIteration;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 1) restrict readonly buffer previousElementsSsbo {
	ELEMENT_TYPE previousElements[];
};
layout (std430, binding = 2) restrict writeonly buffer elementsSsbo {
	ELEMENT_TYPE elements[];
};

void main() {
	uint i = gl_GlobalInvocationID.x;
	uint j = i;
	if (i >= uElementCount) return;

	uint halfSteps = uint(ceil(log((float(uElementCount))) / log(2.0)) - 1.0);

	uint lastIteration = 2 * halfSteps - 1;
	if (uIteration > lastIteration) return;

	if (uIteration == lastIteration) {
		// Offset because we want cumulative sums of elements *strictly* before the current one
		j += 1;
		elements[0] = 0;
		if (j >= uElementCount) return;
	}

	if (uIteration < halfSteps) {
		uint n = uint(pow(2, uIteration + 1));
		if ((i + 1) % n == 0) {
			elements[j] = previousElements[i] + previousElements[i - n/2];
		} else {
			elements[j] = previousElements[i];
		}
	} else {
		uint n0 = uint(pow(2, halfSteps));
		uint n = n0 / uint(pow(2, uIteration - halfSteps));
		uint hn = n / 2;
		if (i >= hn && (i - hn + 1) % n == 0) {
			elements[j] = previousElements[i] + previousElements[i - hn];
		} else {
			elements[j] = previousElements[i];
		}
	}
}
