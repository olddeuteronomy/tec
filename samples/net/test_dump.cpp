
#include <iostream>

#include "tec/tec_bytes.hpp"
#include "tec/tec_dump.hpp"


int main() {
    const char data[] =
        "Hello world! This is a dump of the sequence of printable bytes. "
        "Non-printable bytes are shown in hex: \x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x1A\x1B\x1C\x1D\x1E\x1F"
        "\xA1\xA2\xA3\xA4\xA5\xF0\xFF"
        "\x00"
        ;

    tec::Bytes b(data, strlen(data));
    std::cout
        << "size=" << b.size() << "\n"
        << "capacity=" << b.capacity() << "\n"
        << "pos=" << b.tell() << "\n"
        ;
    std::cout << b.as_hex() << "\n";
    std::cout << tec::Dump::dump_as_table(b) << "\n";
    return 0;
}

/* OUTPUT

offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000032| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000064| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000096|20 h e x :200102030405060708090A0B0C0D0E0F1A1B1C1D1E1FA1A2A3A4A5|
000128|F0FF|

*/
