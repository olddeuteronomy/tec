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

#include "tec/tec_dump.hpp"
#include "tec/net/tec_net_data.hpp"


std::ostream& operator << (std::ostream& os, const std::list<int>& c) {
    bool first{true};
    for( const auto& e: c ) {
        if( first ) {
            os << "[\n";
            first = false;
        } else {
            os << ",\n";
        }
        os << "\t" << e;
    }
    os << "\n]";
    return os;
}


struct Person {
    constexpr const bool serializable() { return true; }

    std::string name;
    std::string surname;
    short age;

    tec::NetData& store(tec::NetData& nd) const {
        nd
            << age
            << name
            << surname
            ;
        return nd;
    }

    tec::NetData& load(tec::NetData& nd) {
        nd
            >> &age
            >> &name
            >> &surname
            ;
        return nd;
    }
};


struct Payload {
    constexpr const bool serializable() { return true; }

    std::list<int> list;
    int i32;
    unsigned long long u64;
    std::string str;
    float f32;
    double d64;
    Person p;
    long double d128;
    tec::Bytes bs;
    bool b;

    void store(tec::NetData& nd) const {
      nd
          << list
          << i32
          << u64
          << str
          << f32
          << d64
          << p
          << d128
          << bs
          << b
          ;
    }

    void load(tec::NetData& nd) {
        nd
            >> &list
            >> &i32
            >> &u64
            >> &str
            >> &f32
            >> &d64
            >> &p
            >> &d128
            >> &bs
            >> &b
            ;
    }

    void dump(std::ostream& os) {
        os
            << "list=" << list << "\n"
            << "i32= " << i32 << "\n"
            << "u64= " << u64 << "\n"
            << "str= '" << str << "'\n"
            << "f32= " << f32 << "\n"
            << "d64= " << d64 << "\n"
            << "p  = " << "{\"" << p.name << " " << p.surname << "\", " << p.age << "}\n"
            << "d128=" << d128 << "\n"
            << "bs=  " << bs.data() << "\n"
            << "b=   " << b << "\n"
            ;
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           TEST
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void save_payload(Payload& pld, tec::NetData& nd) {
    nd << pld;

    std::cout << "\n-------- STORE --------\n";
    pld.dump(std::cout);
    std::cout << std::string(23, '-') << "\nTotal size=" << nd.size() << "\n";

}

void restore_payload(tec::NetData& nd) {
    Payload pld;
    nd.rewind();
    nd >> &pld;

    std::cout << "\n-------- LOAD ---------\n";
    pld.dump(std::cout);

    nd.rewind();

    // Header
    tec::NetData::Header hdr = nd.header();
    std::cout
        << "\nMagic:   " << std::hex << hdr.magic << std::dec
        << "\nVersion: " << hdr.version
        << "\nSize:    " << hdr.size
        << "\n";

    // Dump
    tec::Dump::print<>(std::cout, (const char*)nd.data(), nd.size());
    std::cout << std::string(72, '-') << "\nTotal size (w/header)=" << nd.total_size() << "\n";
}


int main() {
    char hello[] = "Hello!\0";
    tec::NetData nd;

    Payload pld {
        {1, 2, 3, 4}
        , 32
        , 64
        , "This is a string"
        , 3.14f
        , 2.78
        , {"John", "Dow", 61}
        , 1.123456e102
        , {hello, strlen(hello) + 1}
        , true
    };

    save_payload(pld, nd);
    restore_payload(nd);

    return 0;
}
