#pragma once

#include "../serialization/proto.hpp"
#include "google/protobuf/unknown_field_set.h"


namespace crypto {
	inline void xorProtoFields(google::protobuf::UnknownFieldSet &ufs, const serialization::ProtoEntry *entry) {
		if (!entry) return;

		for (int i = 0; i < ufs.field_count(); ++i) {
			auto f = ufs.mutable_field(i);
			for (const auto &field: entry->fields) {
				if (f->number() != field.number) continue;

				if (!field.is_native_type && !field.is_enum) {
					google::protobuf::UnknownFieldSet nested;
					if (nested.ParseFromString(f->length_delimited())) {
						xorProtoFields(nested, serialization::Proto::get().getEntryByName(field.type));
						std::string nested_serialized;
						nested.SerializeToString(&nested_serialized);
						f->set_length_delimited(nested_serialized);
					}
					break;
				}
				if (!field.xor_value.has_value()) break;
				switch (f->type()) {
					case google::protobuf::UnknownField::TYPE_VARINT:
						f->set_varint(f->varint() ^ field.xor_value.value());
						break;
					case google::protobuf::UnknownField::TYPE_FIXED32:
						f->set_fixed32(f->fixed32() ^ field.xor_value.value());
						break;
					case google::protobuf::UnknownField::TYPE_FIXED64:
						f->set_fixed64(f->fixed64() ^ field.xor_value.value());
						break;
					default:
						break;
				}
			}
		}
	}
}// namespace crypto