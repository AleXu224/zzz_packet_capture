#pragma once

#include "../crypto/ec2b.hpp"
#include "../crypto/rsa.hpp"
#include "../util/strings.hpp"
#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "networking.hpp"
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dispatch {
	constexpr std::string_view version = "OSPRODWin3.0.0";
	constexpr std::string_view seed = "76400c67cad01e75";
	constexpr std::string_view token = "ffdbcb63cc855516";
	const std::string params = std::format("?version={}&rsa_ver=3&language=1&platform=3&seed={}&token={}&channel_id=1&sub_channel_id=1", version, seed, token);
	constexpr std::string_view queryDispatchUrl = "https://globaldp-prod-os01.zenlesszonezero.com/query_dispatch";

	struct QueryDispatchEntry {
		std::string title;
		std::string dispatch_url;
	};

	struct QueryDispatchResponse {
		std::vector<QueryDispatchEntry> region_list;
	};

	struct GatewayResponse {
		std::string content;
		std::string sign;
	};

	struct DecryptedBody {
		std::string client_secret_key;
	};

	inline std::vector<uint8_t> rsaDecryptBody(std::string_view b64Ciphertext) {
		auto ct = util::strings::decodeB64(b64Ciphertext);
		if (ct.empty()) throw std::runtime_error("dispatch: empty content");
		auto res = crypto::rsa::decryptMulti(std::span<const uint8_t>{ct});
		if (!res) throw std::runtime_error("dispatch: " + res.error());
		return *res;
	}

	inline std::string_view trimTrailingForward(std::string_view s) {
		auto end = s.find_first_of('\0');
		if (end == std::string_view::npos) return s;
		return s.substr(0, end);
	}

	constexpr glz::opts parseOpts{.comments = true, .error_on_unknown_keys = false};

	inline std::vector<std::pair<std::string, uint64_t>> deriveInitialSeeds() {
		auto url = std::string{queryDispatchUrl} + std::string{params};
		auto queryDispatchResp = squi::Networking::get(url);
		if (!queryDispatchResp.success) throw std::runtime_error("dispatch: " + queryDispatchResp.error);
		if (queryDispatchResp.statusCode != 200) throw std::runtime_error("dispatch: HTTP " + std::to_string(queryDispatchResp.statusCode));

		QueryDispatchResponse qdr{};
		auto ec = glz::read<parseOpts>(qdr, queryDispatchResp.body);
		if (ec) throw std::runtime_error("dispatch: query_dispatch JSON parse failed");

		std::vector<std::pair<std::string, uint64_t>> dispatchUrls;

		for (const auto &entry: qdr.region_list) {
			auto url = entry.dispatch_url + std::string{params};

			auto resp = squi::Networking::get(std::string{url});
			if (!resp.success) throw std::runtime_error("dispatch: " + resp.error);
			if (resp.statusCode != 200) throw std::runtime_error("dispatch: HTTP " + std::to_string(resp.statusCode));

			GatewayResponse gw{};
			auto ec = glz::read<parseOpts>(gw, resp.body);
			if (ec) throw std::runtime_error("dispatch: gateway JSON parse failed");

			auto plainBytes = rsaDecryptBody(gw.content);
			auto bytes = trimTrailingForward(
				std::string_view{reinterpret_cast<const char *>(plainBytes.data()), plainBytes.size()}
			);

			DecryptedBody inner{};
			ec = glz::read<parseOpts>(inner, bytes);
			if (ec) throw std::runtime_error("dispatch: inner JSON parse failed");

			auto ec2bBytes = util::strings::decodeB64(inner.client_secret_key);
			if (ec2bBytes.size() < 32) throw std::runtime_error("dispatch: ec2b too short");

			auto seed = crypto::ec2b::derive_seed(std::span<const uint8_t>{ec2bBytes});
			dispatchUrls.emplace_back(std::pair<std::string, uint64_t>{entry.title, seed});
		}
		return dispatchUrls;
	}

}// namespace dispatch