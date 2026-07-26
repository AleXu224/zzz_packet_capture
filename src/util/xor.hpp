#pragma once

#include "cstdint"
#include "span"

namespace util {
	inline std::vector<uint8_t> xorData(std::ranges::range auto &data, const std::span<const uint8_t> &xorPad) {
        std::vector<uint8_t> dataVec(data.begin(), data.end());
        
		for (size_t i = 0; i < dataVec.size(); ++i) {
			dataVec[i] ^= xorPad[i % xorPad.size()];
		}
		return dataVec;
	}
}// namespace util