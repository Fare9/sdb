//
// Created by farenain on 14/3/26.
//
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>
#include <libsdb/pipe.hpp>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
    /**
     * In case there was an error, use this function to communicate
     * it through the pipe, so the other process will be able to
     * manage it.
     *
     * @param channel pipe used to communicate the error.
     * @param prefix prefix of the message to send.
     */
    void exit_with_perror(sdb::pipe &channel, std::string const &prefix) {
        auto message = prefix + ": " + std::strerror(errno);
        channel.write(reinterpret_cast<std::byte *>(message.data()), message.size());
        exit(-1);
    }
}


sdb::stop_reason::stop_reason(int wait_status) {
    if (WIFEXITED(wait_status)) {
        // check process exited
        reason = process_state::exited;
        info = WEXITSTATUS(wait_status);
    } else if (WIFSIGNALED(wait_status)) {
        // check process received a termination signal
        reason = process_state::terminated;
        info = WTERMSIG(wait_status);
    } else if (WIFSTOPPED(wait_status)) {
        // check process stopped (get information why)
        reason = process_state::stopped;
        info = WSTOPSIG(wait_status);
    }
}

std::unique_ptr<sdb::process> sdb::process::launch(std::filesystem::path path,
                                                   bool debug,
                                                   std::optional<int> stdout_replacement) {
    pid_t pid;
    // create a pipe for being able to communicate with the
    // created process. This pipe will be cloned into child
    // process memory.
    pipe channel(/*close_on_exec=*/true);

    if ((pid = fork()) < 0) {
        error::send_errno("fork failed");
    }

    if (pid == 0) {
        channel.close_read();

        if (stdout_replacement.has_value()) {
            if (dup2(stdout_replacement.value(), STDOUT_FILENO) < 0) {
                exit_with_perror(channel, "stdout replacement failed");
            }
        }

        // We need to request TRACEME, so we wait
        // for the debugger
        if (debug and ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            exit_with_perror(channel, "Tracing failed");
        }
        // Run the program name given
        if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
            exit_with_perror(channel, "Execlp failed");
        }
    }

    // the parent do not need to write anything
    channel.close_write();
    // read the data from the child process
    auto data = channel.read();
    channel.close_read();

    if (!data.empty()) {
        // if it is not empty, there was an error
        waitpid(pid, nullptr, 0);
        auto chars = reinterpret_cast<char *>(data.data());
        error::send(std::string(chars, chars + data.size()));
    }

    std::unique_ptr<process> proc(new process(pid, /*terminate_on_end=*/true, debug));

    if (debug) {
        proc->wait_on_signal();
    }

    return proc;
}

std::unique_ptr<sdb::process> sdb::process::attach(pid_t pid) {
    if (pid == 0) {
        error::send("Invalid PID");
    }
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        error::send_errno("Could not attach");
    }

    std::unique_ptr<process> proc(new process(pid, /*terminate_on_end=*/false, /*is_attached=*/true));

    proc->wait_on_signal();

    return proc;
}

void sdb::process::resume() {
    if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0) {
        error::send_errno("Could not resume");
    }
    state_ = process_state::running;
}

sdb::stop_reason sdb::process::wait_on_signal() {
    int wait_status;
    int options = 0;
    if (waitpid(pid_, &wait_status, options) < 0) {
        error::send_errno("waitpid failed");
    }
    stop_reason reason(wait_status);
    state_ = reason.reason;

    // now check that we just stopped (no other signal),
    // and read the registers
    if (is_attached_ and state_ == process_state::stopped) {
        read_all_registers();
    }


    return reason;
}

sdb::process::~process() {
    if (pid_ != 0) {
        int status;
        // We need to check if the debugger is attached or not
        // the process was maybe created just to launch it.
        if (is_attached_) {
            if (state_ == process_state::running) {
                // Stop process, and wait until it stops
                kill(pid_, SIGSTOP);
                waitpid(pid_, &status, 0);
            }
            // Detach from the process
            ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
            // Continue the process
            kill(pid_, SIGCONT);
        }

        if (terminate_on_end_) {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }
}

void sdb::process::read_all_registers() {
    // Retrieve the general purpose registers, use a ptrace call
    if (ptrace(PTRACE_GETREGS, pid_, nullptr, &get_registers().data_.regs) < 0) {
        error::send_errno("Could not get GPR registers");
    }
    // Retrieve the floating-point registers
    if (ptrace(PTRACE_GETFPREGS, pid_, nullptr, &get_registers().data_.i387) < 0) {
        error::send_errno("Could not get FPR registers");
    }
    // Getting the debug registers, we need to retrieve them reading memory
    for (int i = 0; i < 8; ++i) {
        // get id from dr0, dr1, ... using dr0 as base
        auto id = static_cast<int>(register_id::dr0) + i;
        // retrieve the info structure
        const auto &info = register_info_by_id(static_cast<register_id>(id));

        // we read the 64 bit value from the offset of the register
        errno = 0;
        std::int64_t data = ptrace(PTRACE_PEEKUSER, pid_, info.offset, nullptr);
        if (errno != 0) error::send_errno("Could not read debug register");

        // assign it to the registers
        get_registers().data_.u_debugreg[i] = data;
    }
}

void sdb::process::write_user_area(std::size_t offset, std::uint64_t data) const {
    if (ptrace(PTRACE_POKEUSER, pid_, offset, data) < 0) {
        error::send_errno("Could not write to user area");
    }
}

void sdb::process::write_fprs(const user_fpregs_struct &fprs) {
    if (ptrace(PTRACE_SETFPREGS, pid_, nullptr, &fprs) < 0) {
        error::send_errno("Could not set floating-point registers");
    }
}

void sdb::process::write_gprs(const user_regs_struct &gprs) {
    if (ptrace(PTRACE_SETREGS, pid_, nullptr, &gprs) < 0) {
        error::send_errno("Could not set general purpose registers");
    }
}
