#pragma once

#include "string"
#include "unordered_map"


namespace Serialization::Zod {
	static inline std::unordered_map<std::string, uint32_t> rarityKey{
		{"B", 3},
		{"A", 4},
		{"S", 5},
	};
	static inline std::unordered_map<uint32_t, std::string> keyRarity{
		{3, "B"},
		{4, "A"},
		{5, "S"},
	};
}// namespace Serialization::Zod