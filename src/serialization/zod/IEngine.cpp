#include "IEngine.hpp"

#include "util/strings.hpp"

Serialization::Zod::IEngine Serialization::Zod::IEngine::fromInstance(const data::WeaponInfo &engine, std::unordered_map<uint32_t, std::string> equipMap) {
	std::string location;
	if (auto it = equipMap.find(engine.uid); it != equipMap.end())
		location = it->second;
	
	return Serialization::Zod::IEngine{
		.key = util::strings::toZodKey(std::string{engine.name()}),
		.level = static_cast<uint8_t>(engine.level),
		.modification = static_cast<uint8_t>(engine.modification),
		.phase = static_cast<uint8_t>(engine.phase),
		.location = location,
		.id = std::format("zzz_wengine_{}", engine.uid),
	};
}
