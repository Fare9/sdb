//
// Created by eblazquez on 4/2/26.
//

#ifndef SDB_PARSE_HPP
#define SDB_PARSE_HPP

#include <array>
#include <cstddef>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sdb {
    template <class I>
    std::optional<I> to_integral(std::string_view sv, int base = 10) {
        auto begin = sv.begin();

        if (base == 16 and sv.size() > 1
            and begin[0] == '0' and begin[1] == 'x') {
            begin += 2;
        }
        if (base == 2 and sv.size() > 1
            and begin[0] == '0' and begin[1] == 'b') {
            begin += 2;
        }

        I ret;
        auto result = std::from_chars(begin, sv.end(), ret, base);

        if (result.ptr != sv.end())
            return std::nullopt;
        return ret;
    }

    // We extend the to_integral for std::byte, we use template specialization
    // that will be used to call this function when std::byte isprovided, used
    // later to read an array
    template<>
    inline std::optional<std::byte> to_integral(std::string_view sv, int base) {
        auto uint8 = to_integral<std::uint8_t>(sv, base);
        if (uint8) return static_cast<std::byte>(*uint8);
        return std::nullopt;
    }

    template <class F>
    std::optional<F> to_float(std::string_view sv) {
        F ret;
        auto result = std::from_chars(sv.begin(), sv.end(), ret);

        if (result.ptr != sv.end())
            return std::nullopt;
        return ret;
    }

    /// We can use templates to get the size for data
    template <std::size_t N>
    auto parse_vector(std::string_view text) {
        // invalid template for generating in case of error
        auto invalid = [] { sdb::error::send("Invalid format"); };

        std::array<std::byte, N> bytes;
        const char* c = text.data();

        // the data must start by '['
        if (*c++ != '[') invalid();

        for (unsigned long i = 0, e = N - 1; i < e; ++i) {
            bytes[i] = to_integral<std::byte>({ c, 4}, 16).value();
            c += 4;
            if (*c++ != ',') invalid();
        }

        bytes[N - 1] = to_integral<std::byte>({ c, 4}, 16).value();
        c += 4;

        // Check the user closed ']'
        if (*c++ != ']') invalid();
        if (c != text.end()) invalid();

        return bytes;
    }

}

#endif //SDB_PARSE_HPP