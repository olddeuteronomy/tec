#include <functional>
#include <thread>
#include "tec/tec_semaphore.hpp"

void proc(const tec::Signal& start) {
    // The thread is suspended until `start` is signalled.
    start.wait();

    // The thread resumes. Do something.
    // ...
}

void do_asynch() {
    tec::Signal sig_start;
    // Create a thread in suspended state.
    std::thread thr(proc, std::ref(sig_start));
    // ...
    // Resume the thread proc.
    sig_start.set();
    // ...
}
