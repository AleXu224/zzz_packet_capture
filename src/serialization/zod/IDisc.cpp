#include "IDisc.hpp"

#include "RarityKey.hpp"
#include "ranges"
#include "util/strings.hpp"


Serialization::Zod::IDisc Serialization::Zod::IDisc::fromInstance(const data::DiscInfo &disc, std::unordered_map<uint32_t, std::string> equipMap) {
	std::string location;
	if (auto it = equipMap.find(disc.uid); it != equipMap.end())
		location = it->second;

	return {
		.setKey{util::strings::toZodKey(std::string{disc.getSetName()})},
		.slotKey = std::format("{}", disc.getSlot()),
		.level = static_cast<uint8_t>(disc.level),
		.rarity = keyRarity.at(disc.getRarity()),
		.mainStatKey{disc.getMainStatName()},
		.location = location,
		.substats = [&]() {
			auto ret = std::vector<ISubstat>(4);

			for (auto [subStat, dataSubStat]: std::views::zip(disc.subStats, ret)) {
				dataSubStat.key = subStat.getStatName();
				dataSubStat.upgrades = subStat.add_value;
			}

			return ret;
		}(),
	};
}
