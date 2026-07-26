#pragma once

#include "../serialization/nanokaData.hpp"
#include "google/protobuf/unknown_field_set.h"
#include "serialization/datamine.hpp"
#include <cstdint>
#include <vector>


namespace data {
	struct AvatarSkillLevel {
		uint32_t skill_type = 0;
		uint32_t level = 0;

		static AvatarSkillLevel fromUFS(const google::protobuf::UnknownFieldSet &ufs) {
			AvatarSkillLevel a;
			auto &datamine = serialization::Datamine::get();
			for (int i = 0; i < ufs.field_count(); ++i) {
				const auto &f = ufs.field(i);

				auto number = f.number();
				if (number == datamine.agentSkill.skill_type)
					a.skill_type = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentSkill.level)
					a.level = static_cast<uint32_t>(f.varint());
			}
			return a;
		}
	};

	struct DressedEquip {
		uint32_t uid = 0;
		uint32_t slot = 0;

		static DressedEquip fromUFS(const google::protobuf::UnknownFieldSet &ufs) {
			DressedEquip d;
			auto &datamine = serialization::Datamine::get();
			for (int i = 0; i < ufs.field_count(); ++i) {
				const auto &f = ufs.field(i);

				auto number = f.number();
				if (number == datamine.agentEquip.uid)
					d.uid = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentEquip.slot)
					d.slot = static_cast<uint32_t>(f.varint());
			}
			return d;
		}
	};

	struct AgentInfo {
		uint32_t id = 0;
		uint32_t level = 0;
		uint32_t promotion = 0;
		uint32_t weaponUid = 0;
		uint32_t mindscape = 0;

		std::vector<AvatarSkillLevel> skills;
		std::vector<DressedEquip> dressed_equips;

		inline std::string_view name() const {
			return serialization::NanokaData::get().characters.at(id).en;
		}

		static AgentInfo fromUFS(const google::protobuf::UnknownFieldSet &ufs) {
			AgentInfo a;
			auto &datamine = serialization::Datamine::get();
			for (int i = 0; i < ufs.field_count(); ++i) {
				const auto &f = ufs.field(i);

				auto number = f.number();
				if (number == datamine.agentInfo.level)
					a.level = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentInfo.weaponUid)
					a.weaponUid = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentInfo.mindscape)
					a.mindscape = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentInfo.id)
					a.id = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentInfo.promotion)
					a.promotion = static_cast<uint32_t>(f.varint());
				else if (number == datamine.agentInfo.skills) {
					if (f.type() == google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED) {
						google::protobuf::UnknownFieldSet nested;
						if (nested.ParseFromString(f.length_delimited()))
							a.skills.push_back(AvatarSkillLevel::fromUFS(nested));
					}
				} else if (number == datamine.agentInfo.dressed_equips) {
					if (f.type() == google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED) {
						google::protobuf::UnknownFieldSet nested;
						if (nested.ParseFromString(f.length_delimited()))
							a.dressed_equips.push_back(DressedEquip::fromUFS(nested));
					}
				}
			}
			return a;
		}
	};

}// namespace data
