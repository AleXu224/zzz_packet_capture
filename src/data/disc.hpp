#pragma once

#include "../serialization/nanokaData.hpp"
#include "google/protobuf/unknown_field_set.h"
#include "serialization/datamine.hpp"
#include "statMap.hpp"
#include <cstdint>
#include <vector>


namespace data {
	struct DiscStat {
		uint32_t key = 0;
		uint32_t base_value = 0;
		uint32_t add_value = 0;

		[[nodiscard]] inline std::string_view getStatName() const {
			return data::statMap.at(key).name;
		}

		static DiscStat fromUFS(const google::protobuf::UnknownFieldSet &ufs) {
			DiscStat p;
			auto &datamine = serialization::Datamine::get();
			for (int i = 0; i < ufs.field_count(); ++i) {
				const auto &f = ufs.field(i);

				auto number = f.number();
				if (number == datamine.discStat.key)
					p.key = static_cast<uint32_t>(f.varint());
				else if (number == datamine.discStat.base_value)
					p.base_value = static_cast<uint32_t>(f.varint());
				else if (number == datamine.discStat.add_value)
					p.add_value = static_cast<uint32_t>(f.varint());
			}
			return p;
		}

		static DiscStat fromBytes(std::string_view data) {
			google::protobuf::UnknownFieldSet nested;
			if (nested.ParseFromString(data)) return fromUFS(nested);
			return {};
		}
	};

	struct DiscInfo {
		uint32_t uid = 0;
		uint32_t id = 0;
		uint32_t level = 0;

		DiscStat mainStat;
		std::vector<DiscStat> subStats;

		[[nodiscard]] uint32_t getRarity() const {
			return id / 10 % 10 + 1;
		}

		[[nodiscard]] uint32_t getSlot() const {
			return id % 10;
		}

		[[nodiscard]] uint32_t getSet() const {
			return id / 100 * 100;
		}

		[[nodiscard]] std::string_view getSetName() const {
			return serialization::NanokaData::get().equipment.at(getSet()).en.name;
		}

		[[nodiscard]] std::string_view getMainStatName() const {
			return mainStat.getStatName();
		}

		static DiscInfo fromUFS(const google::protobuf::UnknownFieldSet &ufs) {
			DiscInfo d;
			auto &datamine = serialization::Datamine::get();
			for (int i = 0; i < ufs.field_count(); ++i) {
				const auto &f = ufs.field(i);

				auto number = f.number();
				if (number == datamine.discInfo.uid)
					d.uid = static_cast<uint32_t>(f.varint());
				else if (number == datamine.discInfo.id)
					d.id = static_cast<uint32_t>(f.varint());
				else if (number == datamine.discInfo.level)
					d.level = static_cast<uint32_t>(f.varint());
				else if (number == datamine.discInfo.mainStat)
					d.mainStat = DiscStat::fromBytes(f.length_delimited());
				else if (number == datamine.discInfo.subStats)
					d.subStats.push_back(DiscStat::fromBytes(f.length_delimited()));
			}
			return d;
		}
	};

}// namespace data
