
//! [main]
#include <ctime>
#include <iostream>

#include "tec/tec_timestamp.hpp"

int main() {
    using namespace std::literals;

    std::cout << "Epoch:\n";
    tec::Timestamp ts0;
    std::cout
        << "Count: "<< ts0.count << "\n"
        << "UTC  : "<< ts0.utc_time_str() << "\n"
        << "Local: "<< ts0.local_time_str() << "\n"
        ;
    std::cout << "\nNow:\n";
    tec::Timestamp ts1{tec::Timestamp::now()};
    std::cout
        << "Count: "<< ts1.count << "\n"
        << "UTC  : "<< ts1.utc_time_str() << "\n"
        << "Local: "<< ts1.local_time_str() << "\n"
        ;
    std::cout << "\nOne day and five minutes ago:\n";
    tec::Timestamp ts2{ts1.dur() - 24h - 5min};
    std::cout
        << "Count: "<< ts2.count << "\n"
        << "UTC  : "<< ts2.utc_time_str() << "\n"
        << "Local: "<< ts2.local_time_str() << "\n"
        ;
    std::cout << "\nOne week forward:\n";
    tec::Timestamp ts3{(ts1.dur() + 7 * 24h).count()};
    std::cout
        << "Count: "<< ts3.count << "\n"
        << "UTC  : "<< ts3.utc_time_str() << "\n"
        << "Local: "<< ts3.local_time_str() << "\n"
        ;

    return 0;
}

//! [main]


//! [output]
// Epoch:
// Count: 0
// UTC  : 1970-01-01T00:00:00Z
// Local: 1970-01-01T03:00:00+0300

// Now:
// Count: 1768992601096852237
// UTC  : 2026-01-21T10:50:01Z
// Local: 2026-01-21T13:50:01+0300

// One day and five minutes ago:
// Count: 1768905901096852237
// UTC  : 2026-01-20T10:45:01Z
// Local: 2026-01-20T13:45:01+0300

// One week forward:
// Count: 1769597401096852237
// UTC  : 2026-01-28T10:50:01Z
// Local: 2026-01-28T13:50:01+0300
//! [output]
