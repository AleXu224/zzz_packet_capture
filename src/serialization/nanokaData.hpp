#pragma once

#include "../util/version.hpp"
#include "./nanoka/characters.hpp"
#include "./nanoka/equipment.hpp"
#include "./nanoka/manifest.hpp"
#include "./nanoka/weapons.hpp"
#include "filesystem"
#include "util/serializable.hpp"


namespace serialization {
	constexpr std::string_view nanokaDataPath = "assets/nanokaData.json";

	struct NanokaData {
		std::string version = "0.0.0";
		NanokaCharacters characters;
		NanokaEquipment equipment;
		NanokaWeapons weapons;


		[[nodiscard]] static inline NanokaData fromBytes(std::string_view data) {
			NanokaData nanokaData;
			auto ec = glz::read_json(nanokaData, data);
			if (ec) {
				throw std::runtime_error("Failed to parse NanokaData: " + std::to_string(static_cast<int>(ec)));
			}
			return nanokaData;
		}

		[[nodiscard]] static inline bool exists() {
			std::filesystem::path path = nanokaDataPath;
			return std::filesystem::exists(path);
		}

		[[nodiscard]] static inline std::expected<NanokaData, std::string> fetchLatest() {
			NanokaData ret;

			auto latestManifest = NanokaManifest::get();
			auto &version = latestManifest.zzz.live;
			ret.version = version;

			auto charactersLatest = squi::Networking::get(std::format("https://static.nanoka.cc/zzz/{}/character.json", version));
			if (!charactersLatest.success || charactersLatest.statusCode != 200) {
				return std::unexpected("Failed to fetch latest NanokaData: " + std::to_string(charactersLatest.statusCode));
			}
			auto charactersEc = glz::read<glz::opts{.error_on_unknown_keys = false}>(ret.characters, charactersLatest.body);
			if (charactersEc) {
				return std::unexpected("Failed to parse latest NanokaData: " + glz::format_error(charactersEc, charactersLatest.body));
			}

			auto equipmentLatest = squi::Networking::get(std::format("https://static.nanoka.cc/zzz/{}/equipment.json", version));
			if (!equipmentLatest.success || equipmentLatest.statusCode != 200) {
				return std::unexpected("Failed to fetch latest NanokaData: " + std::to_string(equipmentLatest.statusCode));
			}
			auto equipmentEc = glz::read<glz::opts{.error_on_unknown_keys = false}>(ret.equipment, equipmentLatest.body);
			if (equipmentEc) {
				return std::unexpected("Failed to parse latest NanokaData: " + glz::format_error(equipmentEc, equipmentLatest.body));
			}

			auto weaponsLatest = squi::Networking::get(std::format("https://static.nanoka.cc/zzz/{}/weapon.json", version));
			if (!weaponsLatest.success || weaponsLatest.statusCode != 200) {
				return std::unexpected("Failed to fetch latest NanokaData: " + std::to_string(weaponsLatest.statusCode));
			}
			auto weaponsEc = glz::read<glz::opts{.error_on_unknown_keys = false}>(ret.weapons, weaponsLatest.body);
			if (weaponsEc) {
				return std::unexpected("Failed to parse latest NanokaData: " + glz::format_error(weaponsEc, weaponsLatest.body));
			}

			return ret;
		}

		[[nodiscard]] inline bool shouldUpdate() const {
			auto latestManifest = NanokaManifest::get();
			if (latestManifest.zzz.live.empty()) {
				std::println("Failed to fetch latest manifest for NanokaData update check.");
				return false;
			}

			auto liveVersion = util::Version::parse(latestManifest.zzz.live);
			auto currentVersion = util::Version::parse(version);

			return currentVersion < liveVersion;
		}

		static inline const NanokaData &get() {
			static NanokaData nanokaData = [&]() {
				auto cached = util::serializable::fromFile<NanokaData>(nanokaDataPath);
				if (!cached || cached->shouldUpdate()) {
					auto latest = fetchLatest();
					if (!latest.has_value()) {
						std::println("{}", latest.error());
						if (!cached) {
							throw std::runtime_error("No cached NanokaData available.");
						}
						return *cached;
					}
					util::serializable::toFile(*latest, nanokaDataPath);
					cached = latest.value();
				}
				return *cached;
			}();
			return nanokaData;
		}
	};
}// namespace serialization