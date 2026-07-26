#pragma once

#include "cstdint"
#include "map"
#include "string"


namespace serialization {
	namespace NanokaCharacter {
		struct Value {
			uint32_t rank;
			std::string en;
		};
	}// namespace NanokaCharacter

	using NanokaCharacters = std::map<uint32_t, NanokaCharacter::Value>;
}// namespace serialization
