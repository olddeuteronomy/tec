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

#include <cstdio>
#include <iostream>
#include <ostream>
#include <string>
#include <sys/types.h>

#include "tec/net/tec_net_data.hpp"


struct Payload {
    int i32;
    unsigned long long u64;
    std::string str;
    float f;
    double d;
    tec::Bytes bs;
    bool b;

    void store(tec::NetData& data_out) {
        data_out
            << i32
            << u64
            << str
            << bs
            << f
            << d
            << b
            ;
    }

    void load(tec::NetData& data_in) {
        data_in
            >> &i32
            >> &u64
            >> &str
            >> &bs
            >> &f
            >> &d
            >> &b
            ;
    }

    void print(std::ostream& os) {
        os
            << "i32=" << i32 << "\n"
            << "u64=" << u64 << "\n"
            << "str='" << str << "'\n"
            << "f=  " << f << "\n"
            << "d=  " << d << "\n"
            << "bs= " << bs.data() << "\n"
            << "b=  " << b << "\n"
            ;
    }
};


int main() {
    char hello[] = "Hello!\0";

    Payload pld{32
                , 64
                , "This is a string"
                , 3.14f
                , 2.78
                , {hello, strlen(hello) + 1}
                , true
    };
    tec::NetData nd;

    std::cout << "-------- STORE\n";
    pld.print(std::cout);
    pld.store(nd);

    std::cout << "-------- LOAD\n";
    Payload pld2;
    nd.rewind();
    pld2.load(nd);
    pld2.print(std::cout);

    return 0;
}
