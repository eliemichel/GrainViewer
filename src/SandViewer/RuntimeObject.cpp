#include "Behavior.h"
#include "RuntimeObject.h"

#define forEachBehavior for (BehaviorIterator it = beginBehaviors(), end = endBehaviors(); it != end; ++it)
#define forEachBehaviorConst for (ConstBehaviorIterator it = cbeginBehaviors(), end = cendBehaviors(); it != end; ++it)

RuntimeObject::RuntimeObject()
{}

void RuntimeObject::start()
{
	forEachBehavior {
		it->second->start();
	}
}

void RuntimeObject::reloadShaders()
{
	forEachBehavior {
		if (it->second->isEnabled())
			it->second->reloadShaders();
	}
}

void RuntimeObject::update(float time, int frame)
{
	forEachBehavior {
		if (it->second->isEnabled())
			it->second->update(time, frame);
	}
}

void RuntimeObject::render(const Camera & camera, const World & world, RenderType target) const
{
	forEachBehaviorConst {
		if (it->second->isEnabled())
			it->second->render(camera, world, target);
	}
}

void RuntimeObject::onPostRender(float time, int frame)
{
	forEachBehavior{
		if (it->second->isEnabled())
		it->second->onPostRender(time, frame);
	}
}

#undef forEachBehavior
#undef forEachBehaviorConst
