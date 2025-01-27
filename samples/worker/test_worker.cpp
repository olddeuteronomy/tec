/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2024 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
#include "tec/tec_trace.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_worker.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Test Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Test parameters
struct WorkerParams
{
  int count;
  tec::Seconds init_delay;
  tec::Result init_result; // To emulate init() failure
  tec::Seconds process_delay;
  tec::Seconds finalize_delay;
  tec::Result finalize_result; // To emulate finalize() failure
  tec::Seconds worker_delay;
};


// Instantiate Worker class using default tec::Message.
using Worker = tec::Worker<WorkerParams>;

// Test command.
static constexpr const tec::Message::cmd_t CMD_CALL_PROCESS = 1;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              MyWorker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class MyWorker: public Worker {
public:
    MyWorker(const WorkerParams& params) : Worker(params) {}

protected:
    tec::Result init() override {
        TEC_ENTER("Test::init()");

        // Emulate error.
        if( !params_.init_result ) {
            TEC_TRACE(params_.init_result);
            return params_.init_result;
        }
        // Pause.
        std::this_thread::sleep_for(params_.init_delay);

        // Initiate processing.
        send({CMD_CALL_PROCESS});
        return{};
    }

    void process(const tec::Message&) override
    {
        TEC_ENTER("Test::process()");
        TEC_TRACE("count={}.", ++params_.count);

        // Pause...
        std::this_thread::sleep_for(params_.process_delay);

        if( params_.count >= 10 ) {
            // Quit message loop.
            send({tec::Message::QUIT});
        }
        else {
            // ... otherwise repeat processing.
            send({CMD_CALL_PROCESS});
        }
    }

    tec::Result finalize() override
    {
        TEC_ENTER("Test::finalize()");
        // Pause...
        std::this_thread::sleep_for(params_.finalize_delay);
        // Return result.
        return params_.finalize_result;
    }
};


tec::Result test_worker() {
    // Emulate delays and errors.
    WorkerParams params{
        /*int count*/ 0,

        /*Seconds init_delay*/ tec::Seconds{5},
        /*tec::Result  init_result*/ {}, // Set to {1, "init() failed"} to emulate init() error.

        /*Seconds process_delay*/ tec::Seconds{1}, // 1, delay in process() callback.

        /*Seconds finalize_delay*/ tec::Seconds{2},
        /*tec::Result  finalize_result*/ {2, "finalize() failed"}, // Set to {2, "finalize() failed"} to emulate finalize() error.

        /*Seconds worker_delay*/ tec::Seconds{10} // Delay between worker's run() and terminate().
    };
    MyWorker worker(params);

    // Start the worker and check for initialization error.
    if( !worker.run() ) {
        return worker.result();
    }

    // Wait for worker is finished.
    worker.sig_terminated().wait();
    return worker.result();
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main()
{
    tec::println("*** Running {} built at {}, {} with {} ***", __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    auto result = test_worker();

    tec::println("\nExited with {}", result);
    tec::print("Press <Enter> to quit ...");
    std::getchar();

    return result.code.value_or(tec::Result::ErrCode::Unspecified);
}
