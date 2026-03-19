//
// Created by farenain on 14/3/26.
//

#ifndef SDB_ERROR_HPP
#define SDB_ERROR_HPP

#include <stdexcept>
#include <cstring>

namespace sdb {
    /**
     * Class representing an internal error from the debugger.
     * We use our own class instead of using the runtime error
     * from C++
     */
    class error : public std::runtime_error {
    public:
        [[noreturn]]
        static void send(const std::string & what) {
            throw error(what);
        }
        [[noreturn]]
        static void send_errno(const std::string & prefix) {
            throw error(prefix + ": " + strerror(errno));
        }

    private:
        error(const std::string & what) : std::runtime_error(what) {}
    };
}

#endif //SDB_ERROR_HPP