#pragma once

#include "google/protobuf/unknown_field_set.h"
#include "utf8cpp/utf8.h"// IWYU pragma: keep
#include <print>

namespace util {
	static inline void printUFS(const google::protobuf::UnknownFieldSet &ufs, int indent = 0) {
		std::string pad(indent, ' ');
		for (int i = 0; i < ufs.field_count(); ++i) {
			const google::protobuf::UnknownField &f = ufs.field(i);
			int num = f.number();
			switch (f.type()) {
				case google::protobuf::UnknownField::TYPE_VARINT:
					std::println("{}{}: {}", pad, num, f.varint());
					break;
				case google::protobuf::UnknownField::TYPE_FIXED32:
					std::println("{}{}: 0x{:08X}", pad, num, f.fixed32());
					break;
				case google::protobuf::UnknownField::TYPE_FIXED64:
					std::println("{}{}: 0x{:016X}", pad, num, f.fixed64());
					break;
				case google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED: {
					auto s = f.length_delimited();
					google::protobuf::UnknownFieldSet nested;
					if (nested.ParseFromString(s)) {
						std::println("{}{} {{", pad, num);
						printUFS(nested, indent + 2);
						std::println("{}}}", pad);
					} else {
						if (utf8::is_valid(s)) {
							std::println("{}{}: \"{}\"", pad, num, s);
						} else {
							std::print("{}{}: ", pad, num, s.size());
							for (const auto &b: s) {
								std::print("{:02X}", static_cast<uint8_t>(b));
							}
							std::println();
						}
					}
					break;
				}
				case google::protobuf::UnknownField::TYPE_GROUP:
					std::println("{}{} {{", pad, num);
					printUFS(f.group(), indent + 2);
					std::println("{}}}", pad);
					break;
			}
		}
	}
}// namespace util