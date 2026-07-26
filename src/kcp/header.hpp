#pragma once

#include "../util/reversedBytes.hpp"

namespace KCP {
	static constexpr uint8_t cmdPush = 81;
	static constexpr uint8_t cmdAck = 82;
	static constexpr uint8_t cmdWasAsk = 83;
	static constexpr uint8_t cmdWasIns = 84;

	struct Header {
		static constexpr size_t size = 28;

		uint32_t conv;
		uint32_t token;
		uint8_t cmd;
		uint8_t frg;
		uint16_t wnd;
		uint32_t ts;
		uint32_t sn;
		uint32_t una;
		uint32_t len;

		static Header fromBytes(const std::span<const uint8_t> &data) {
			if (data.size() < size) {
				throw std::runtime_error("Not enough data to read KCP header");
			}
			Header header{
				.conv = util::to<uint32_t>(data | std::views::drop(0)),
				.token = util::to<uint32_t>(data | std::views::drop(4)),
				.cmd = data[8],
				.frg = data[9],
				.wnd = util::to<uint16_t>(data | std::views::drop(10)),
				.ts = util::to<uint32_t>(data | std::views::drop(12)),
				.sn = util::to<uint32_t>(data | std::views::drop(16)),
				.una = util::to<uint32_t>(data | std::views::drop(20)),
				.len = util::to<uint32_t>(data | std::views::drop(24)),
			};
			return header;
		}
	};
}// namespace KCP