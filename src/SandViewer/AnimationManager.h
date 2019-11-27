#pragma once

#include <vector>
#include <functional>

/**
 * Animations are callbacks called every frame
 */
class AnimationManager {
public:
	void addAnimation(std::function<void(float, int)> && anim);
	void clear();
	void update(float time, int frame);

private:
	std::vector<std::function<void(float,int)>> m_anims;
};
