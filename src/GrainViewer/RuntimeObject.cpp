/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "Behavior.h"
#include "RuntimeObject.h"
#include "utils/jsonutils.h"

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

bool RuntimeObject::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "name", name, name);
	jrOption(json, "viewLayers", viewLayers, viewLayers);
	return true;
}
