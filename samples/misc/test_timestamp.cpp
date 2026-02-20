// Time-stamp: <Last changed 2026-02-20 16:28:55 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/

#include <ctime>
#include <iostream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
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
