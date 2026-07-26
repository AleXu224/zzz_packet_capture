#pragma once
#include "../util/reversedBytes.hpp"
#include "ec2b_tables.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>

namespace crypto::ec2b {
	constexpr uint32_t magic = 0x45633262;// "Ec2b"
	constexpr size_t keySize = 16;
	constexpr size_t seedSize = 2048;
	constexpr uint64_t finalXOR = 0xCEAC3B5A867837ACULL;

	inline std::array<uint8_t, 16> unscramble_aes_key(const std::array<uint8_t, 16> &key) {
		using namespace ec2b_tables;

		std::array<uint8_t, 16> chip = key;

		for (int i = 0; i < 16; i++) chip[i] ^= roundKeys[i];

		for (int rnd = 1; rnd < 10; rnd++) {
			for (int i = 0; i < 16; i++) chip[i] = sboxInv[chip[i]];
			auto temp = chip;
			for (int i = 0; i < 16; i++) chip[i] = temp[shiftRowsInv[i]];
			for (int col = 0; col < 16; col += 4) {
				auto a0 = chip[col], a1 = chip[col + 1], a2 = chip[col + 2], a3 = chip[col + 3];
				chip[col] = G14[a0] ^ G9[a3] ^ G13[a2] ^ G11[a1];
				chip[col + 1] = G14[a1] ^ G9[a0] ^ G13[a3] ^ G11[a2];
				chip[col + 2] = G14[a2] ^ G9[a1] ^ G13[a0] ^ G11[a3];
				chip[col + 3] = G14[a3] ^ G9[a2] ^ G13[a1] ^ G11[a0];
			}
			for (int i = 0; i < 16; i++) chip[i] ^= roundKeys[rnd * 16 + i];
		}

		for (int i = 0; i < 16; i++) chip[i] = sboxInv[chip[i]];
		{
			auto temp = chip;
			for (int i = 0; i < 16; i++) chip[i] = temp[shiftRowsInv[i]];
		}
		for (int i = 0; i < 16; i++) chip[i] ^= roundKeys[160 + i];

		return chip;
	}

	struct Parsed {
		std::array<uint8_t, 16> key;
		std::array<uint8_t, seedSize> seed;
	};

	inline Parsed parse(std::span<const uint8_t> data) {
		if (data.size() < 28) throw std::runtime_error("Ec2b: too short");

		if (util::reversedInteger<uint32_t>(data) != magic) throw std::runtime_error("Ec2b: bad magic");

		uint32_t key_sz = util::to<uint32_t>(data.subspan(4));
		if (key_sz != keySize) throw std::runtime_error("Ec2b: bad key size");

		size_t ps_off = 8 + key_sz;
		if (ps_off + 4 > data.size()) throw std::runtime_error("Ec2b: key OOB");
		uint32_t payload_sz = util::to<uint32_t>(data.subspan(ps_off));
		if (payload_sz != seedSize) throw std::runtime_error("Ec2b: bad payload size");

		size_t data_off = ps_off + 4;
		if (data_off + payload_sz > data.size()) throw std::runtime_error("Ec2b: payload OOB");

		std::array<uint8_t, 16> scrambledKey{};
		std::memcpy(scrambledKey.data(), data.data() + 8, keySize);

		std::array<uint8_t, seedSize> scramblerSeed{};
		std::memcpy(scramblerSeed.data(), data.data() + data_off, seedSize);

		auto key = unscramble_aes_key(scrambledKey);
		for (int i = 0; i < 16; i++) key[i] ^= ec2b_tables::keyXORPadTable[i];

		return {key, scramblerSeed};
	}

	inline uint64_t scramble(const Parsed &p) {
		uint64_t val = 0xFFFFFFFFFFFFFFFFULL;
		for (size_t i = 0; i < seedSize; i += 8) {
			val ^= util::to<uint64_t>(p.seed | std::views::drop(i));
		}
		uint64_t k0 = util::to<uint64_t>(p.key);
		uint64_t k1 = util::to<uint64_t>(p.key | std::views::drop(8));
		return val ^ k0 ^ k1 ^ finalXOR;
	}

	inline uint64_t derive_seed(std::span<const uint8_t> ec2b_data) {
		auto parsed = parse(ec2b_data);
		return scramble(parsed);
	}
}// namespace crypto::ec2b