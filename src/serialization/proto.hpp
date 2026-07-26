#pragma once

#include "cstdint"
#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "optional"
#include "string"
#include "unordered_map"
#include "util/serializable.hpp"
#include "vector"


namespace serialization {
	constexpr std::string_view protoPath = "assets/nap.json";
	constexpr std::string_view protoUrl = "https://github.com/AleXu224/zzz_packet_capture/raw/refs/heads/master/assets/nap.json";

	struct ProtoEntryField {
		int32_t number = 0;
		std::string name;
		std::string type;
		std::optional<uint32_t> xor_value;
		bool is_native_type = false;
		bool is_enum = false;
		bool repeated = false;
	};

	struct ProtoEntry {
		std::optional<uint16_t> cmd_id;
		std::string name;
		std::vector<ProtoEntryField> fields;
	};

	struct Proto {
		std::vector<ProtoEntry> entries;
		std::unordered_map<std::string, ProtoEntry *> entriesByName;

		const ProtoEntry *getEntryById(uint16_t cmd_id) const {
			auto it = std::lower_bound(entries.begin(), entries.end(), cmd_id, [](const ProtoEntry &entry, uint16_t id) {
				return entry.cmd_id.value_or(0) < id;
			});
			if (it == entries.end() || it->cmd_id.value_or(0) != cmd_id) {
				return nullptr;
			}
			return &(*it);
		}

		const ProtoEntry *getEntryByName(const std::string &name) const {
			auto it = entriesByName.find(name);
			if (it != entriesByName.end()) {
				return it->second;
			}
			return nullptr;
		}

		void sortEntries() {
			std::sort(entries.begin(), entries.end(), [](const ProtoEntry &a, const ProtoEntry &b) {
				return a.cmd_id.value_or(0) < b.cmd_id.value_or(0);
			});
		}

		void sortFields() {
			for (auto &entry: entries) {
				std::sort(entry.fields.begin(), entry.fields.end(), [](const ProtoEntryField &a, const ProtoEntryField &b) {
					return a.number < b.number;
				});
			}
		}

		void populateEntriesByName() {
			entriesByName.clear();
			for (auto &entry: entries) {
				entriesByName[entry.name] = &entry;
			}
		}

		static inline Proto fromFile() {
			Proto proto;
			auto cached = util::serializable::fromFile<std::vector<ProtoEntry>>(protoPath);
			if (!cached) {
				auto latestProto = util::serializable::fromNetwork<std::vector<ProtoEntry>>(protoUrl);
				if (!latestProto) {
					throw std::runtime_error("Failed to load latest nap.json");
				}
				util::serializable::toFile(*latestProto, protoPath);
				cached = std::move(latestProto.value());
			}
			proto.entries = std::move(*cached);

			proto.sortEntries();
			proto.sortFields();
			proto.populateEntriesByName();

			return proto;
		}

		static inline const Proto &get() {
			static Proto proto = fromFile();
			return proto;
		}
	};
}// namespace serialization