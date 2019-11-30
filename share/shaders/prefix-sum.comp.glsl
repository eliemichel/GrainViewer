#version 450 core
#include "sys:defines"

uniform float uTime;
uniform uint uPointCount;
uniform uint uIteration;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 1) restrict readonly buffer previousElementsSsbo {
	uint previousElements[];
};
layout (std430, binding = 2) restrict writeonly buffer elementsSsbo {
	uint elements[];
};

void main() {
	uint i = gl_GlobalInvocationID.x;
	uint j = i;
	if (i >= uPointCount) return;

	uint halfSteps = uint(ceil(log((float(uPointCount))) / log(2.0)) - 1.0);

	uint lastIteration = 2 * halfSteps - 1;
	if (uIteration > lastIteration) return;

	if (uIteration == lastIteration) {
		// Offset because we want cumulative sums of elements *strictly* before the current one
		j += 1;
		elements[0] = 0;
		if (j >= uPointCount) return;
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
