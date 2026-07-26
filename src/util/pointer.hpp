#pragma once

namespace util {
    template<class T, class Destructor>
    [[nodiscard]] std::unique_ptr<T, Destructor> makePtr(T *ptr, Destructor d) {
        return std::unique_ptr<T, Destructor>(ptr, d);
    }
}