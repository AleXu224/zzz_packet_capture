#pragma once

#include "cstdint"
#include "map"
#include "string"


namespace serialization {
	struct EquipmentValue {
		struct En {
			std::string name;
		};
		En en;
	};

	using NanokaEquipment = std::map<uint32_t, EquipmentValue>;
}// namespace serialization
