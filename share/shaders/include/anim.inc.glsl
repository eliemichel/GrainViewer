
// depreciated
uint AnimatedPointId(uint pointId, uint frameCount, uint pointCount, float time, float fps) {
	uint pointCountPerFrame = pointCount / frameCount;
	uint frame = uint(time * fps) % max(1, frameCount);
	return pointId + pointCountPerFrame * frame;
}

// better: pointCount is the point count per frame, not the total size of the buffer
uint AnimatedPointId2(uint pointId, uint frameCount, uint pointCount, float time, float fps) {
	uint frame = uint(time * fps) % max(1, frameCount);
	return pointId + pointCount * frame;
}

