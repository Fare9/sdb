//
// Created by farenain on 11/3/26.
//
#include <catch2/catch_test_macros.hpp>
#include <libsdb/process.hpp>
#include <libsdb/error.hpp>
#include <libsdb/pipe.hpp>
#include <libsdb/bit.hpp>

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

TEST_CASE("Write register works", "[register]") {
    bool close_on_exec = false;

    sdb::pipe channel(close_on_exec);

    auto proc = process::launch(
        "targets/reg_write",
                /* debug= */true,
                /* stdout_replacement= */channel.get_write());
    channel.close_write();

    proc->resume();
    proc->wait_on_signal();

    // write rsi register with the second parameter
    auto& regs = proc->get_registers();
    regs.write_by_id(register_id::rsi, 0xcafecafe);

    // print the output
    proc->resume();
    proc->wait_on_signal();

    auto output = channel.read();
    REQUIRE(to_string_view(output) == "0xcafecafe");

    // Now check with the mm0
    regs.write_by_id(register_id::mm0, 0xba5eba11);

    proc->resume();
    proc->wait_on_signal();

    output = channel.read();
    REQUIRE(to_string_view(output) == "0xba5eba11");

    // Now check xmm0
    regs.write_by_id(register_id::xmm0, 42.24);

    proc->resume();
    proc->wait_on_signal();

    output = channel.read();
    REQUIRE(to_string_view(output) == "42.24");

    // We need now to test x87, and to do that we need
    // to write in st0, and then 2 other registers
    // fsw (the FPU status word)
    // ftw (the FPU tag word)
    regs.write_by_id(register_id::st0, 42.24l);
    // status word tracks the current size of the FPU stack
    // and reports errors like stack overflows, precision errors,
    // or divisions by zero. Bits 11 through 13 track the top
    // of the stack. Top of the stack starts at index 0 and
    // actually goes down, wrapping around up to 7. To push our
    // value to the stack, we set bits 11 through 13 to 7
    regs.write_by_id(register_id::fsw, std::uint16_t{0b0011100000000000});
    // Finally 16-bit tag register tracks which of the st registers are
    // valid, empty or special (meaning they contain NaNs or inifinity).
    // A tag of 0b11 means empty, 0b00 means valid. We want to set the first
    // tag to 0b00 and the rest to 0b11
    regs.write_by_id(register_id::ftw,
        std::uint16_t{0b0011111111111111});
    // Now we can write the test
    proc->resume();
    proc->wait_on_signal();

    output = channel.read();
    REQUIRE(to_string_view(output) == "42.24");
}

TEST_CASE("Read register works", "[register]") {
    auto proc = process::launch("targets/reg_read");
    auto& regs = proc->get_registers();

    proc->resume();
    proc->wait_on_signal();

    REQUIRE(regs.read_by_id_as<std::uint64_t>(register_id::r13) == 0xcafecafe);

    proc->resume();
    proc->wait_on_signal();

    REQUIRE(regs.read_by_id_as<std::uint8_t>(register_id::r13b) == 42);

    proc->resume();
    proc->wait_on_signal();

    REQUIRE(regs.read_by_id_as<byte64>(register_id::mm0) == to_byte64(0xba5eba11ull));

    proc->resume();
    proc->wait_on_signal();

    REQUIRE(regs.read_by_id_as<byte128>(register_id::xmm0) == to_byte128(64.125));

    proc->resume();
    proc->wait_on_signal();

    REQUIRE(regs.read_by_id_as<long double>(register_id::st0) == 64.125L);
}