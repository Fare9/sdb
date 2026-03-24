//
// Created by eblazquez on 3/21/26.
//

#ifndef SDB_REGISTERS_HPP
#define SDB_REGISTERS_HPP

#include <variant>

#include <sys/user.h>
#include <libsdb/register_info.hpp>
#include <libsdb/types.hpp>


namespace sdb {
    class process;
    class registers {
    public:
        // No public constructors available, the registers
        // will be generated privately
        registers() = delete;
        registers(const registers&) = delete;
        registers& operator=(const registers&) = delete;

        // value will be used to hold the different
        // values from the registers, we will implement
        // it as a std::variant.
        using value = std::variant<
            std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
            std::int8_t, std::int16_t, std::int32_t, std::int64_t,
            float, double, long double,
            byte64, byte128>;
        [[nodiscard]] value read(const register_info& info) const;
        void write(const register_info& info, value val);

        // A few templates to allow read and write values by id
        template <class T>
        T read_by_id_as(const register_id id) const {
            return std::get<T>(read(register_info_by_id(id)));
        }
        void write_by_id(const register_id id, const value &val) {
            write(register_info_by_id(id), val);
        }

    private:
        // only the sdb::process class should be able
        // to generate a register, so we declare it as
        // a friend class.
        friend process;
        registers(process& proc) : proc_(&proc) {};

        user data_;
        process* proc_;
    };
}

#endif //SDB_REGISTERS_HPP