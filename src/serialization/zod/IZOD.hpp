#pragma once

#include "IAgent.hpp"
#include "IDisc.hpp"
#include "IEngine.hpp"
#include "cstdint"
#include "data/exportSettings.hpp"
#include "optional"
#include "pcap/pcap.hpp"
#include "string"
#include "vector"


namespace Serialization::Zod {
	struct IZOD {
		std::string format = "ZOD";
		uint8_t version = 1;
		std::string source = "ZZZ Packet Capture";
		std::optional<std::vector<IAgent>> characters = std::nullopt;
		std::optional<std::vector<IDisc>> discs = std::nullopt;
		std::optional<std::vector<IEngine>> wengines = std::nullopt;

		static IZOD fromPcap(const Pcap &pcap, const data::ExportSettings &exportSettings);
	};
}// namespace Serialization::Zod