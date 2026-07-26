#pragma once

#include "array"
#include "concepts"
#include "cstdint"
#include "ranges"


namespace util {
	template<size_t N>
	inline std::array<uint8_t, N> reversedBytes(const std::ranges::range auto &data) {
		std::array<uint8_t, N> result;
		for (const auto &[src, dst]: std::views::zip(data | std::views::take(N) | std::views::reverse, result)) {
			dst = src;
		}
		return result;
	}

	template<class T>
	inline T to(const std::ranges::range auto &data) {
		T result;
		result = *std::bit_cast<T *>(data.data());
		return result;
	}
	
	template<std::integral T>
	inline T reversedInteger(const std::ranges::range auto &data) {
		return std::byteswap(to<T>(data));
	}
}// namespace util