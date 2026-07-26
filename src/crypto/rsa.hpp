#pragma once

#include "../crypto/key.hpp"
#include "../util/pointer.hpp"
#include "../util/strings.hpp"
#include "openssl/param_build.h"
#include <array>
#include <cstdint>
#include <expected>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace crypto::rsa {
	constexpr size_t KeySize = 128;// RSA-1024 = 128 bytes
	constexpr uint64_t PublicExponent = 65537;

	struct Key {
		std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)> bld{nullptr, &OSSL_PARAM_BLD_free};
		std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey{nullptr, &EVP_PKEY_free};

		Key() : pkey(nullptr, &EVP_PKEY_free) {
			constexpr auto nBytes = util::strings::decodeB64Fixed<KeySize>(
				util::strings::extractXmlTag(clientPrivatekey, "<Modulus>", "</Modulus>")
			);
			constexpr auto dBytes = util::strings::decodeB64Fixed<KeySize>(
				util::strings::extractXmlTag(clientPrivatekey, "<D>", "</D>")
			);

			auto n = util::makePtr(BN_bin2bn(nBytes.data(), static_cast<int>(KeySize), nullptr), &BN_free);
			auto e = util::makePtr(BN_new(), &BN_free);
			auto d = util::makePtr(BN_bin2bn(dBytes.data(), static_cast<int>(KeySize), nullptr), &BN_free);
			if (!n || !e || !d || !BN_set_word(e.get(), static_cast<BN_ULONG>(PublicExponent))) {
				throw std::runtime_error("RSA: BN setup failed");
			}

			bld.reset(OSSL_PARAM_BLD_new());
			if (!bld || !OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_N, n.get()) || !OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_E, e.get()) || !OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_D, d.get())) {
				throw std::runtime_error("RSA: OSSL_PARAM_BLD failed");
			}

			auto params = util::makePtr(OSSL_PARAM_BLD_to_param(bld.get()), &OSSL_PARAM_free);
			if (!params) throw std::runtime_error("RSA: OSSL_PARAM_BLD_to_param failed");

			auto ctx = util::makePtr(EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr), &EVP_PKEY_CTX_free);
			EVP_PKEY *pKeyPtr = nullptr;
			if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) <= 0 || EVP_PKEY_fromdata(ctx.get(), &pKeyPtr, EVP_PKEY_KEYPAIR, params.get()) <= 0) {
				throw std::runtime_error("RSA: EVP_PKEY_fromdata failed");
			}
			pkey.reset(pKeyPtr);
		}
	};

	inline std::expected<std::vector<uint8_t>, std::string> decryptBlock(std::span<const uint8_t> ciphertext) {
		static Key key;

		auto ctx = util::makePtr(EVP_PKEY_CTX_new(key.pkey.get(), nullptr), &EVP_PKEY_CTX_free);
		if (!ctx) return std::unexpected("EVP_PKEY_CTX_new failed");

		if (EVP_PKEY_decrypt_init(ctx.get()) <= 0) {
			return std::unexpected("EVP_PKEY_decrypt_init failed");
		}

		if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) {
			return std::unexpected("EVP_PKEY_CTX_set_rsa_padding failed");
		}

		size_t outlen = KeySize;
		std::vector<uint8_t> out(KeySize);
		if (EVP_PKEY_decrypt(ctx.get(), out.data(), &outlen, ciphertext.data(), ciphertext.size()) <= 0) {
			return std::unexpected("RSA decrypt failed");
		}

		out.resize(outlen);
		return out;
	}

	inline std::expected<std::vector<uint8_t>, std::string> decryptMulti(std::span<const uint8_t> data) {
		if (data.size() % KeySize != 0) return std::unexpected("RSA: input not multiple of key size");
		std::vector<uint8_t> result;
		for (size_t i = 0; i < data.size(); i += KeySize) {
			auto ciphertext = data.subspan(i, KeySize);
			auto plain = decryptBlock(ciphertext);
			if (!plain) return std::unexpected(std::move(plain.error()));
			result.insert(result.end(), plain.value().begin(), plain.value().end());
		}
		return result;
	}

}// namespace crypto::rsa