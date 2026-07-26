#pragma once

#include "../util/reversedBytes.hpp"
#include "cstdint"


namespace KCP {
	struct MessageHeader {
		static constexpr size_t size = 12;

		std::array<uint8_t, 4> magicBytes;
		uint16_t commandId;
		uint16_t headLength;
		uint32_t bodyLength;

		static MessageHeader fromBytes(const std::span<const uint8_t> &data) {
			if (data.size() < size) {
				throw std::runtime_error("Not enough data to read KCP message header");
			}
			MessageHeader header{
				.magicBytes = {data[0], data[1], data[2], data[3]},
				.commandId = util::reversedInteger<uint16_t>(data | std::views::drop(4)),
				.headLength = util::reversedInteger<uint16_t>(data | std::views::drop(6)),
				.bodyLength = util::reversedInteger<uint32_t>(data | std::views::drop(8)),
			};
			return header;
		}
	};
}// namespace KCP