#pragma once

#include <array>
#include <cstdint>
#include <limits>

namespace crypto {
	// Directly ported from https://github.com/hashblen/auto-artifactarium/blob/master/src/cs_rand.rs
	struct Random {
	private:
		int32_t mBig{};
		int32_t mSeed{};
		int32_t inext{};
		int32_t inextp{};
		std::array<int32_t, 56> seedArray{};

	public:
		Random(int32_t seed) : mBig(std::numeric_limits<int32_t>::max()),
							   mSeed(161803398),
							   inext(0),
							   inextp(0),
							   seedArray{} {
			const int32_t subtraction = (seed == std::numeric_limits<int32_t>::min()) ? mBig : std::abs(seed);
			int32_t mj = mSeed - subtraction;
			seedArray[55] = mj;
			int32_t mk = 1;
			for (size_t i = 1; i < 55; i++) {
				auto ii = (21 * i) % 55;
				seedArray[ii] = mk;
				mk = mj - mk;
				if (mk < 0)
					mk += mBig;
				mj = seedArray[ii];
			}
			for (size_t k = 0; k < 4; k++) {
				for (size_t i = 1; i < 56; i++) {
					auto n = 1 + (i + 30) % 55;
					seedArray[i] -= seedArray[n];
					if (seedArray[i] < 0) seedArray[i] += mBig;
				}
			}
			inext = 0;
			inextp = 21;
		}

		int32_t internalSample() {
			int32_t locInext = inext + 1;
			int32_t locInextp = inextp + 1;
			int32_t ret = seedArray[locInext] - seedArray[locInextp];

			if (locInext >= 56) locInext = 1;
			if (locInextp >= 56) locInextp = 1;

			if (ret == mBig) ret -= 1;
			if (ret < 0) ret += mBig;

			seedArray[locInext] = ret;

			inext = locInext;
			inextp = locInextp;

			return ret;
		}

		double nextDouble() {
			return static_cast<double>(internalSample()) * (1.0 / static_cast<double>(mBig));
		}

		int32_t next(int32_t maxValue) {
			return static_cast<int32_t>(nextDouble() * static_cast<double>(maxValue));
		}

		int32_t next() { return internalSample(); }

		uint64_t nextSafeUint64() {
			double v = nextDouble() * static_cast<double>(std::numeric_limits<uint64_t>::max());
			return static_cast<uint64_t>(v);
		}
	};

	inline int32_t seedFromUnixSeconds(int64_t unixSeconds) {
		constexpr int64_t unixEpochOffsetSeconds = 62'135'596'800;
		int64_t totalSeconds = unixSeconds + unixEpochOffsetSeconds;
		return static_cast<int32_t>(totalSeconds & 0xFFFFFFFFULL);
	}

	inline uint64_t clientRandKey(int32_t seed) {
		Random rng(seed);
		int32_t nextV = rng.next(std::numeric_limits<int32_t>::max());
		return (static_cast<uint64_t>(static_cast<uint32_t>(nextV)) << 32) | static_cast<uint64_t>(static_cast<uint32_t>(seed));
	}
}// namespace crypto