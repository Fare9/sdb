//
// Created by eblazquez on 3/21/26.
//

#include <iostream>
#include <type_traits>
#include <algorithm>

#include <libsdb/registers.hpp>
#include <libsdb/process.hpp>
#include <libsdb/bit.hpp>

namespace {
    /**
     * Function to widen the provided value to sdb::byte128, instead of allowing C++
     * to do that for us, we write a helper function. We will use compile time checks
     * so some of the checks will be solved earlier. We check the size, and provide
     * the next bigger type for the transformation.
     *
     * @tparam T any provided type.
     * @param info register info data.
     * @param t the value to convert.
     * @return std::array of 128 bits (16 bytes)
     */
    template<class T>
    sdb::byte128 widen(const sdb::register_info &info, T t) {
        using namespace sdb;
        if constexpr (std::is_floating_point_v<T>) {
            if (info.format == register_format::double_float) {
                return to_byte128(static_cast<double>(t));
            }
            if (info.format == register_format::long_double) {
                return to_byte128(static_cast<long double>(t));
            }
        } else if constexpr (std::is_signed_v<T>) {
            if (info.format == register_format::uint) {
                switch (info.size) {
                    case 2: return to_byte128(static_cast<std::int16_t>(t));
                    case 4: return to_byte128(static_cast<std::int32_t>(t));
                    case 8: return to_byte128(static_cast<std::int64_t>(t));
                }
            }
        }

        return to_byte128(t);
    }
}

/**
 * Reading the correct value will be a bit difficult, this part will
 * obtain the std::register_info, and from the data_ we will get the
 * base pointer to the register, then from the structures with register_info
 * will extract the offset, and access a specific size.
 *
 * @param info register where to extract the information
 * @return
 */
sdb::registers::value sdb::registers::read(const register_info &info) const {
    // obtain a pointer to the raw bytes
    auto bytes = as_bytes(data_);
    // now read with the correct size
    if (info.format == register_format::uint) {
        switch (info.size) {
            case 1:
                return from_bytes<std::uint8_t>(bytes + info.offset);
            case 2:
                return from_bytes<std::uint16_t>(bytes + info.offset);
            case 4:
                return from_bytes<std::uint32_t>(bytes + info.offset);
            case 8:
                return from_bytes<std::uint64_t>(bytes + info.offset);
            default:
                sdb::error::send("Unexpected register size");
        }
    } else if (info.format == register_format::double_float) {
        return from_bytes<double>(bytes + info.offset);
    } else if (info.format == register_format::long_double) {
        return from_bytes<long double>(bytes + info.offset);
    } else if (info.format == register_format::vector and info.size == 8) {
        return from_bytes<byte64>(bytes + info.offset);
    } else {
        return from_bytes<byte128>(bytes + info.offset);
    }
}

void sdb::registers::write(const register_info &info, value val) {
    auto bytes = as_bytes(data_);

    /**
     * The std::visit will allow us to avoid write many if/else
     * to check for the size of value. std::visit takes a function
     * and a std::variant and calls the given function with the
     * value stored in the std::variant. So, if we provide a generic
     * lambda, std::visit preserves the type, auto& will keep the
     * original value.
     */
    std::visit([&](auto &v) {
        // we now accept values smaller than the size of the register
        if (sizeof(v) <= info.size) {
            // here we obtain a value big enough for the register.
            auto wide = widen(info, v);
            auto val_bytes = as_bytes(wide);
            std::copy(val_bytes, val_bytes + info.size, bytes + info.offset);
        } else {
            std::cerr << "sdb::register::write called with "
                    "mismatched register and value sizes";
            std::terminate();
        }
    }, val);

    if (info.type == register_type::fpr) {
        proc_->write_fprs(data_.i387);
        return;
    }

    // We have a problem with the offsets, for both PTRACE_PEEKUSER and PTRACE_POKEUSER
    // we require the addresses to align to 8 bytes, they should be divisible by 8
    // with no remainder. This is a problem for the high 8-bit registers, which are offset
    // into the superregister (ah, bh, ch, dh). We can align the address through an
    // AND bitwise with the NOT of 0b111, setting the last three
    auto aligned_offset = info.offset & ~0b111;
    proc_->write_user_area(aligned_offset,
                           from_bytes<std::uint64_t>(bytes + aligned_offset));
}
