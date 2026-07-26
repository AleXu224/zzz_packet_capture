#pragma once

#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "networking.hpp"
#include "optional"
#include "print"
#include "string"


namespace util::serializable {
	template<class T>
    [[nodiscard]] static inline std::optional<T> fromJson(std::string_view data) {
        T obj;
        auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(obj, data);
        if (ec) {
            std::println("Failed to parse {}: {}", typeid(T).name(), glz::format_error(ec, data));
            return std::nullopt;
        }
        return obj;
    }


    [[nodiscard]] inline std::optional<std::string> toJson(auto &&self)  {
        std::string buffer;
        auto ec = glz::write_json(self, buffer);
        if (ec) {
            std::println("Failed to serialize {}: {}", typeid(std::decay_t<decltype(self)>).name(), static_cast<int>(ec));
            return std::nullopt;
        }
        return buffer;
    }

    template<class T>
    [[nodiscard]] static inline std::optional<T> fromFile(std::string_view path) {
        std::ifstream file(std::string{path});
        if (!file.is_open()) {
            return std::nullopt;
        }
        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return util::serializable::fromJson<T>(data);
    }

    inline bool toFile(auto &&self, std::string_view path) {
        std::ofstream file(std::string{path});
        if (!file.is_open()) {
            std::println("Failed to open {} for writing.", path);
            return false;
        }
        auto json = util::serializable::toJson(self);
        if (!json.has_value()) {
            std::println("Failed to serialize {} to JSON.", typeid(std::decay_t<decltype(self)>).name());
            return false;
        }
        file << *json;
        return true;
    }

	template<class T>
    [[nodiscard]] static inline std::optional<T> fromNetwork(std::string_view url) {
        auto res = squi::Networking::get(std::string{url});
        if (!res.success || res.statusCode != 200) {
            std::println("Failed to fetch {}: {}", typeid(T).name(), res.error);
            return std::nullopt;
        }
        return util::serializable::fromJson<T>(res.body);
    }
}// namespace util