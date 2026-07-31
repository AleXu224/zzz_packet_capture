#pragma once

namespace data {
	struct ExportSettings {
		uint8_t minDiscRarity = 3;
		uint8_t minDiscLevel = 0;
		uint8_t minEngineRarity = 3;
		uint8_t minEngineLevel = 0;
		uint8_t minAgentRarity = 4;
		uint8_t minAgentLevel = 0;
		bool exportDiscs = true;
		bool exportAgents = true;
		bool exportEngines = true;
	};
}// namespace data