// Time-stamp: <Last changed 2025-04-01 01:43:52 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2025 The Emacs Cat (https://github.com/olddeuteronomy/tec).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------
----------------------------------------------------------------------*/

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_result.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_worker.hpp"
#include "tec/tec_worker_thread.hpp"
#include <thread>


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Test Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Test parameters.
struct TestParams
{
    tec::Seconds init_delay;
    tec::Result init_result; // To emulate on_init() failure
    tec::Seconds process_delay;
    tec::Seconds exit_delay;
    tec::Result exit_result; // To emulate on_exit() failure
};

// Instantiate Test Worker class using default tec::Message.
struct TestMessage: public tec::WorkerMessage {
    int counter;
};

// Test command.
constexpr static const tec::WorkerMessage::Command CMD_CALL_PROCESS = 1;

// Instantiate TestWorker.
using TestWorkerTraits = tec::worker_traits<
    TestParams,
    TestMessage
    >;
using TestWorker = tec::WorkerThread<TestWorkerTraits>;



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              MyWorker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class MyWorker: public TestWorker {
public:
    MyWorker(const TestParams& params)
        : TestWorker{params}
    {
        // Register handler.
        register_handler(
            CMD_CALL_PROCESS,
            [](auto worker, auto msg){
                TEC_ENTER("CMD_CALL_PROCESS");
                TEC_TRACE("counter={}", msg.counter);
                if( msg.counter <= 10 ) {
                    // Continue processing.
                    std::this_thread::sleep_for(worker.params().process_delay);
                    worker.send({{CMD_CALL_PROCESS}, msg.counter + 1});
                }
                else {
                    // Stop message loop and terminate the Worker.
                    worker.send(tec::quit<TestMessage>());
                }
            });
    }

protected:
    tec::Result on_init() override {
        // Emulates some extra processing.
        std::this_thread::sleep_for(params().init_delay);
        if( params().init_result ) {
            // Initiate processing, starting with 1.
            send({{CMD_CALL_PROCESS}, 1});
        }
        return params().init_result;
    }

    tec::Result on_exit() override {
        return params().exit_result;
    }
};


tec::Result test_worker() {
    // Emulate delays and errors.
    TestParams params{
        .init_delay = tec::Seconds{2},
        .init_result = {}, // Set to {1, "on_init() failed"} to emulate on_init() error.
        .process_delay = tec::Seconds{1},
        .exit_delay = tec::Seconds{2},
        .exit_result =  {2, "on_exit() failed"}, // Set to {2, "on_exit() failed"} to emulate on_exit() error.

    };

    // Build the daemon.
    auto daemon{tec::Daemon::Builder<MyWorker>{}(params)};

    // Start the daemon and check for an initialization error.
    auto result = daemon->run();
    if( !result ) {
        return result;
    }

    // Wait for daemon is finished.
    daemon->sig_terminated().wait();

    // This call to `terminate)()` is not required if we don't want to get
    // the result of daemon termination.
    return daemon->terminate();
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
    tec::println("*** Running {} built at {}, {} with {} ***", __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    auto result = test_worker();

    tec::println("\nExited with {}", result);
    tec::print("Press <Enter> to quit ...");
    std::getchar();

    return result.code.value_or(tec::Error::Code<>::Unspecified);
}
