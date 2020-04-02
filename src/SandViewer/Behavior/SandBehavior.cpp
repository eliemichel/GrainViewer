
#include "SandBehavior.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool SandBehavior::deserialize(const rapidjson::Value & json)
{
	autoDeserialize(json, m_properties);
	return true;
}
