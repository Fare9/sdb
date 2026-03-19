//
// Created by farenain on 11/3/26.
//
#include <iostream>

#include <string_view>

#include <editline/readline.h>
#include <string>

#include <vector>

#include <algorithm>
#include <sstream>

#include <libsdb/process.hpp>

#include <libsdb/error.hpp>

// all the symbols that are only used in an implementation
// file, we will add it to an anonymous namespace, so they
// will not conflict with other names.
namespace {
    std::unique_ptr<sdb::process> attach(int argc, const char ** argv) {
        // Passing PID
        if (argc == 3 &&
            argv[1] == std::string_view("-p")) {
            pid_t pid = std::atoi(argv[2]);
            return sdb::process::attach(pid);
        }
        // Passing program name
        else {
            const char* program_path = argv[1];
            return sdb::process::launch(program_path);
        }
    }

    std::vector<std::string> split(const std::string_view str, const char delimiter) {
        std::vector<std::string> out{};
        std::stringstream ss {std::string{str}};
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            out.push_back(item);
        }

        return out;
    }

    bool is_prefix(const std::string_view str, const std::string_view of) {
        if (str.size() > of.size()) return false;
        return std::equal(str.begin(), str.end(), of.begin());
    }

    void print_stop_reason(
        const sdb::process& process,
        const sdb::stop_reason reason) {
        std::cout << "Process " << process.pid() << ' ';

        switch (reason.reason) {
            case sdb::process_state::exited:
                std::cout << "exited with status " << static_cast<int>(reason.info);
                break;
            case sdb::process_state::stopped:
                std::cout << "stopped with signal " << sigabbrev_np(reason.info);
                break;
            case sdb::process_state::terminated:
                std::cout << "terminated with signal " << sigabbrev_np(reason.info);
                break;
            default:
                break;
        }

        std::cout << std::endl;
    }

    void handle_command(
        const std::unique_ptr<sdb::process>& process,
        const std::string_view line) {
        const auto args = split(line, ' ');
        const auto& command = args[0];

        if (is_prefix(command, "continue")) {
            process->resume();
            auto reason = process->wait_on_signal();
            print_stop_reason(*process, reason);
        }
        else {
            std::cerr << "Unknown command\n";
        }
    }

    void main_loop(const std::unique_ptr<sdb::process>& process) {
        char *line = nullptr;
        while ((line = readline("sdb> ")) != nullptr) {
            std::string line_str;

            if (line == std::string_view("")) {
                free(line);
                if (history_length > 0) {
                    line_str = history_list()[history_length - 1]->line;
                }
            }
            else {
                line_str = std::string(line);
                add_history(line);
                free(line);
            }

            if (!line_str.empty()) {
                try {
                    handle_command(process, line_str);
                } catch (const sdb::error& err) {
                    std::cout << err.what() << std::endl;
                }
            }
        }
    }
}

int
main(int argc, const char **argv) {
    if (argc == 1) {
        std::cerr << "No arguments given\n";
        return -1;
    }

    try {
        auto process = attach(argc, argv);
        main_loop(process);
    } catch (const sdb::error& err) {
        std::cout << err.what() << std::endl;
    }
}