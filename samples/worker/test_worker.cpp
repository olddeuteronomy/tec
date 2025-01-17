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

using Seconds = tec::Seconds;

// Test parameters
struct WorkerParams
{
  int count;
  Seconds init_delay;
  tec::Result init_result; // To emulate init() failure
  Seconds process_delay;
  Seconds finalize_delay;
  tec::Result finalize_result; // To emulate finalize() failure
  Seconds worker_delay;
};


// Instantiate Worker class
using Worker = tec::Worker<WorkerParams>;

// Test command
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

        // Emulate error
        if( !params_.init_result ) {
            TEC_TRACE(params_.init_result);
            return params_.init_result;
        }
        // Pause
        std::this_thread::sleep_for(params_.init_delay);

        // Initiate processing again.
        send({CMD_CALL_PROCESS});
        return{};
    }

    void process(const tec::Message&) override
    {
        TEC_ENTER("Test::process()");
        TEC_TRACE("count=%.", ++params_.count);
        // Pause...
        std::this_thread::sleep_for(params_.process_delay);
        // ...then repeat processing.
        send({CMD_CALL_PROCESS});
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

        /*Seconds init_delay*/ Seconds{5},
        /*tec::Result  init_result*/ {}, // Set to {1, "init() failed"} to emulate init() error.

        /*Seconds process_delay*/ Seconds{1}, // 1, delay in process() callback.

        /*Seconds finalize_delay*/ Seconds{2},
        /*tec::Result  finalize_result*/ {2, "finalize() failed"}, // Set to {2, "finalize() failed"} to emulate finalize() error.

        /*Seconds worker_delay*/ Seconds{10} // Delay between worker's run() and terminate().
    };
    MyWorker worker(params);

    if( !worker.run() ) {
        return worker.result();
    }

    // Run worker for params.worker_delay seconds.
    std::this_thread::sleep_for(params.worker_delay);

    // Gently terminate the worker, obtaining finalize() callback result correctly.
    return worker.terminate();

    // NOTE: We can't get the proper result of finalize() callback here.
    // return worker.result();
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEC_DECLARE_TRACER();

int main()
{
    tec::println("*** Running % built at %, % with % ***", __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    auto result = test_worker();

    tec::println("\nExited with %", result);
    tec::print("Press <Enter> to quit ...");
    std::getchar();

    return result.code.value_or(tec::Result::ErrCode::Unspecified);
}
