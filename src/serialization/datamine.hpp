#pragma once

#include "cstdint"
#include "fstream"
#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "string"


namespace serialization {
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

		std::string version;
		std::map<std::string, std::string> xorSeeds;
		uint32_t cmdPlayerGetTokenScRsp;
		uint32_t cmdGetEquipDataScRsp;
		uint32_t cmdGetWeaponDataScRsp;
		uint32_t cmdGetAvatarDataScRsp;

		AgentInfo agentInfo;
		AvatarSkillLevel agentSkill;
		DressedEquip agentEquip;

		DiscInfo discInfo;
		DiscStat discStat;

		WeaponInfo weaponInfo;

		static std::optional<Datamine> fromFile() {
			std::ifstream file("assets/datamine.json");
			if (!file.is_open()) {
				return std::nullopt;
			}
			std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			Datamine datamine;
			auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(datamine, data);
			if (ec) {
				std::println("Failed to parse datamine.json: {}", glz::format_error(ec, data));
				return std::nullopt;
			}
			return datamine;
		}

		static const Datamine &get() {
			static const auto datamine = fromFile();
			if (!datamine) {
				throw std::runtime_error("Failed to load datamine.json");
			}
			return *datamine;
		}
	};
}// namespace serialization