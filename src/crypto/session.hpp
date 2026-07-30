#pragma once

#include "../crypto/crandom.hpp"
#include "../crypto/rsa.hpp"
#include "../crypto/xorpad.hpp"
#include "../util/reversedBytes.hpp"
#include "../util/strings.hpp"
#include "google/protobuf/unknown_field_set.h"
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace crypto {
	struct Session {
		std::optional<uint64_t> serverRandKey;
		std::optional<uint64_t> clientRandKey;
		std::optional<uint64_t> sessionKey;
		std::array<uint8_t, crypto::xorpad::size> sessionPad{};
		bool sessionPadReady = false;

		std::vector<uint8_t> decryptBody(auto bodyBytes) {
			const auto &pad = sessionPadReady ? sessionPad : crypto::xorpad::initial();
			std::vector<uint8_t> out(bodyBytes.begin(), bodyBytes.end());
			for (size_t i = 0; i < out.size(); ++i) out[i] ^= pad[i % pad.size()];
			return out;
		}

		inline uint64_t extractServerRandKey(std::span<const uint8_t> bodyAfterInitialXor) {
			google::protobuf::UnknownFieldSet ufs;
			auto success = ufs.ParseFromArray(bodyAfterInitialXor.data(), static_cast<int>(bodyAfterInitialXor.size()));
			if (!success) throw std::runtime_error("session: failed to parse PlayerGetTokenScRsp body as protobuf");

			for (size_t i = 0; i < ufs.field_count(); ++i) {
				const auto &field = ufs.field(i);

				if (field.type() != google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED) continue;
				auto fieldBytes = field.length_delimited();
				auto fieldDecoded = util::strings::decodeB64(std::string{reinterpret_cast<const char *>(fieldBytes.data()), fieldBytes.size()});
				if (fieldDecoded.size() != crypto::rsa::KeySize) continue;

				std::vector<uint8_t> plain;
				auto res = crypto::rsa::decryptBlock(std::span<const uint8_t>{fieldDecoded});
				if (!res) {
					std::println("session: failed to decrypt server_rand_key field: {}", res.error());
					continue;
				}
				plain = std::move(*res);
				if (plain.size() != 8) continue;

				auto key = util::to<uint64_t>(plain);
				return key;
			}

			throw std::runtime_error("session: server_rand_key field not found in PlayerGetTokenScRsp");
		}

		inline std::optional<std::pair<uint64_t, uint64_t>> bruteForceClientRandKey(
			uint64_t serverRandKey,
			std::span<const uint8_t> targetBody,
			int64_t unixSeconds,
			int64_t windowSeconds = 5
		) {
			for (int64_t delta = -windowSeconds; delta <= windowSeconds; ++delta) {
				int32_t seed = seedFromUnixSeconds(unixSeconds + delta);
				crypto::Random rng(seed);

				int32_t nextV = rng.next(std::numeric_limits<int32_t>::max());
				uint64_t crk = (static_cast<uint64_t>(static_cast<uint32_t>(nextV)) << 32) | static_cast<uint64_t>(static_cast<uint32_t>(seed));
				uint64_t sessionKey = serverRandKey ^ crk;

				auto pad = crypto::xorpad::session(sessionKey);
				std::vector<uint8_t> dec;
				dec.reserve(targetBody.size());
				for (size_t i = 0; i < targetBody.size(); ++i) {
					dec.push_back(targetBody[i] ^ pad[i % pad.size()]);
				}

				google::protobuf::UnknownFieldSet fields;
				auto success = fields.ParseFromArray(dec.data(), static_cast<int>(dec.size()));
				// If the protobuf parses successfully it is almost certainly the correct key
				if (success) {
					std::println("Found at delta={}", delta);
					return std::make_pair(crk, sessionKey);
				}
			}
			return std::nullopt;
		}

		inline void deriveSessionKey(std::span<const uint8_t> messageBody, int64_t unixSeconds) {
			auto result = bruteForceClientRandKey(*serverRandKey, messageBody, unixSeconds);
			if (!result) {
				std::println("session: brute force failed (no matching seed in window)");
				return;
			}
			clientRandKey = result->first;
			sessionKey = result->second;
			sessionPad = crypto::xorpad::session(*sessionKey);
			sessionPadReady = true;
			std::println("session: derived client_rand_key={:016X} session_key={:016X}", *clientRandKey, *sessionKey);
		}
	};
}// namespace crypto