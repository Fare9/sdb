//
// Created by eblazquez on 3/21/26.
//

#ifndef SDB_TYPES_HPP
#define SDB_TYPES_HPP

#include <array>
#include <cstddef>

namespace sdb {
    class virt_addr {
    public:
        virt_addr() = default;

        // disallow implicit conversions setting the constructor
        // as explicit.
        explicit virt_addr(std::uint64_t addr)
            : addr_(addr) {
        }

        [[nodiscard]] std::uint64_t addr() const {
            return addr_;
        }

        /**
         * Different operators that we want to be able to do in the
         * virtual addresses
         */
        virt_addr operator+(std::int64_t offset) const {
            return virt_addr(addr_ + offset);
        }

        virt_addr operator-(std::int64_t offset) const {
            return virt_addr(addr_ - offset);
        }

        virt_addr &operator+=(std::int64_t offset) {
            addr_ += offset;
            return *this;
        }

        virt_addr &operator-=(std::int64_t offset) {
            addr_ -= offset;
            return *this;
        }

        bool operator==(const virt_addr &other) const {
            return addr_ == other.addr_;
        }

        bool operator!=(const virt_addr &other) const {
            return addr_ != other.addr_;
        }

        bool operator<(const virt_addr &other) const {
            return addr_ < other.addr_;
        }

        bool operator<=(const virt_addr &other) const {
            return addr_ <= other.addr_;
        }

        bool operator>(const virt_addr &other) const {
            return addr_ > other.addr_;
        }

        bool operator>=(const virt_addr &other) const {
            return addr_ >= other.addr_;
        }

    private:
        std::uint64_t addr_ = 0;
    };


    using byte64 = std::array<std::byte, 8>;
    using byte128 = std::array<std::byte, 16>;
}

#endif //SDB_TYPES_HPP
