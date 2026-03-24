//
// Created by farenain on 14/3/26.
//

#ifndef SDB_PROCES_HPP
#define SDB_PROCES_HPP

#include <filesystem>
#include <memory>
#include <sys/types.h>
#include <cstdint>

#include <libsdb/registers.hpp>

namespace sdb {
    /**
     * States for the process, we keep track of them
     * during the debugging session.
     */
    enum class process_state {
        stopped,
        running,
        exited,
        terminated
    };

    /**
     * Structure with information about the state of the
     * process, and information about the signal to the
     * process.
     */
    struct stop_reason {
        stop_reason(int wait_status);

        process_state reason;
        std::uint8_t info;
    };


    /**
     * @brief This class represents a process, and it will allow
     * the user to process it. The process cannot be copied, because
     * this would mean a new process would be created in the system.
     */
    class process {
    public:
        // Static constructors, a process cannot be created in any other way
        /**
         * Launch constructor that given a path to a binary, it
         * creates a process.
         *
         * @param path path to the binary to create a process.
         * @return unique pointer to the created process.
         */
        static std::unique_ptr<process> launch(std::filesystem::path path, bool debug = true);

        /**
         * Attach constructor that given a pid, it attaches
         * to a process.
         *
         * @param pid process to attach to.
         * @return unique pointer to the created process.
         */
        static std::unique_ptr<process> attach(pid_t pid);
        // Deleted constructors and copy constructors
        // in this way the user cannot create its own processes
        process() = delete;
        process(const process&) = delete;
        process& operator=(const process&) = delete;

        // Destructor of the process to free any acquired resource
        ~process();

        void resume();
        stop_reason wait_on_signal();

        /**
         * Return pid of process
         * @return pid of represented process.
         */
        [[nodiscard]] pid_t pid() const {
            return pid_;
        }

        [[nodiscard]] process_state state() const {
            return state_;
        }

        registers& get_registers() {
            return *registers_;
        }

        [[nodiscard]] const registers& get_registers() const {
            return *registers_;
        }

        void write_user_area(std::size_t offset, std::uint64_t data) const;

        void write_fprs(const user_fpregs_struct& fprs);

        void write_gprs(const user_regs_struct& fprs);

    private:
        /**
         * Private constructor for Process, used internally
         * to create a process through launch or attach.
         * @param pid process pid.
         * @param terminate_on_end way of terminating the process.
         */
        process(pid_t pid, bool terminate_on_end, bool is_attached) :
            pid_(pid), terminate_on_end_(terminate_on_end), is_attached_(is_attached),
            registers_(new registers(*this))
        {}

        void read_all_registers();

        pid_t pid_ = 0;
        bool terminate_on_end_ = true;
        process_state state_ = process_state::stopped;
        bool is_attached_ = true;
        std::unique_ptr<registers> registers_;

    };
}

#endif //SDB_PROCES_HPP