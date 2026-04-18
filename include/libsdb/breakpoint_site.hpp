//
// Created by farenain on 11/4/26.
//

#ifndef SDB_BREAKPOINT_SITE_HPP
#define SDB_BREAKPOINT_SITE_HPP

#include <cstdint>
#include <cstddef>
#include <libsdb/types.hpp>

namespace sdb {
    // we will add process as a friend class to use the private
    // constructors, so we do a forward declaration
    class process;

    /// @brief breakpoint_site defines a software breakpoint with a virtual address
    /// it provides information about the breakpoint like an id, if enabled or not,
    /// the address, and also utilities to enable it or disable it, etc.
    class breakpoint_site {
    public:
        /// Avoid any public constructor, copy operator or assignment
        /// only a process should be able to create a breakpoin
        /// for safety reasons to manage virtual memory.
        breakpoint_site() = delete;
        breakpoint_site(const breakpoint_site&) = delete;
        breakpoint_site& operator=(const breakpoint_site&) = delete;

        // Later we will use the id_type to have different breakpoints
        // generate it as a definition instead of only using std::int32_t
        using id_type = std::int32_t;
        id_type id() const { return id_; }

        void enable();
        void disable();

        bool is_enabled() const { return is_enabled_; }
        virt_addr address() const { return address_; }

        // helper, used to internally check the addresses
        bool at_address(virt_addr addr) const {
            return address_ == addr;
        }

        // helper, used to know if address is in a virtual address range
        bool in_range(virt_addr low, virt_addr high) const {
            return low <= address_ and address_ < high;
        }

    private:
        // constructor from the software breakpoint, only used
        // by process
        breakpoint_site(
            process& proc, virt_addr address);
        friend process; // so process can use this constructor

        id_type id_;
        process* process_;
        virt_addr address_;
        bool is_enabled_;
        // a software breakpoint is implement with a simple byte
        // we keep it in this object
        std::byte saved_data_;
    };
}

#endif //SDB_BREAKPOINT_SITE_HPP