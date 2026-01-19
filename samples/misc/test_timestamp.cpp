
#include <ctime>
#include <iostream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_timestamp.hpp"


int main() {
    tec::Timestamp ts{tec::Timestamp::now()};

    std::cout << "Count: "<< ts.count << "\n";

    std::cout << "UTC  : "<< ts.utc_time_str() << "\n";
    std::cout << "Local: "<< ts.local_time_str() << "\n";

    using namespace std::literals;

    std::cout << "\nOne day and 5 minutes ago:\n";
    tec::Timestamp ts2{(ts.dur() - 24h - 5min).count()};

    std::cout << "UTC  : "<< ts2.utc_time_str() << "\n";
    std::cout << "Local: "<< ts2.local_time_str() << "\n";

    std::cout << "\nOne week forward:\n";
    tec::Timestamp ts3{(ts.dur() + 7 * 24h).count()};

    std::cout << "UTC  : "<< ts3.utc_time_str() << "\n";
    std::cout << "Local: "<< ts3.local_time_str() << "\n";

    return 0;
}
