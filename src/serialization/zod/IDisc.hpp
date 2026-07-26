#pragma once

#include "ISubstat.hpp"
#include "cstdint"
#include "data/disc.hpp"
#include "string"
#include "unordered_map"
#include "vector"


namespace Serialization::Zod {
	struct IDisc {
		std::string setKey;
		std::string slotKey;
		uint8_t level;
		std::string rarity;
		std::string mainStatKey;
		std::string location;
		bool lock = false;
		bool trash = false;
		std::vector<ISubstat> substats;

		static IDisc fromInstance(const data::DiscInfo &disc, std::unordered_map<uint32_t, std::string> equipMap);
	};
}// namespace Serialization::Zod