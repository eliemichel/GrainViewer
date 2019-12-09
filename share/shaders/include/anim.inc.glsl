
uint AnimatedPointId(uint pointId, uint frameCount, uint pointCount, float time, float fps) {
	uint pointCountPerFrame = pointCount / frameCount;
	uint frame = uint(time * fps) % max(1, frameCount);
	return pointId + pointCountPerFrame * frame;
}

