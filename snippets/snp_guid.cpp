//![guid]
#include "tec/tec_guid.hpp"

//...
for (int i = 0; i < 5; ++i) {
    std::cout << tec::guid::generate() << "\n";
}
// Possible output.
// 12fb0aa8-3370-4b65-9ca2-796c6ea69da4
// be532f6d-2eac-4ecf-b732-14f95403f339
// 0ff569e1-9154-4a21-9ad3-36468fa35776
// ca9b0354-ed1d-44a4-b4fa-4d94c94ab129
// 715140cc-53ca-4264-8053-5c35da1a8f46
//![guid]
