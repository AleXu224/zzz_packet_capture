#pragma once

#include "cstdint"
#include "fstream"
#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "optional"
#include "string"
#include "unordered_map"
#include "vector"


namespace serialization {
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

		void populateEntriesByName() {
			entriesByName.clear();
			for (auto &entry: entries) {
				entriesByName[entry.name] = &entry;
			}
		}

		static inline Proto fromFile() {
			std::ifstream file("assets/nap.json");
			if (!file.is_open()) {
				throw std::runtime_error("Failed to open nap.json");
			}
			Proto proto;
			std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			auto ec = glz::read_json(proto.entries, buffer);
			if (ec) {
				throw std::runtime_error("Failed to deserialize nap.json: " + std::string(ec.custom_error_message));
			}

			std::sort(proto.entries.begin(), proto.entries.end(), [](const ProtoEntry &a, const ProtoEntry &b) {
				return a.cmd_id.value_or(0) < b.cmd_id.value_or(0);
			});

			for (auto &entry: proto.entries) {
				std::sort(entry.fields.begin(), entry.fields.end(), [](const ProtoEntryField &a, const ProtoEntryField &b) {
					return a.number < b.number;
				});
			}

			proto.populateEntriesByName();

			return proto;
		}

		static inline const Proto &get() {
			static Proto proto = fromFile();
			return proto;
		}
	};
}// namespace serialization