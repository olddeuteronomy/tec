//! [tr]
#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"

void do_something() {
    TEC_ENTER("do_something");
    tec::Status status;
    //
    // Call some function that modifies `status` variable, like
    // status = open_file(filename);
    //
    TEC_TRACE("open_file returned {}.", status)
}

// If _TEC_TRACE macro is defined, you get the following
// in the console if the file opened successfully:
//
// [681236363] * do_something entered.
// [681236365] do_something: open_files returned [Success].
//! [tr]
