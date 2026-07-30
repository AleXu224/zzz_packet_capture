#pragma once

#include "cstdint"
#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "string"
#include "util/serializable.hpp"


namespace serialization {
	constexpr std::string_view dataminePath = "assets/datamine.json";
	constexpr std::string_view datamineUrl = "https://github.com/AleXu224/zzz_packet_capture/raw/refs/heads/master/assets/datamine.json";

	struct Datamine {
		struct AvatarSkillLevel {
			uint32_t skill_type;
			uint32_t level;
		};

		struct DressedEquip {
			uint32_t uid;
			uint32_t slot;
		};

		struct AgentInfo {
			uint32_t id;
			uint32_t level;
			uint32_t promotion;
			uint32_t weaponUid;
			uint32_t core;
			uint32_t mindscape;
			uint32_t skills;
			uint32_t dressed_equips;
		};

		struct AgentData {
			uint32_t agents;
		};

		struct EquipData {
			uint32_t discs;
		};

		struct WeaponData {
			uint32_t weapons;
		};

		struct DiscStat {
			uint32_t key;
			uint32_t base_value;
			uint32_t add_value;
		};

		struct DiscInfo {
			uint32_t uid;
			uint32_t id;
			uint32_t level;

			uint32_t mainStat;
			uint32_t subStats;
		};

		struct WeaponInfo {
			uint32_t id;
			uint32_t uid;
			uint32_t level;
			uint32_t phase;
			uint32_t modification;
		};

		std::map<std::string, std::string> xorSeeds;
		uint32_t cmdPlayerGetTokenScRsp;
		uint32_t cmdGetEquipDataScRsp;
		uint32_t cmdGetWeaponDataScRsp;
		uint32_t cmdGetAvatarDataScRsp;

		AgentData agentData;
		AgentInfo agentInfo;
		AvatarSkillLevel agentSkill;
		DressedEquip agentEquip;

		EquipData equipData;
		DiscInfo discInfo;
		DiscStat discStat;

		WeaponData weaponData;
		WeaponInfo weaponInfo;

		static inline const Datamine &get() {
			static auto datamine = util::serializable::fromFile<Datamine>(dataminePath);
			if (!datamine) {
				datamine = util::serializable::fromNetwork<Datamine>(datamineUrl);
				if (!datamine) {
					throw std::runtime_error("Failed to load latest datamine.json");
				}
				util::serializable::toFile(*datamine, dataminePath);
			}
			return *datamine;
		}
	};
}// namespace serialization