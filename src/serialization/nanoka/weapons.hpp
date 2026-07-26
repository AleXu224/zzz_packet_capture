#pragma once

#include "cstdint"
#include "map"
#include "string"

namespace serialization {
	struct NanokaWeaponsValue {
		uint32_t rank;
		std::string en;
	};

	using NanokaWeapons = std::map<uint32_t, NanokaWeaponsValue>;
}// namespace serialization
