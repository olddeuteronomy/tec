// Time-stamp: <Last changed 2026-02-11 15:23:51 by magnolia>
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
 * @file tec_base64.hpp
 * @brief A header-only Base64 encoder/decoder (C++17+).
 * @author The Emacs Cat
 * @date 2026-01-04
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <cstdint>


namespace tec {

namespace base64 {

inline constexpr std::string_view chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Build a lookup table for decoding at compile-time.
 */
static constexpr std::array<int, 256> build_decode_table() {
    std::array<int, 256> table{};
    for (size_t i = 0; i < 256; ++i) {
        table[i] = -1;
    }
    for (size_t i = 0; i < chars.size(); ++i) {
        table[static_cast<uint8_t>(chars[i])] = static_cast<int>(i);
    }
    return table;
}

static constexpr auto decode_table = build_decode_table();


/**
 * Validates whether a string is a properly formatted Base64 string.
 */
inline bool is_valid(std::string_view data) {
    if (data.empty() || data.size() % 4 != 0) return false;

    size_t size = data.size();
    size_t padding = 0;

    for (size_t i = 0; i < size; ++i) {
        unsigned char c = static_cast<uint8_t>(data[i]);

        if (c == '=') {
            padding++;
            // Check if '=' is at the end and total padding <= 2
            if (i < size - 2 || (i == size - 2 && data[size - 1] != '='))
                return false;
            if (padding > 2)
                return false;
            continue;
        }

        // No characters allowed after padding started
        if (padding > 0)
            return false;

        // Check against lookup table
        if (decode_table[c] == -1)
            return false;
    }
    return true;
}


/**
 * Encodes raw binary data into a Base64 string.
 */
inline std::string encode(std::string_view data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}


/**
 * Decodes a Base64 string into a vector of bytes.
 * Ignores non-alphabet characters (like newlines) if is_valid is not used.
 */
inline std::vector<uint8_t> decode(std::string_view data) {
    std::vector<uint8_t> out;
    out.reserve((data.size() / 4) * 3);
    int val = 0;
    int valb = -8;
    for (char c : data) {
        if (c == '=')
            break;
        int v = decode_table[static_cast<uint8_t>(c)];
        if (v == -1)
            continue; // Skip newlines.
        val = (val << 6) + v;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

} // namespace base64

} // namespace tec
