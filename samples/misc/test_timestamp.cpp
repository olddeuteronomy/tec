
#include <ctime>
#include <iostream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_timestamp.hpp"


int main() {
    std::cout << "Epoch:\n";
    tec::Timestamp ts0;
    std::cout << "Count: "<< ts0.count << "\n";
    std::cout << "UTC  : "<< ts0.utc_time_str() << "\n";
    std::cout << "Local: "<< ts0.local_time_str() << "\n";

    std::cout << "\nNow:\n";
    tec::Timestamp ts1{tec::Timestamp::now()};
    std::cout << "Count: "<< ts1.count << "\n";
    std::cout << "UTC  : "<< ts1.utc_time_str() << "\n";
    std::cout << "Local: "<< ts1.local_time_str() << "\n";

    using namespace std::literals;

    std::cout << "\nOne day and five minutes ago:\n";
    tec::Timestamp ts2{ts1.dur() - 24h - 5min};
    std::cout << "Count: "<< ts2.count << "\n";
    std::cout << "UTC  : "<< ts2.utc_time_str() << "\n";
    std::cout << "Local: "<< ts2.local_time_str() << "\n";

    std::cout << "\nOne week forward:\n";
    tec::Timestamp ts3{(ts1.dur() + 7 * 24h).count()};
    std::cout << "Count: "<< ts3.count << "\n";
    std::cout << "UTC  : "<< ts3.utc_time_str() << "\n";
    std::cout << "Local: "<< ts3.local_time_str() << "\n";

    return 0;
}
