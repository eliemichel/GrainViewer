#include "Behavior.h"
#include "RuntimeObject.h"

#define forEachBehavior for (BehaviorIterator it = beginBehaviors(), end = endBehaviors(); it != end; ++it)
#define forEachBehaviorConst for (ConstBehaviorIterator it = cbeginBehaviors(), end = cendBehaviors(); it != end; ++it)
#define b (it->second)

RuntimeObject::RuntimeObject()
{}

void RuntimeObject::start()
{
	forEachBehavior {
		b->start();
	}
}

void RuntimeObject::reloadShaders()
{
	forEachBehavior {
		if (b->isEnabled())
			b->reloadShaders();
	}
}

void RuntimeObject::update(float time, int frame)
{
	forEachBehavior {
		if (b->isEnabled())
			b->update(time, frame);
	}
}

void RuntimeObject::onPreRender(const Camera& camera, const World& world, RenderType target)
{
	forEachBehavior {
		if (b->isEnabled())
			b->onPreRender(camera, world, target);
	}
}

void RuntimeObject::render(const Camera & camera, const World & world, RenderType target) const
{
	forEachBehaviorConst {
		if (b->isEnabled())
			b->render(camera, world, target);
	}
}

void RuntimeObject::onPostRender(float time, int frame)
{
	forEachBehavior{
		if (b->isEnabled())
			b->onPostRender(time, frame);
	}
}

#undef forEachBehavior
#undef forEachBehaviorConst
#undef b
