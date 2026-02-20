// Time-stamp: <Last changed 2026-02-20 16:31:09 by magnolia>
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

#include "../test_data.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*   Test tec::dump::as_table, tec::base64::encode, tec::base64::decode
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


int main() {
    tec::Bytes blob(test::test_blob, strlen(test::test_blob));
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
000000|0102030405060708090A0B0C0D0F A B C D E F G H I J K L M N O P Q R|
000032| S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x|
000064| y z 0 1 2 3 4 5 6 7 8 9 + /|

ENCODED:
offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000| A Q I D B A U G B w g J C g s M D Q 9 B Q k N E R U Z H S E l K|
000032| S 0 x N T k 9 Q U V J T V F V W V 1 h Z W m F i Y 2 R l Z m d o|
000064| a W p r b G 1 u b 3 B x c n N 0 d X Z 3 e H l 6 M D E y M z Q 1|
000096| N j c 4 O S s v|

DECODED:
offset|00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
======|++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--++--|
000000|0102030405060708090A0B0C0D0F A B C D E F G H I J K L M N O P Q R|
000032| S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x|
000064| y z 0 1 2 3 4 5 6 7 8 9 + /|

*/
