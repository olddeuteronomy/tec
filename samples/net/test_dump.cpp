
#include <iostream>

#include "tec/tec_dump.hpp"
#include "tec/tec_memfile.hpp"
#include "tec/tec_base64.hpp"


int main() {
    const char data[] =
        "Hello world! This is a dump of the sequence of printable bytes. "
        "Non-printable bytes are shown in hex: \x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x1A\x1B\x1C\x1D\x1E\x1F"
        "\xA1\xA2\xA3\xA4\xA5\xF0\xFF"
        "\x00"
        ;

    tec::MemFile b(data, strlen(data)+1);
    std::cout
        << "Size:" << b.size() << "\n"
        << "Cap: " << b.capacity() << "\n"
        << "Pos: " << b.tell() << "\n\n"
        ;
    std::cout << tec::dump::as_table(b.as_hex()) << "\n";
    std::cout
        << "Base64 decoded:\n"
        << "\""
        << tec::base64::to_base64(data)
        << "\"\n"
        ;
    return 0;
}

/* OUTPUT

Size:131
Cap: 8192
Pos: 131

offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000032| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000064| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000096|20 h e x :200102030405060708090A0B0C0D0E0F1A1B1C1D1E1FA1A2A3A4A5|
000128|F0FF00|

*/
