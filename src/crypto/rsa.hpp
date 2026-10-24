#pragma once

#include "../crypto/key.hpp"
#include "../util/strings.hpp"
#include <cstdint>
#include <expected>
#include <mbedtls/bignum.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/rsa.h>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace crypto::rsa {
	constexpr size_t KeySize = 128;// RSA-1024 = 128 bytes
	constexpr uint64_t PublicExponent = 65537;

	namespace detail {
		struct Mpi {
			mbedtls_mpi value;

			Mpi() { mbedtls_mpi_init(&value); }
			~Mpi() { mbedtls_mpi_free(&value); }
			Mpi(const Mpi &) = delete;
			Mpi &operator=(const Mpi &) = delete;
		};

		template<size_t N>
		void readB64(mbedtls_mpi &mpi, std::string_view openTag, std::string_view closeTag) {
			auto bytes = util::strings::decodeB64(
				util::strings::extractXmlTag(clientPrivatekey, openTag, closeTag)
			);
			if (bytes.size() != N) throw std::runtime_error("RSA: unexpected decoded value size");
			if (mbedtls_mpi_read_binary(&mpi, bytes.data(), bytes.size()) != 0) {
				throw std::runtime_error("RSA: mbedtls_mpi_read_binary failed");
			}
		}
	}// namespace detail

	struct Key {
		mbedtls_rsa_context rsa{};
		mbedtls_entropy_context entropy{};
		mbedtls_ctr_drbg_context drbg{};
		std::mutex mutex;

		Key() {
			mbedtls_rsa_init(&rsa);
			mbedtls_entropy_init(&entropy);
			mbedtls_ctr_drbg_init(&drbg);

			try {
				static constexpr char personalization[] = "zzz_packet_capture";
				if (mbedtls_ctr_drbg_seed(
						&drbg,
						mbedtls_entropy_func,
						&entropy,
						reinterpret_cast<const unsigned char *>(personalization),
						sizeof(personalization) - 1
					)
					!= 0) {
					throw std::runtime_error("RSA: mbedtls_ctr_drbg_seed failed");
				}

				detail::Mpi n, d, p, q;
				detail::readB64<KeySize>(n.value, "<Modulus>", "</Modulus>");
				detail::readB64<KeySize>(d.value, "<D>", "</D>");
				detail::readB64<KeySize / 2>(p.value, "<P>", "</P>");
				detail::readB64<KeySize / 2>(q.value, "<Q>", "</Q>");

				detail::Mpi e;
				if (mbedtls_mpi_lset(&e.value, static_cast<mbedtls_mpi_sint>(PublicExponent)) != 0) {
					throw std::runtime_error("RSA: mbedtls_mpi_lset failed");
				}

				if (mbedtls_rsa_import(&rsa, &n.value, &p.value, &q.value, &d.value, &e.value) != 0) {
					throw std::runtime_error("RSA: mbedtls_rsa_import failed");
				}
				if (mbedtls_rsa_complete(&rsa) != 0) {
					throw std::runtime_error("RSA: mbedtls_rsa_complete failed");
				}
				if (mbedtls_rsa_check_privkey(&rsa) != 0) {
					throw std::runtime_error("RSA: mbedtls_rsa_check_privkey failed");
				}
			} catch (...) {
				mbedtls_ctr_drbg_free(&drbg);
				mbedtls_entropy_free(&entropy);
				mbedtls_rsa_free(&rsa);
				throw;
			}
		}

		~Key() {
			mbedtls_ctr_drbg_free(&drbg);
			mbedtls_entropy_free(&entropy);
			mbedtls_rsa_free(&rsa);
		}
		Key(const Key &) = delete;
		Key &operator=(const Key &) = delete;
	};

	inline std::expected<std::vector<uint8_t>, std::string> decryptBlock(std::span<const uint8_t> ciphertext) {
		static Key key;
		std::lock_guard lock{key.mutex};

		if (ciphertext.size() != KeySize) return std::unexpected("RSA: ciphertext size does not match key size");

		std::vector<uint8_t> out(KeySize);
		size_t outlen = 0;
		int ret = mbedtls_rsa_pkcs1_decrypt(&key.rsa, mbedtls_ctr_drbg_random, &key.drbg, &outlen, ciphertext.data(), out.data(), out.size());
		if (ret != 0) {
			char err[128]{};
			mbedtls_strerror(ret, err, sizeof(err));
			return std::unexpected(std::string{"RSA decrypt failed: "} + err);
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
