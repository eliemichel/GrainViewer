#include "AnimationManager.h"

void AnimationManager::addAnimation(std::function<void(float, int)> && anim)
{
	m_anims.push_back(std::move(anim));
}

void AnimationManager::clear() {
	m_anims.clear();
}

void AnimationManager::update(float time, int frame)
{
	for (const auto & ch : m_anims) {
		ch(time, frame);
	}
}
