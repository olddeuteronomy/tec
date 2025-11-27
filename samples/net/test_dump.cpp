
#include "tec/tec_dump.hpp"


int main() {
    const char data[] =
        "Hello world! This is a dump of the sequence of printable bytes. "
        "Non-printable bytes are shown in hex: \x00\x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x1A\x1B\x1C\x1D\x1E\x1F";

    tec::Dump::print(std::cout, data, sizeof(data) - 1);
    return 0;
}

/* OUTPUT

offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000020| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000040| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000060|20 h e x :20000102030405060708090a0b0c0d0e0f1a1b1c1d1e1f        |

*/
