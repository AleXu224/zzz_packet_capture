#pragma once

#include "../../data/agent.hpp"
#include "cstdint"
#include <string>


namespace Serialization::Zod {
	struct IAgent {
		std::string equippedEngine = ""; // Zenless optimizer bugs out and sets all skills to lvl 1 if you don't have this
		
		std::string key;
		uint8_t level;
		uint8_t mindscape;
		uint8_t promotion;
		
		uint8_t core;
		uint8_t dodge;
		uint8_t basic;
		uint8_t chain;
		uint8_t special;
		uint8_t assist;
		uint8_t potential;

		std::string id = key;
		
		static IAgent fromInstance(const data::AgentInfo &agent);
	};
}// namespace Serialization::Zod
