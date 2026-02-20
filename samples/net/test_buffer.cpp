// Time-stamp: <Last changed 2026-02-20 16:30:48 by magnolia>
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

#include <iostream>

#include "tec/tec_dump.hpp"
#include "tec/tec_memfile.hpp"


void print_buffer(int n, const tec::Blob& buf) {
    std::cout
        << "\n"
        << n << ") "
        << std::string(70, '=')
        << "\n"
        << "Blk=" << buf.block_size() << "\n"
        << "Cap=" << buf.capacity() << "\n"
        << "Siz=" << buf.size() << "\n"
        << "Pos=" << buf.tell() << "\n"
        << tec::dump::as_table(buf.as_hex())
        << "\n"
        ;
}


int main() {
    tec::Blob buf(4);
    // tec::Blob buf;

    // 1)
    print_buffer(1, buf);

    // 2)
    unsigned int32{1234};
    buf.write(&int32, sizeof(unsigned));
    print_buffer(2, buf);

    // 3)
    const char str[] = "Hello, world!";
    buf.write(str, strlen(str));
    print_buffer(3, buf);

    // 4)
    buf.rewind();
    print_buffer(4, buf);

    // 5)
    unsigned int32a{0};
    buf.read(&int32a, sizeof(unsigned));
    std::cout << int32a << "\n";
    print_buffer(5, buf);

    // 6)
    int len = strlen(str);
    char s[BUFSIZ];
    buf.read(s, len);
    s[len] = '\0';
    std::cout << s << "\n";
    print_buffer(6, buf);

    // 7)
    buf.seek(0, SEEK_SET);
    print_buffer(7, buf);
    unsigned int32b{0};
    buf.read(&int32b, sizeof(unsigned));
    std::cout << int32b << "\n";
    print_buffer(7, buf);

    // 8)
    buf.seek(0, SEEK_END);
    print_buffer(8, buf);
    buf.seek(-13, SEEK_CUR);
    print_buffer(8, buf);
    char s2[BUFSIZ];
    buf.read(s2, len);
    s2[len] = '\0';
    std::cout << s2 << "\n";
    print_buffer(8, buf);

    return 0;
}
