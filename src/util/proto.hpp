#pragma once

#include "debug.hpp"
#include "google/protobuf/unknown_field_set.h"
#include "utf8cpp/utf8.h"// IWYU pragma: keep

namespace util {
	static inline void printUFS(const google::protobuf::UnknownFieldSet &ufs, int indent = 0) {
		std::string pad(indent, ' ');
		for (int i = 0; i < ufs.field_count(); ++i) {
			const google::protobuf::UnknownField &f = ufs.field(i);
			int num = f.number();
			switch (f.type()) {
				case google::protobuf::UnknownField::TYPE_VARINT:
					debug::log("{}{}: {}", pad, num, f.varint());
					break;
				case google::protobuf::UnknownField::TYPE_FIXED32:
					debug::log("{}{}: 0x{:08X}", pad, num, f.fixed32());
					break;
				case google::protobuf::UnknownField::TYPE_FIXED64:
					debug::log("{}{}: 0x{:016X}", pad, num, f.fixed64());
					break;
				case google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED: {
					auto s = f.length_delimited();
					google::protobuf::UnknownFieldSet nested;
					if (nested.ParseFromString(s)) {
						debug::log("{}{} {{", pad, num);
						printUFS(nested, indent + 2);
						debug::log("{}}}", pad);
					} else {
						if (utf8::is_valid(s)) {
							debug::log("{}{}: \"{}\"", pad, num, s);
						} else {
							debug::print("{}{}: ", pad, num, s.size());
							for (const auto &b: s) {
								debug::print("{:02X}", static_cast<uint8_t>(b));
							}
							debug::log();
						}
					}
					break;
				}
				case google::protobuf::UnknownField::TYPE_GROUP:
					debug::log("{}{} {{", pad, num);
					printUFS(f.group(), indent + 2);
					debug::log("{}}}", pad);
					break;
			}
		}
	}
}// namespace util