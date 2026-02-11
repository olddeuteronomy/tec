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

#include <cstdlib>
#include <iostream>

#include "tec/tec_dump.hpp"
#include "tec/tec_memfile.hpp"
#include "tec/tec_base64.hpp"


int main() {
    const char data[] =
        "Hello world! This is a dump of the sequence of printable bytes. "
        "Non-printable bytes are shown in hex: "
        "\x01\x02\x03\x04\x05\x06\x07"
        "\x08\x09\x0A\x0B\x0C\x0D\x0E"
        "\x0F\x1A\x1B\x1C\x1D\x1E\x1F"
        "\xA1\xA2\xA3\xA4\xA5\xF0\xFF"
        "\x00"
        ;

    tec::Bytes blob(data, strlen(data));
    std::cout << "ORIGIN:\n"
        << tec::dump::as_table(blob.as_hex()) << "\n";

    tec::Bytes blob_encoded(tec::base64::encode(blob.str()));
    std::cout << "\nENCODED:\n"
        << tec::dump::as_table(blob_encoded.as_hex()) << "\n";

    if (!tec::base64::is_valid(blob_encoded.str())) {
        std::cerr << "Error: not valid encoded data\n";
        std::exit(1);
    }

    auto decoded = tec::base64::decode(blob_encoded.str());
    tec::Bytes blob_decoded(decoded.data(), decoded.size());
    std::cout << "\nDECODED:\n"
        << tec::dump::as_table(blob_decoded.as_hex()) << "\n";

    return 0;
}

/* OUTPUT

ORIGIN:
offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000032| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000064| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000096|20 h e x :200102030405060708090A0B0C0D0E0F1A1B1C1D1E1FA1A2A3A4A5|
000128|F0FF|

ENCODED:
offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| S G V s b G 8 g d 2 9 y b G Q h I F R o a X M g a X M g Y S B k|
000032| d W 1 w I G 9 m I H R o Z S B z Z X F 1 Z W 5 j Z S B v Z i B w|
000064| c m l u d G F i b G U g Y n l 0 Z X M u I E 5 v b i 1 w c m l u|
000096| d G F i b G U g Y n l 0 Z X M g Y X J l I H N o b 3 d u I G l u|
000128| I G h l e D o g A Q I D B A U G B w g J C g s M D Q 4 P G h s c|
000160| H R 4 f o a K j p K X w / w = =|

DECODED:
offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| H e l l o20 w o r l d !20 T h i s20 i s20 a20 d u m p20 o f20 t|
000032| h e20 s e q u e n c e20 o f20 p r i n t a b l e20 b y t e s .20|
000064| N o n - p r i n t a b l e20 b y t e s20 a r e20 s h o w n20 i n|
000096|20 h e x :200102030405060708090A0B0C0D0E0F1A1B1C1D1E1FA1A2A3A4A5|
000128|F0FF|

*/
