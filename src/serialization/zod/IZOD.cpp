#include "IZOD.hpp"

#include "ranges"

namespace Serialization::Zod {
	IZOD IZOD::fromPcap(const Pcap &pcap, const data::ExportSettings &exportSettings) {
		std::unordered_map<uint32_t, std::string> engineMap;
		std::unordered_map<uint32_t, std::string> discMap;
		for (const auto &agent: pcap.agents) {
			engineMap[agent.weaponUid] = util::strings::toZodKey(std::string{agent.name()});
			for (const auto &equip: agent.dressed_equips) {
				discMap[equip.uid] = util::strings::toZodKey(std::string{agent.name()});
			}
		}

		auto &nanokaData = serialization::NanokaData::get();

		return {
			.characters = pcap.agents | std::views::filter([&](const data::AgentInfo &agent) {
							  return agent.level >= exportSettings.minAgentLevel && (nanokaData.characters.at(agent.id).rank + 1) >= exportSettings.minAgentRarity;
						  })
						| std::views::transform([](const data::AgentInfo &agent) {
							  return IAgent::fromInstance(agent);
						  })
						| std::ranges::to<std::vector<IAgent>>(),
			.discs = pcap.discs | std::views::filter([&](const data::DiscInfo &disc) {
						 return disc.level >= exportSettings.minDiscLevel && disc.getRarity() >= exportSettings.minDiscRarity;
					 })
				   | std::views::transform([&](const data::DiscInfo &disc) {
						 return IDisc::fromInstance(disc, discMap);
					 })
				   | std::ranges::to<std::vector<IDisc>>(),
			.wengines = pcap.engines | std::views::filter([&](const data::WeaponInfo &weapon) {
							return weapon.level >= exportSettings.minEngineLevel && (nanokaData.weapons.at(weapon.id).rank + 1) >= exportSettings.minEngineRarity;
						})
					  | std::views::transform([&](const data::WeaponInfo &engine) {
							return IEngine::fromInstance(engine, engineMap);
						})
					  | std::ranges::to<std::vector<IEngine>>(),
		};
	};
}// namespace Serialization::Zod