#pragma once

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <print>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif


namespace debug {
	namespace detail {
		inline std::filesystem::path executablePath() {
#ifdef _WIN32
			std::wstring buffer(4096, L'\0');
			const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			buffer.resize(length);
			return {buffer};
#else
			std::error_code ec;
			return std::filesystem::read_symlink("/proc/self/exe", ec);
#endif
		}

		inline std::ofstream &logFile() {
			static std::ofstream file = [] {
				std::error_code ec;
				auto logsDir = executablePath().parent_path() / "logs";
				std::filesystem::create_directories(logsDir, ec);
				return std::ofstream{logsDir / "zzz_packet_capture.log", std::ios::app};
			}();
			return file;
		}

		inline std::string &lineBuffer() {
			static std::string buffer;
			return buffer;
		}

		inline void appendToLog(std::string_view text, bool endLine) {
			static std::mutex mutex;
			std::lock_guard lock{mutex};

			auto &buffer = lineBuffer();
			buffer.append(text);

			if (!endLine) return;

			auto &file = logFile();
			if (file.is_open()) {
				file << std::format("[{:%Y-%m-%d %H:%M:%S}] ", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())) << buffer << '\n';
				file.flush();
			}
			buffer.clear();
		}
	}// namespace detail

	template<class... Args>
	void print(std::format_string<Args...> fmt, Args &&...args) {
		std::print(fmt, std::forward<Args>(args)...);
		detail::appendToLog(std::format(fmt, std::forward<Args>(args)...), false);
	}

	inline void log() {
		std::println();
		detail::appendToLog("", true);
	}

	template<class... Args>
	void log(std::format_string<Args...> fmt, Args &&...args) {
		std::println(fmt, std::forward<Args>(args)...);
		detail::appendToLog(std::format(fmt, std::forward<Args>(args)...), true);
	}

	template<class... Args>
	void log(std::FILE *stream, std::format_string<Args...> fmt, Args &&...args) {
		std::println(stream, fmt, std::forward<Args>(args)...);
		detail::appendToLog(std::format(fmt, std::forward<Args>(args)...), true);
	}
}// namespace debug
