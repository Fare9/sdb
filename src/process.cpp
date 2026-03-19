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

std::unique_ptr<sdb::process> sdb::process::launch(std::filesystem::path path, bool debug) {
    pid_t pid;
    // create a pipe for being able to communicate with the
    // created process. This pipe will be cloned into child
    // process memory.
    pipe channel(/*close_on_exec=*/true);

    if ((pid = fork()) < 0) {
        error::send_errno("fork failed");
    }

    if (pid == 0) {
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
