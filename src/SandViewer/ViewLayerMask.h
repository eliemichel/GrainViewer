#pragma once

#include <rapidjson/document.h>

class ViewLayerMask {
public:
	bool deserialize(const rapidjson::Value& json);
	uint64_t raw() const { return m_mask; }
	bool operator&(const ViewLayerMask& other) const;

private:
	uint64_t m_mask = -1; // -1 puts all bits to 1, meaning "visible on all layers"
};
