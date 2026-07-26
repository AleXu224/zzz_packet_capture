#pragma once

#include <vector>


namespace serialization {
	enum class Direction {
		incoming,
		outgoing,
	};

	struct Packet {
		Direction direction = Direction::incoming;
		int64_t timestamp = 0;
		std::vector<uint8_t> data;
	};

	struct PacketList {
		std::vector<Packet> packets;
	};
}// namespace serialization