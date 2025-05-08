// Time-stamp: <Last changed 2025-05-08 21:54:31 by magnolia>
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
#include "tec/tec_worker.hpp"


//========================================================================
//                             TEST
//========================================================================

int main() {
    // Define Worker's parameters.
    struct Params {
        int start;
    } params{1};

    // Declare the worker.
    using MyWorker = tec::Worker<Params>;

    // Create the worker.
    auto wrk{MyWorker::Builder<MyWorker>{}(params)};

    // Register a handler for the `int' message.
    wrk->register_handler<int>(
        [](auto worker, auto msg) {
            TEC_ENTER("HANDLER [int]");
            int counter = std::any_cast<int>(msg);
            TEC_TRACE("counter={}", counter);
            if( counter <= 10 ) {
                // Emulate processing...
                std::this_thread::sleep_for(tec::Seconds{1});
                // Continue counting.
                worker.send({counter + 1});
            }
            else {
                // Quit message loop and terminate the worker.
                worker.send(tec::nullmsg());
            }
       });

    // Start the worker.
    wrk->run();
    // Initiate counting.
    wrk->send({params.start});
    // Wait for Worker is terminated.
    wrk->sig_terminated().wait();

    return 0;
}
