// Time-stamp: <Last changed 2026-02-03 15:56:30 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/

#include <string>
#include <thread>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_message.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_worker.hpp"


// Test parameters.
struct TestParams
{
    tec::Seconds init_delay;
    tec::Status  init_result; // To emulate on_init() failure.
    tec::Seconds process_delay;
    tec::Seconds exit_delay;
    tec::Status  exit_result; // To emulate on_exit() failure.
    int max_count; // Number of
};

// Test compound message.
struct Position {
    int x;
    int y;
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           TestWorker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class TestWorker final: public tec::Worker<TestParams> {
public:
    explicit TestWorker(const TestParams& params)
        : tec::Worker<TestParams>{params}
    {
        register_callback<TestWorker, int>(this, &TestWorker::process_int);
        register_callback<TestWorker, std::string>(this, &TestWorker::process_string);
        register_callback<TestWorker, const char*>(this, &TestWorker::process_const_char);
        register_callback<TestWorker, Position>(this, &TestWorker::process_position);
    }

protected:
    virtual void process_int(const tec::Message& msg) {
        TEC_ENTER("HANDLER <int>");
        const int counter = std::any_cast<int>(msg);
        TEC_TRACE(">>> counter={}", counter);
        if( counter <= params().max_count ) {
            // Increment `counter` and continue processing...
            std::this_thread::sleep_for(params().process_delay);
            // ... by sending a message to itself.
            send({counter + 1});
        }
        else {
            // Quit message loop and terminate the Worker.
            // NOTE: you cannot use `terminate()` in the Worker's thread!
            send(tec::nullmsg());
        }
    }

    virtual void process_string(const tec::Message& msg) {
        TEC_ENTER("HANDLER <string>");
        auto str = std::any_cast<std::string>(msg);
        TEC_TRACE(">>> \"{}\"", str);
    }

    virtual void process_const_char(const tec::Message& msg) {
        TEC_ENTER("HANDLER <const char*>");
        auto str = std::any_cast<const char*>(msg);
        TEC_TRACE(">>> \"{}\"", str);
    }

    virtual void process_position(const tec::Message& msg) {
        TEC_ENTER("HANDLER <Position>");
        auto pos = std::any_cast<Position>(msg);
        TEC_TRACE(">>> x={} y={}", pos.x, pos.y);
    }

protected:
    tec::Status on_init() override {
        // Emulates some extra processing and possible initialization error.
        std::this_thread::sleep_for(params().init_delay);
        if( params().init_result ) {
            // Initiate processing, starting with 1.
            send({1});
        }
        return params().init_result;
    }

    tec::Status on_exit() override {
        return params().exit_result;
    }

};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                            TEST
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Status test_daemon() {
    // Emulate delays and errors.
    TestParams params{
        .init_delay    = tec::Seconds{2},
        .init_result   = {}, // Set to {1, "on_init() failed"} to emulate on_init() error.
        .process_delay = tec::Seconds{1},
        .exit_delay    = tec::Seconds{2},
        .exit_result   = {}, // Set to {2, "on_exit() failed"} to emulate on_exit() error.
        .max_count = 10,
    };

    // Build a daemon using the derived TestWorker class.
    auto daemon{tec::Daemon::Builder<TestWorker>{}(params)};

    // Start the daemon and check for an initialization error.
    auto status = daemon->run();
    if( !status ) {
        return status;
    }

    // Send different messages.
    daemon->send({std::string("This is a string!")});
    daemon->send({"This is a const char*!"});
    daemon->send({Position{234, 71}});

    // Wait for daemon is finished.
    daemon->sig_terminated().wait();

    // This call to `terminate()` is not required
    // if we don't want to get the status of daemon termination.
    return daemon->terminate();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
    tec::println("*** Running {} built at {}, {} with {} ***",
                 __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    // Run the daemon.
    const auto status = test_daemon();

    tec::println("\nExited with {}", status);
    tec::print("Press <Enter> to quit ...");
    std::getchar();

    return status.code.value_or(0);
}
