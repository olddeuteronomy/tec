// Time-stamp: <Last changed 2025-11-27 13:47:25 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2025 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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

#pragma once

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <cctype>


namespace tec {

struct Dump {

    template <typename TStream>
    static void print(TStream& os, const char* dst, size_t length) {
        constexpr size_t bytes_per_line = 32;

        // Header: decimal column numbers, 2 digits, padded with 0
        os << "offset|";
        for (size_t i = 0; i < bytes_per_line; i += 2) {
            os << std::setw(2) << std::setfill('0') << i << "  ";
        }
        os << '\n';

        // Separator line.
        os << "======|";
        for (size_t i = 0 ; i < bytes_per_line / 2 ; ++i) {
            os << "++--";
        }
        os << "|\n";

        const char* byte = dst;
        char fill[3]{' ', ' ', '\0'};

        // For all rows.
        for (size_t base = 0; base < length; base += bytes_per_line) {
            size_t n = std::min(bytes_per_line, length - base);

            // Offset: 6-digit decimal
            os << std::dec << std::setw(6) << std::setfill('0') << base << "|";

            // Dump all columns.
            for (size_t col = 0; col < n; ++col) {
                int c = *byte++ & 0xFF;
                if (32 < c && c < 127) {
                    fill[1] = c;
                    os << std::setw(2) << std::setfill(' ');
                    os << fill;
                }
                else {
                    os << std::hex << std::setw(2) << std::setfill('0')
                       << c;
                }
            }

            // Pad incomplete line.
            for (size_t i = n; i < bytes_per_line; ++i) {
                os << "  ";
            }

            os << "|\n";
        }

        // Restore defaults.
        os << std::dec << std::setfill(' ');
    }

};

} // namespace tec

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           USAGE
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main() {
    const char data[] =
        "Hello world! This is a dump of the sequence of printable bytes. "
        "Non-printable bytes are shown in hex: \x00\x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x1A\x1B\x1C\x1D\x1E\x1F";

    dump(std::cout, data, sizeof(data) - 1);
    return 0;
}

OUTPUT:

offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000020| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000040| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000060|20 h e x :20000102030405060708090a0b0c0d0e0f1a1b1c1d1e1f        |

 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
