#pragma once

#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "string"
#include "util/serializable.hpp"

namespace serialization {
	constexpr std::string_view manifestPath = "assets/manifest.json";
	constexpr std::string_view manifestUrl = "https://github.com/AleXu224/zzz_packet_capture/raw/refs/heads/master/assets/manifest.json";

	struct Manifest {
		std::string version;

		static inline const Manifest &get() {
			static auto manifest = util::serializable::fromFile<Manifest>(manifestPath);
			if (!manifest) {
				manifest = util::serializable::fromNetwork<Manifest>(manifestUrl);
				if (!manifest) {
					throw std::runtime_error("Failed to load latest manifest.json");
				}
                util::serializable::toFile(*manifest, manifestPath);
			}
			return *manifest;
		}
	};
}// namespace serialization