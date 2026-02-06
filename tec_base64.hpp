// Time-stamp: <Last changed 2026-02-06 14:11:14 by magnolia>
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
#include <cstdint>

#if __cplusplus >= 202002L
#include <bit>
#endif


namespace tec {


static constexpr char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


inline std::string to_base64(std::string_view data) {
    std::string result;
    std::size_t full_groups = data.size() / 3;
    std::size_t remainder = data.size() % 3;

    result.reserve((data.size() + 2) / 3 * 4);  // Pre-allocate exact size
    const uint8_t* in = reinterpret_cast<const uint8_t*>(data.data());

    // Process full 3-byte groups
    for (std::size_t i = 0; i < full_groups; ++i) {
        uint32_t triplet = (in[0] << 16) | (in[1] << 8) | in[2];

#if __cplusplus >= 202002L
        // Use std::bit_cast for modern compilers (faster and clearer)
        result += b64_alphabet[std::bit_cast<uint8_t>((triplet >> 18) & 0x3F)];
        result += b64_alphabet[std::bit_cast<uint8_t>((triplet >> 12) & 0x3F)];
        result += b64_alphabet[std::bit_cast<uint8_t>((triplet >> 6)  & 0x3F)];
        result += b64_alphabet[std::bit_cast<uint8_t>(triplet & 0x3F)];
#else
        result += b64_alphabet[(triplet >> 18) & 0x3F];
        result += b64_alphabet[(triplet >> 12) & 0x3F];
        result += b64_alphabet[(triplet >> 6)  & 0x3F];
        result += b64_alphabet[triplet & 0x3F];
#endif
        in += 3;
    }

    // Handle remainder
    if (remainder == 1) {
        uint32_t triplet = in[0] << 16;
        result += b64_alphabet[(triplet >> 18) & 0x3F];
        result += b64_alphabet[(triplet >> 12) & 0x3F];
        result += "==";
    } else if (remainder == 2) {
        uint32_t triplet = (in[0] << 16) | (in[1] << 8);
        result += b64_alphabet[(triplet >> 18) & 0x3F];
        result += b64_alphabet[(triplet >> 12) & 0x3F];
        result += b64_alphabet[(triplet >> 6)  & 0x3F];
        result += '=';
    }

    return result;
}


inline std::string from_base64(std::string_view encoded) {
    if (encoded.empty()) return {};

    // Remove padding
    std::size_t padding = 0;
    if (encoded.back() == '=') {
        ++padding;
        if (encoded.size() > 1 && encoded[encoded.size() - 2] == '=') ++padding;
    }
    std::size_t valid_len = encoded.size() - padding;

    if (valid_len % 4 != 0) {
        return {};
    }

    std::string result;
    result.reserve((valid_len * 3) / 4);

    // Lookup table for decoding
    static constexpr int8_t decode_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 0-15
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 16-31
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  // 32-47
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  // 48-63
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  // 64-79
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  // 80-95
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  // 96-111
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  // 112-127
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 128-255 (rest -1)
        // ... (initialize rest to -1 if needed, but above covers ASCII)
    };

    for (std::size_t i = 0; i < valid_len; i += 4) {
        int32_t sextet_a = decode_table[static_cast<uint8_t>(encoded[i])];
        int32_t sextet_b = decode_table[static_cast<uint8_t>(encoded[i+1])];
        int32_t sextet_c = decode_table[static_cast<uint8_t>(encoded[i+2])];
        int32_t sextet_d = decode_table[static_cast<uint8_t>(encoded[i+3])];

        if (sextet_a == -1 || sextet_b == -1 || sextet_c == -1 || sextet_d == -1) {
            return {};
        }

        uint32_t triplet = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        result += static_cast<char>((triplet >> 16) & 0xFF);
        if (i + 2 < valid_len) result += static_cast<char>((triplet >> 8) & 0xFF);
        if (i + 3 < valid_len) result += static_cast<char>(triplet & 0xFF);
    }

    return result;
}

} // namespace tec
