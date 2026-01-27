
//! [header]
//
// worker.hpp -- a common header.
//
#include "tec/tec_daemon.hpp"

// Worker parameters.
struct TestParams {
    // Add
};
// A compound message.
struct Position {
    int x;
    int y;
};

// Actual Worker implementation exposed as Daemon.
std::unique_ptr<tec::Daemon> create_daemon(const TestParams&);
//! [header]


//! [worker]
//
// worker.cpp -- TestWorker implementation.
//
#include "tec/tec_worker.hpp"
#include "worker.hpp"

class TestWorker final: public tec::Worker<TestParams> {
public:
    explicit TestWorker(const TestParams& params)
        : tec::Worker<TestParams>{params}
    {
        // Register callbacks.
        register_callback<TestWorker, int>(this, &TestWorker::process_int);
        register_callback<TestWorker, std::string>(this, &TestWorker::process_string);
        register_callback<TestWorker, Position>(this, &TestWorker::process_position);
    }

    // Process `int` message.
    virtual void process_int(const tec::Message& msg) {
        const int counter = std::any_cast<int>(msg);
        // Do something with `counter`...
    }

    // Process `std::string` message.
    virtual void process_string(const tec::Message& msg) {
        auto str = std::any_cast<std::string>(msg);
        // Do something with `str`...
    }

    // Process `Position` message.
    virtual void process_position(const tec::Message& msg) {
        auto pos = std::any_cast<Position>(msg);
        // Do something with `pos.x` and `pos.y`...
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Expose actual Worker implementation as Daemon.
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    std::unique_ptr<tec::Daemon> create_test_worker(const TestParams&)  {
        auto daemon{tec::Daemon::Builder<TestWorker>{}(params)};
        return daemon;
    }
//! [worker]


//! [run]
//
// run.cpp -- run TestWorker as Daemon.
// NOTE: The actual TestWorker implementation is completely hidden.
// Compilation units `worker.cpp` and `main.cpp` are independent.
//
#include "tec/tec_status.hpp"
#include "tec/tec_daemon.hpp"
#include "worker.hpp"

tec::Status run() {
    TestParams params{};
    // Create a daemon.
    auto daemon = create_daemon(params);

    // Start the daemon thread and check for an initialization error.
    auto status = daemon->run();
    if( !status ) {
        return status;
    }

    // Send different messages.

    // `process_int` will be called in the Worker's thread.
    daemon->send({7});
    // `process_string` will be called in the Worker's thread.
    daemon->send({std::string("This is a string!")});
    // `process_position` will be called in the Worker's thread.
    daemon->send({Position{2034, 710}});

    // Optional -- if we want to check an exit status.
    return daemon->terminate();
}
//! [run]
