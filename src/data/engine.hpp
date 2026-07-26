#pragma once

#include "../serialization/nanokaData.hpp"
#include "google/protobuf/unknown_field_set.h"
#include "serialization/datamine.hpp"
#include <cstdint>


namespace data {
	struct WeaponInfo {
		uint32_t id = 0;
		uint32_t uid = 0;
		uint32_t level = 0;
		uint32_t phase = 0;
		uint32_t modification = 0;

		[[nodiscard]] std::string_view name() const {
			return serialization::NanokaData::get().weapons.at(id).en;
		}

		static WeaponInfo fromUFS(const google::protobuf::UnknownFieldSet &ufs) {
			WeaponInfo w;
			auto &datamine = serialization::Datamine::get();
			for (int i = 0; i < ufs.field_count(); ++i) {
				const auto &f = ufs.field(i);

				auto number = f.number();
				if (number == datamine.weaponInfo.id)
					w.id = static_cast<uint32_t>(f.varint());
				else if (number == datamine.weaponInfo.uid)
					w.uid = static_cast<uint32_t>(f.varint());
				else if (number == datamine.weaponInfo.level)
					w.level = static_cast<uint32_t>(f.varint());
				else if (number == datamine.weaponInfo.phase)
					w.phase = static_cast<uint32_t>(f.varint());
				else if (number == datamine.weaponInfo.modification)
					w.modification = static_cast<uint32_t>(f.varint());
			}
			return w;
		}
	};

}// namespace data
