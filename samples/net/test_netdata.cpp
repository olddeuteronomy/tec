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
#include <list>
#include <unordered_map>

#include "tec/tec_dump.hpp"
#include "tec/net/tec_net_data.hpp"


template <typename T>
std::ostream& operator << (std::ostream& os, const std::list<T>& c) {
    os << "[";
    for( const auto& e: c ) {
        os << "\n\t" << e;
    }
    os << "\n]";
    return os;
}

template <typename K, typename V>
std::ostream& operator << (std::ostream& os, const std::unordered_map<K, V>& m) {
    os << "(";
    for( const auto& [k, v]: m) {
        os << "\n\t" << k << ": " << v;
    }
    os << "\n)";
    return os;
}


struct Person {
    constexpr const bool serializable() { return true; }

    short age;
    std::string name;
    std::string surname;

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

    friend std::ostream& operator << (std::ostream& os, const Person& p) {
        os
            << "{"
            << "\n\t" << p.age
            << ",\n\t\"" << p.name << "\""
            << ",\n\t\"" << p.surname << "\""
            << "\n}\n";
        return os;
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
    std::unordered_map<int, Person> map;

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
          << map
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
            >> &map
            ;
    }

    friend std::ostream& operator <<(std::ostream& os, const Payload& pld) {
        os
            << "list=" << pld.list << "\n"
            << "i32= " << pld.i32 << "\n"
            << "u64= " << pld.u64 << "\n"
            << "str= '" << pld.str << "'\n"
            << "f32= " << pld.f32 << "\n"
            << "d64= " << pld.d64 << "\n"
            << "Person=" << pld.p
            << "d128=" << pld.d128 << "\n"
            << "bs=  " << pld.bs.data() << "\n"
            << "b=   " << pld.b << "\n"
            << "Persons= " << pld.map << "\n"
            ;
        return os;
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
    std::cout << pld;
    std::cout << std::string(23, '-') << "\nsize=" << nd.size() << "\n";

}

void restore_payload(tec::NetData& nd) {
    Payload pld;
    nd.rewind();
    nd >> &pld;

    std::cout << "\n-------- LOAD ---------\n";
    std::cout << pld;

    nd.rewind();

    // Header
    tec::NetData::Header hdr = nd.header();
    std::cout
        << "\nMagic:   " << std::hex << hdr.magic
        << "\nVersion: " << hdr.version << std::dec
        << "\nSize:    " << hdr.size
        << "\n";

    // Dump
    tec::Dump::print(std::cout, static_cast<const char*>(nd.data()), nd.size());
    std::cout << std::string(72, '-') << "\nTotal size (w/header)=" << nd.total_size() << "\n";
}


int main() {
    std::cout << "Size of bool=" << sizeof(bool) << "\n";
    std::cout << "Size of float=" << sizeof(float) << "\n";
    std::cout << "Size of double=" << sizeof(double) << "\n";
    std::cout << "Size of long double=" << sizeof(long double) << "\n\n";

    char hello[] = "Hello!\0";
    std::unordered_map<int, Person> persons = {
        {1256, {31, "John", "Smith"}},
        {78, {39, "Harry", "Long"}},
        {375, {67, "Kevin", "Longsdale"}},
    };

    tec::NetData nd;

    Payload pld {
        {1, 2, 3, 4}
        , 32
        , 64
        , "This is a string"
        , 3.14f
        , 2.78
        , {61, "John", "Dow"}
        , 1.123456e102
        , {hello, strlen(hello) + 1}
        , true
        , persons
    };

    save_payload(pld, nd);
    restore_payload(nd);

    return 0;
}
