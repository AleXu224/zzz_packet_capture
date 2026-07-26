#pragma once

#include "cstdint"
#include "data/engine.hpp"
#include "string"


namespace Serialization::Zod {
	struct IEngine {
		std::string key;
		uint8_t level;
		uint8_t modification;
		uint8_t phase;
		std::string location;
		bool lock = false;
		std::string id;

		static IEngine fromInstance(const data::WeaponInfo &engine, std::unordered_map<uint32_t, std::string> equipMap);
	};
}// namespace Serialization::Zod