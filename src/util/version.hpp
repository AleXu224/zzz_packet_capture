#pragma once

#include "charconv"
#include "cstdint"
#include "format"
#include "ranges"
#include "string_view"


namespace util {
	struct Version {
		uint32_t major = 0;
		uint32_t minor = 0;
		uint32_t patch = 0;

		static inline Version parse(std::string_view versionStr) {
			Version version;
			auto parts = versionStr | std::views::split('.')
					   | std::views::transform([](auto &&part) {
							 uint32_t value = 0;
							 std::from_chars(part.data(), part.data() + part.size(), value);
							 return value;
						 });

			auto values = {&version.major, &version.minor, &version.patch};
			for (auto &&[part, valuePtr]: std::views::zip(parts, values)) {
				*valuePtr = part;
			}

			return version;
		}

		[[nodiscard]] inline std::string toString() const {
			return std::format("{}.{}.{}", major, minor, patch);
		}

		[[nodiscard]] inline bool operator==(const Version &other) const {
			return major == other.major && minor == other.minor && patch == other.patch;
		}

		[[nodiscard]] inline bool operator<(const Version &other) const {
			if (major != other.major) return major < other.major;
			if (minor != other.minor) return minor < other.minor;
			return patch < other.patch;
		}
	};
}// namespace util