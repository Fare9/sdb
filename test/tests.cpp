//
// Created by farenain on 11/3/26.
//
#include <catch2/catch_test_macros.hpp>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>

#include <sys/types.h>
#include <signal.h>

#include <fstream>
#include <iostream>

using namespace sdb;

namespace {
    /**
     * Help function to retrieve from /proc/<pid>/stat the
     * process status character from the line.
     *
     * @param pid pid to retrieve the information.
     * @return character representing the status of a process.
     */
    char get_process_status(pid_t pid) {
        std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
        std::string data;
        std::getline(stat, data);
        auto index_of_last_parenthesis = data.rfind(')');
        auto index_of_status_indicator = index_of_last_parenthesis + 2;
        return data[index_of_status_indicator];
    }


    bool process_exists(pid_t pid) {
        auto ret = kill(pid, 0);
        return ret != -1 and errno != ESRCH;
    }
}

TEST_CASE("process::launch success", "[process]") {
    auto proc = process::launch("/bin/true");
    CAPTURE(proc->pid());
    REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch no such program", "[process]") {
    REQUIRE_THROWS_AS(process::launch("no such program"), error);
}

TEST_CASE("process::attach success", "[process]") {
    auto target = process::launch("./targets/run_endlessly", false);
    auto proc = process::attach(target->pid());
    char status = get_process_status(target->pid());
    REQUIRE(status == 't');
}

TEST_CASE("process::attach invalid PID", "[process]") {
    REQUIRE_THROWS_AS(process::attach(0), error);
}

TEST_CASE("process::resume success", "[process]") {
    {
        auto proc = process::launch("targets/run_endlessly");
        proc->resume();
        auto status = get_process_status(proc->pid());
        // Check if status is running or success
        auto success = status == 'R' or status == 'S';
        REQUIRE(success);
    }

    {
        auto target = process::launch("targets/run_endlessly", false); // launch without debugging
        auto proc = process::attach(target->pid()); // attach to it
        proc->resume();
        auto status = get_process_status(proc->pid());
        auto success = status == 'R' or status == 'S';
        REQUIRE(success);
    }
}

TEST_CASE("process::resume already terminated", "[process]") {
    auto proc = process::launch("targets/end_immediately");
    proc->resume();
    proc->wait_on_signal(); // wait until it finishes
    REQUIRE_THROWS_AS(proc->resume(), error);
}