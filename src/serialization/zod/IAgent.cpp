#include "IAgent.hpp"

#include "../../util/strings.hpp"

Serialization::Zod::IAgent Serialization::Zod::IAgent::fromInstance(const data::AgentInfo &agent) {
	return {
		.key = util::strings::toZodKey(std::string{agent.name()}),
		.level = static_cast<uint8_t>(agent.level),
		.mindscape = static_cast<uint8_t>(agent.mindscape),
		.promotion = static_cast<uint8_t>(agent.promotion - 1),
		.core = static_cast<uint8_t>(agent.skills.at(4).level - 1),
		.dodge = static_cast<uint8_t>(agent.skills.at(2).level),
		.basic = static_cast<uint8_t>(agent.skills.at(0).level),
		.chain = static_cast<uint8_t>(agent.skills.at(3).level),
		.special = static_cast<uint8_t>(agent.skills.at(1).level),
		.assist = static_cast<uint8_t>(agent.skills.at(5).level),
		.potential = 0,
	};
}