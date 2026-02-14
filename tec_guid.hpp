// Time-stamp: <Last changed 2026-02-14 13:05:15 by magnolia>
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

/**
 *   @file tec_guid.hpp
 *   @brief GUID generation utilities.
 *   @date 2026-01-17
 */

#pragma once

#include <random>
#include <array>
#include <cstdint>
#include <string>
# include <iomanip>
#include <sstream>
#include <cstring>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/// 16-byte GUID
using uuid_t = std::array<std::uint8_t, 16>;

namespace guid {

namespace details {

/// Thread-local randomness.
struct randgen {
    static std::mt19937_64& get_rng() {
        thread_local static std::mt19937_64 rng__{std::random_device{}()};
        return rng__;
    }
}; // randgen

} // namespace details


/**
 * @brief Generates 16-byte uuid_t version 4.
 */
inline uuid_t generate_v4()
{
    uuid_t uuid;
    //
    // Fill with cryptographically strong random bytes if possible,
    // otherwise high-quality pseudo-random bytes.
    //
    std::uniform_int_distribution<std::uint8_t> dist{0, 255};
    for (auto& byte : uuid) {
        byte = dist(details::randgen::get_rng());
    }
    //
    // Set version (4) --> bits 12-15 of byte 6
    //
    uuid[6] = (uuid[6] & 0x0F) | 0x40;   // version 4
    //
    // Set variant (RFC 4122) --> bits 6-7 of byte 8 = 10
    //
    uuid[8] = (uuid[8] & 0x3F) | 0x80;   // variant 1 (10xx xxxx)
    return uuid;
}

inline std::ostream& operator<<(std::ostream& os, const uuid_t& uuid) {
    auto flags = os.flags();
    os << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) os << '-';
        os << std::setw(2) << static_cast<unsigned>(uuid[i]);
    }
    os.flags(flags);  // restore original flags
    return os;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Formatting helpers
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief Convert GUID to hex string (in lowercase).
 */
inline std::string to_string(const uuid_t& uuid)
{
    std::ostringstream os;
    os << uuid;
    std::string result = os.str();
    return result;
}

/**
 * @brief Generate new GUID as hex string (in lowercase).
 * @snippet snp_guid.cpp guid
 */
inline std::string generate()
{
    return to_string(generate_v4());
}


} // namespace guid

} // namespace tec


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Quick usage examples
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
int main()
{
    for (int i = 0; i < 5; ++i)
    {
        std::cout << tec::guid::generate() << "\n";
    }

    // Example outputs:
    // 8f4e2b1a-7d9c-4e3f-9a2b-c5d8e1f60789
    // d7a94f12-3b6e-41c5-8f2d-9e0c1a5b6789
    // ...
}
*/
