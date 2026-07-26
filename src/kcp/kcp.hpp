#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <span>
#include <vector>

#include "../serialization/packets.hpp"
#include "./header.hpp"


namespace KCP {
	struct KCP {
		struct Segment {
			uint32_t sn = 0;
			uint8_t frg = 0;
			std::vector<uint8_t> data;
		};

		struct Stream {
			std::map<uint32_t, Segment> rcvBuf;
			std::deque<Segment> rcvQueue;
			uint32_t rcvNxt = 0;
			bool rcvNxtKnown = false;
		};

		Stream incomingStream;
		Stream outgoingStream;

		static constexpr size_t maxRcvBuf = 1024;

		std::vector<std::vector<uint8_t>> receive(std::span<const uint8_t> data, serialization::Direction direction) {
			std::vector<std::vector<uint8_t>> messages;
			auto &stream = direction == serialization::Direction::outgoing ? outgoingStream : incomingStream;

			size_t offset = 0;
			while (data.size() - offset >= Header::size) {
				Header header = Header::fromBytes(data.subspan(offset, Header::size));

				size_t remaining = data.size() - offset - Header::size;
				size_t payloadLen = std::min<size_t>(header.len, remaining);

				if (header.len > remaining) break;

				if (header.cmd == cmdPush && header.len > 0) {
					Segment seg{
						.sn = header.sn,
						.frg = header.frg,
						.data = std::vector<uint8_t>(
							data.begin() + offset + Header::size,
							data.begin() + offset + Header::size + payloadLen
						),
					};

					if (!stream.rcvNxtKnown) {
						stream.rcvNxt = header.sn;
						stream.rcvNxtKnown = true;
					}

					int32_t diff = static_cast<int32_t>(header.sn - stream.rcvNxt);
					if (diff >= 0 && !stream.rcvBuf.contains(header.sn)) {
						stream.rcvBuf.emplace(header.sn, std::move(seg));
					}

					promote(stream);

					if (stream.rcvBuf.size() > maxRcvBuf) {
						stream.rcvNxt = stream.rcvBuf.begin()->first;
						promote(stream);
					}
				}

				offset += Header::size + payloadLen;
			}

			while (!stream.rcvQueue.empty()) {
				uint32_t expectedCount = static_cast<uint32_t>(stream.rcvQueue.front().frg) + 1;
				if (stream.rcvQueue.size() < expectedCount) break;

				bool complete = true;
				for (uint32_t i = 0; i < expectedCount; ++i) {
					if (stream.rcvQueue[i].frg != static_cast<uint8_t>(expectedCount - 1 - i)) {
						complete = false;
						break;
					}
				}

				if (!complete) {
					stream.rcvQueue.pop_front();
					continue;
				}

				std::vector<uint8_t> payload;
				for (uint32_t i = 0; i < expectedCount; ++i) {
					const auto &seg = stream.rcvQueue[i];
					payload.insert(payload.end(), seg.data.begin(), seg.data.end());
				}
				for (uint32_t i = 0; i < expectedCount; ++i) {
					stream.rcvQueue.pop_front();
				}

				messages.push_back(std::move(payload));
			}

			return messages;
		}

		static void promote(Stream &stream) {
			while (true) {
				auto it = stream.rcvBuf.find(stream.rcvNxt);
				if (it == stream.rcvBuf.end()) break;
				stream.rcvQueue.push_back(std::move(it->second));
				stream.rcvBuf.erase(it);
				stream.rcvNxt++;
			}
		}
	};
};// namespace KCP