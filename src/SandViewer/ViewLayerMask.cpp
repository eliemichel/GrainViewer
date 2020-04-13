#include "ViewLayerMask.h"
#include "utils/jsonutils.h"

bool ViewLayerMask::deserialize(const rapidjson::Value& json)
{
	if (!json.IsString()) return false;

	std::string maskStr = json.GetString();
	m_mask = 0;
	uint64_t bit = 1;
	for (const char& c : maskStr) {
		if (c == '1') {
			m_mask |= bit;
		}
		bit = bit << 1;
	}

	return true;
}

bool ViewLayerMask::operator&(const ViewLayerMask& other) const
{
	return (other.m_mask & m_mask) != 0;
}
