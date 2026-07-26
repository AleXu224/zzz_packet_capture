#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>


namespace util::strings {
	constexpr uint8_t b64Val(char c) {
		if (c >= 'A' && c <= 'Z') return static_cast<uint8_t>(c - 'A');
		if (c >= 'a' && c <= 'z') return static_cast<uint8_t>(c - 'a' + 26);
		if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0' + 52);
		if (c == '+') return 62;
		if (c == '/') return 63;
		return 0;
	}

	constexpr std::string_view extractXmlTag(std::string_view xml, std::string_view open_tag, std::string_view close_tag) {
		auto start = xml.find(open_tag);
		if (start == std::string_view::npos) throw "open tag not found";
		start += open_tag.size();
		auto end = xml.find(close_tag, start);
		if (end == std::string_view::npos) throw "close tag not found";
		return xml.substr(start, end - start);
	}

	template<size_t N>
	consteval std::array<uint8_t, N> decodeB64Fixed(std::string_view s) {
		std::array<uint8_t, N> out{};
		size_t out_idx = 0;
		uint32_t buf = 0;
		int bits = 0;
		for (char c: s) {
			if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
			buf = (buf << 6) | b64Val(c);
			bits += 6;
			if (bits >= 8) {
				bits -= 8;
				if (out_idx < N) out[out_idx++] = static_cast<uint8_t>((buf >> bits) & 0xFF);
			}
		}
		if (out_idx != N) throw "b64 size mismatch";
		return out;
	}

	inline std::vector<uint8_t> decodeB64(std::string_view s) {
		std::vector<uint8_t> out;
		out.reserve(s.size() * 3 / 4);
		uint32_t buf = 0;
		int bits = 0;
		for (char c: s) {
			if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
			buf = (buf << 6) | b64Val(c);
			bits += 6;
			if (bits >= 8) {
				bits -= 8;
				out.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF));
			}
		}
		return out;
	}

	inline std::string toZodKey(std::string res) {
		for (auto &c: res) {
			if (c == '\'') {
				c = ' ';
				continue;
			}
			if (!std::isalnum(c) && !std::isspace(c)) {
				c = ' ';
				continue;
			}
			c = std::tolower(c);
		}

		std::string ret;
		ret.reserve(res.size());

		auto it = res.begin();
		while (it != res.end()) {
			auto wordStart = std::find_if_not(it, res.end(), [](char c) {
				return std::isspace(c);
			});
			if (wordStart == res.end()) break;
			auto wordEnd = std::find_if(wordStart, res.end(), [](char c) {
				return std::isspace(c);
			});

			*wordStart = std::toupper(*wordStart);

			ret.append(wordStart, wordEnd);
			it = wordEnd;
		}
		return ret;
	}
}// namespace util::strings