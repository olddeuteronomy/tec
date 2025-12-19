// Time-stamp: <Last changed 2025-12-18 15:30:21 by magnolia>
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

/**
 * @file tec_dump.hpp
 * @brief A byte buffer class with stream-like read/write semantics.
 * @author The Emacs Cat
 * @date 2025-12-17
 */

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <ostream>
#include <string>


namespace tec {

struct Dump {

    static std::string dump_as_table(const std::string& s) {
        constexpr size_t bytes_per_line = 32;
        constexpr size_t chars_per_line = 2 * bytes_per_line;
        std::ostringstream os;
        // Header: decimal column numbers, 2 digits, padded with 0
        os << "offset|";
        for (size_t i = 0; i < bytes_per_line; i += 2) {
            os << std::setw(2) << std::setfill('0') << i << "  ";
        }
        os << '\n';
        // Separator line.
        os << "======|";
        for (size_t i = 0; i < bytes_per_line / 2; ++i) {
            os << "++--";
        }
        // Print bytes as 2 characters.
        for(size_t n = 0 ; n < s.size() ; n += 2) {
            if((n % chars_per_line) == 0) {
                os << "|\n"
                   << std::dec << std::setw(6) << std::setfill('0')
                   << n / 2 << "|";
            }
            os << s[n] << s[n+1];
        }
        os << "|";
        return os.str();
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
        "Non-printable bytes are shown in hex: \x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x1A\x1B\x1C\x1D\x1E\x1F"
        "\xA1\xA2\xA3\xA4\xA5\xF0\xFF"
        "\x00"
        ;

    tec::Bytes b(data, strlen(data));
    std::cout << tec::Dump::dump_as_table(b) << "\n";
    return 0;
}

OUTPUT:

offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000032| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000064| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000096|20 h e x :200102030405060708090A0B0C0D0E0F1A1B1C1D1E1FA1A2A3A4A5|
000128|F0FF|

 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
