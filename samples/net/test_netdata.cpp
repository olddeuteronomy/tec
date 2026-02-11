// Time-stamp: <Last changed 2026-02-12 00:36:21 by magnolia>
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

#include <cstdio>
#include <iostream>

#include "tec/tec_dump.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_nd_compress.hpp"

#include "../test_data.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                               TESTS
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void print_data(const test::TestData& data) {
    std::cout << data << "\n";
}

void print_nd(const tec::NetData& nd) {
    std::cout
        << "HEADER ===================================\n"
        << "Magic:   " << std::hex << nd.header.magic << "\n"
        << "Version: " << nd.header.version << std::dec << "\n"
        << "ID:      " << nd.header.id << "\n"
        << "Size:    " << nd.header.size << "\n"
        << "Orig:    " << nd.header.size_uncompressed << "\n"
        // << "Bytes:   " << nd.bytes().size() << "\n"
        << "=========================================\n"
        ;
    std::cout << tec::dump::as_table(nd.bytes().as_hex()) << "\n";
}


int main() {
    test::TestData data;
    test::TestData::init(data);

    // Store data.
    tec::NetData nd_in;
    print_data(data);
    nd_in << data;
    print_nd(nd_in);

    // Compressing.
    tec::NdCompress comp(tec::CompressionParams::kCompressionZlib);
    comp.compress(nd_in);

    //
    // Emulate NetData transferring...
    //
    tec::NetData nd_out;
    nd_out.copy_from(nd_in);
    print_nd(nd_out);

    // Uncompressing.
    tec::NdCompress uncomp;
    uncomp.uncompress(nd_out);
    print_nd(nd_out);

    // Restore data.
    test::TestData data_out;
    nd_out.rewind(); // To be sure we're reading from the beginning.
    nd_out >> data_out;
    print_data(data_out);

    return 0;
}
