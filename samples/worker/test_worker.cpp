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
    int init_error;
    Seconds process_delay;
    Seconds finalize_delay;
    int finalize_error;
    Seconds worker_delay;
};


// Instantiate Worker class
using Worker = tec::Worker<WorkerParams>;

// Test command
static const tec::BasicMessage::cmd_t CMD_CALL_PROCESS = 1;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              MyWorker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class MyWorker: public Worker {
public:
    MyWorker(const WorkerParams& params) : Worker(params) {}

protected:
    tec::Result<> init() override {
        TEC_ENTER("init()");

        // Emulate error
        if( params().init_error ) {
            return params().init_error;
        }
        // Pause
        std::this_thread::sleep_for(params().init_delay);

        // Initiate processing
        send({CMD_CALL_PROCESS});
        return{};
    }

    void process(const tec::BasicMessage&) override
    {
        TEC_ENTER("process()");
        TEC_TRACE("count=%.", ++params().count);
        // Pause...
        std::this_thread::sleep_for(params().process_delay);
        // ...then repeat processing
        send({CMD_CALL_PROCESS});
    }

    tec::Result<> finalize() override
    {
        TEC_ENTER("finalize()");
        // Pause...
        std::this_thread::sleep_for(params().finalize_delay);
        // Return error
        return params().finalize_error;
    }
};


int test_worker() {
    // Emulate delays and errors.
    WorkerParams params{
        /*int count*/ 0,

        /*Seconds init_delay*/ Seconds{5},
        /*Status  init_error*/ 0, // 0, set to 1 to emulate init() error

        /*Seconds process_delay*/ Seconds{1}, // 1, delay in process() callback

        /*Seconds finalize_delay*/ Seconds{2},
        /*Status  finalize_error*/ 0, // 0, set to 2 to emulate finalize() error

        /*Seconds worker_delay*/ Seconds{10} // 0, delay between worker's run() and terminate()
    };
    MyWorker worker(params);
    worker.create();

    // Pause worker if everything is OK
    if( worker.run().ok() ) {
        std::this_thread::sleep_for(params.worker_delay);
    }

    // Gently terminate the worker
    auto retval = worker.terminate().code;

    // Print metrics
    auto mcs = worker.metrics();
    tec::println("\nMetrics");
    tec::println("init:     % %", mcs.time_init.count(), mcs.units.c_str());
    tec::println("exec:     % %", mcs.time_exec.count(), mcs.units.c_str());
    tec::println("finalize: % %", mcs.time_finalize.count(), mcs.units.c_str());
    tec::println("total:    % %", mcs.time_total.count(), mcs.units.c_str());

    return retval.value();
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

    int retval = test_worker();

    tec::println("\nexit_code() = %", retval);
    tec::print("Press <Enter> to quit ...");

#if defined(UNICODE) && !defined(__TEC_WINDOWS__)
    std::wgetchar();
#else
    std::getchar();
#endif
    return retval;
}
