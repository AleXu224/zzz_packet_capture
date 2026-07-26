#pragma once

#include "string"

namespace Serialization::Zod {
	struct ISubstat {
		std::string key;
		uint32_t upgrades;
	};
}// namespace Serialization::Zod