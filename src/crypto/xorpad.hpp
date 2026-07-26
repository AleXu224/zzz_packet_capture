#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <random>


namespace crypto::xorpad {
	constexpr size_t size = 4096;

	inline std::optional<uint64_t> initialSeed{};

	inline std::array<uint8_t, size> generate(uint64_t seed, bool bigEndian) {
		std::array<uint8_t, size> pad{};
		std::mt19937_64 mt(seed);
		for (size_t i = 0; i < 512; i++) {
			uint64_t val = mt();
			auto vals = std::bit_cast<std::array<uint8_t, 8>>(val);
			if (bigEndian)
				std::reverse(vals.begin(), vals.end());
			memcpy(&pad[i * 8], vals.data(), 8);
		}
		return pad;
	}

	inline const std::array<uint8_t, size> &initial() {
		if (!initialSeed) {
			throw std::runtime_error("Initial seed is not set");
		}
		static auto pad = generate(*initialSeed, false);
		static uint64_t lastSeed = *initialSeed;
		if (lastSeed != *initialSeed) {
			pad = generate(*initialSeed, false);
			lastSeed = *initialSeed;
		}
		return pad;
	}

	inline std::array<uint8_t, size> session(uint64_t sessionKey) {
		return generate(sessionKey, true);
	}
}// namespace crypto::xorpad
