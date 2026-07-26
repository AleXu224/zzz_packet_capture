#pragma once

#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "networking.hpp"
#include "optional"
#include "string"

namespace serialization {
	struct Zzz {
		std::string live;
	};

	struct Manifest {
		Zzz zzz;

		[[nodiscard]] static inline Manifest fromBytes(std::string_view data) {
			Manifest manifest;
			auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(manifest, data);
			if (ec) {
				throw std::runtime_error("Failed to parse manifest: " + glz::format_error(ec, data));
			}
			return manifest;
		}

		[[nodiscard]] static inline std::optional<Manifest> fromNetwork() {
			auto response = squi::Networking::get("https://static.nanoka.cc/manifest.json");
			if (!response.success || response.statusCode != 200) {
				return std::nullopt;
			}
			try {
				return fromBytes(response.body);
			} catch (const std::exception &e) {
				std::println("Failed to parse manifest: {}", e.what());
				return std::nullopt;
			}
		}

		[[nodiscard]] static inline const Manifest &get() {
			static Manifest manifest = fromNetwork().value_or(Manifest{});
			return manifest;
		}
	};
}// namespace serialization
