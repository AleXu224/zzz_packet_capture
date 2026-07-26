#pragma once

#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "optional"
#include "string"
#include "util/serializable.hpp"

namespace serialization {
	struct Zzz {
		std::string live;
	};

	struct NanokaManifest {
		Zzz zzz;

		[[nodiscard]] static inline const NanokaManifest &get() {
			static auto manifest = util::serializable::fromNetwork<NanokaManifest>("https://static.nanoka.cc/manifest.json").value_or(NanokaManifest{});
			return manifest;
		}
	};
}// namespace serialization
