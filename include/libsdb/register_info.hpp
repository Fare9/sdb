//
// Created by farenain on 17/3/26.
//

#ifndef SDB_REGISTER_INFO_HPP
#define SDB_REGISTER_INFO_HPP

#include <algorithm>
#include <cstdint> // standard size integers
#include <cstddef>
#include <string_view> // manage char * with C++ API
#include <sys/user.h> // structure of registers from Linux

#include "error.hpp"

namespace sdb {
    /**
     * Provide each register in the system its own unique
     * enumerator value.
     */
    enum class register_id {
#define DEFINE_REGISTER(name,dwarf_id,size,offset,type,format) name
#include <libsdb/detail/registers.inc>
#undef DEFINE_REGISTER
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
    inline constexpr register_info g_register_infos[] = {
#define DEFINE_REGISTER(name, dwarf_id, size, offset, type, format) \
            { register_id::name, #name, dwarf_id, size, offset, type, format }
#include <libsdb/detail/registers.inc>
#undef DEFINE_REGISTER
    };

    /**
     * Helper function that we will use to create different
     * search functions.
     *
     * @tparam F Template to allow any function in the std::find_if.
     * @param f function used in the comparison to look for a register.
     * @return constant reference to a struct with the information of a register.
     */
    template<class F>
    const register_info &register_info_by(F f) {
        auto it = std::find_if(
            std::begin(g_register_infos),
            std::end(g_register_infos), f
        );

        if (it == std::end(g_register_infos))
            error::send("Can't find register info");

        return *it;
    }

    inline const register_info &register_info_by_id(register_id id) {
        return register_info_by([id](auto &i) { return i.id == id; });
    }

    inline const register_info &register_info_by_name(std::string_view name) {
        return register_info_by([name](auto &i) { return i.name == name; });
    }

    inline const register_info &register_info_by_dwarf(std::int32_t dwarf_id) {
        return register_info_by([dwarf_id](auto &i) { return i.dwarf_id == dwarf_id; });
    }
}

#endif //SDB_REGISTER_INFO_HPP
