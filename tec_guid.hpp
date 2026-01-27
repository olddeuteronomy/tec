// Time-stamp: <Last changed 2026-01-28 00:14:42 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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

/// Thread-local randomness (recommended).
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
 * @brief Convert GUID to hex string (in lowercase, by default).
 */
inline std::string to_string(const uuid_t& uuid, bool uppercase = false)
{
    std::ostringstream os;
    os << uuid;
    std::string result = os.str();
    // If we need uppercase.
    if (uppercase) {
        for (char& c : result) {
            if (c >= 'a' && c <= 'f') c -= 32;
        }
    }
    return result;
}

/**
 * @brief Generate new GUID as hex string (in lowercase, by default).
 * @snippet snp_guid.cpp guid
 */
inline std::string generate(bool uppercase = false)
{
    return to_string(generate_v4(), uppercase);
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
