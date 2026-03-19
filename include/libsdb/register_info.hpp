//
// Created by farenain on 17/3/26.
//

#ifndef SDB_REGISTER_INFO_HPP
#define SDB_REGISTER_INFO_HPP

#include <cstdint> // standard size integers
#include <cstddef>
#include <string_view> // manage char * with C++ API
#include <sys/user.h> // structure of registers from Linux

namespace sdb {
    /**
     * Provide each register in the system its own unique
     * enumerator value.
     */
    enum class register_id {
        // ?
    };

    /**
     * The type of register, we have different
     * types: general purpose, floating-point,
     * debug...
     */
    enum class register_type {
        gpr, // general purpose register
        sub_gpr, // part of general purpose register
        fpr, // floating-point register
        dr, // debug register
    };

    /**
     * Different ways of interpreting a register.
     */
    enum class register_format {
        uint,
        double_float,
        long_double,
        vector
    };

    /**
     * Collects all the information needed about a single register.
     */
    struct register_info {
        register_id id;
        std::string_view name;
        std::int32_t dwarf_id;
        std::size_t size;
        std::size_t offset;
        register_type type;
        register_format format;
    };

    /**
     * Used to keep all the information for every register in
     * the system.
     */
    inline constexpr const register_info g_register_infos[] = {
        //?
    };
}

#endif //SDB_REGISTER_INFO_HPP