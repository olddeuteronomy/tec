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
#include <sstream>
#include <list>
#include <unordered_map>

#include "tec/tec_dump.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/tec_serialize.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             Person
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct Person: tec::Serializable {
    Person() {}
    Person(short _age, const char* _name, const char* _surname)
        : age{_age}
        , name{_name}
        , surname{_surname}
    {}

    short age;
    std::string name;
    std::string surname;

    tec::NetData& store(tec::NetData& nd) const override {
        nd
            << age
            << name
            << surname
            ;
        return nd;
    }

    tec::NetData& load(tec::NetData& nd) override {
        nd
            >> age
            >> name
            >> surname
            ;
        return nd;
    }

    friend std::ostream& operator << (std::ostream& os, const Person& p) {
        os << p.json(p);
        return os;
    }

    std::string to_json() const {
        std::ostringstream os;
        os
            << this->json(age, "age") << sep
            << this->json(name, "name") << sep
            << this->json(surname, "surname")
            ;
        return os.str();
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Payload
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct Payload: tec::Serializable {

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

    tec::NetData& store(tec::NetData& nd) const override {
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
      return nd;
    }

    tec::NetData& load(tec::NetData& nd) override {
        nd
            >> list
            >> i32
            >> u64
            >> str
            >> f32
            >> d64
            >> p
            >> d128
            >> bs
            >> b
            >> map
            ;
        return nd;
    }


    friend std::ostream& operator << (std::ostream& os, const Payload& p) {
        os << p.json(p);
        return os;
    }

    std::string to_json() const {
        std::ostringstream os;
        os
            << this->json_container(list, "list") << sep
            << this->json(i32, "i32") << sep
            << this->json(u64, "u64") << sep
            << this->json(str, "str") << sep
            << this->json(f32, "f32") << sep
            << this->json(d64, "d64") << sep
            << this->json(p, "person") << sep
            << this->json(d128, "d128") << sep
            // << "bs: " << this->bs.data() << ", "
            << this->json(b, "b") << sep
            << this->json(map, "persons")
            ;
        return os.str();
    }
};



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                               TEST
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void save_payload(const Payload& pld, tec::NetData& nd) {
    nd << pld;

    std::cout << "\n-------- STORE --------\n";
    std::cout << pld << "\n";
    std::cout << std::string(23, '-') << "\nsize=" << nd.size() << "\n";

}

void restore_payload(tec::NetData& nd) {
    Payload pld;
    nd.rewind();
    nd >> pld;

    std::cout << "\n-------- LOAD ---------\n";
    std::cout << pld << "\n";

    // Header
    tec::NetData::Header hdr = nd.header();
    std::cout
        << "\nMagic:   " << std::hex << hdr.magic
        << "\nVersion: " << hdr.version << std::dec
        << "\nSize:    " << hdr.size
        << "\n";

    // TODO: valgrind emitted errors in tec::Dump::print!
    std::cout << tec::Dump::dump_as_table(nd.bytes()) << "\n";
    std::cout << std::string(72, '-')
              << "\nTotal size (w/header)=" << nd.total_size() << "\n";
}


int main() {
    tec::NetData nd;

    char hello[] = "Hello!\0";
    std::unordered_map<int, Person> persons = {
        {1256, {31, "Mary", "Smith"}},
        {78, {39, "Harry", "Long"}},
        {375, {67, "Kevin", "Longsdale"}},
    };

    Payload pld;
    pld.list = {1, 2, 3, 4};
    pld.i32 = 32;
    pld.u64 = 1767623391515;
    pld.str = "This is a string";
    pld.f32 = 3.14f;
    pld.d64 = 2.78;
    pld.p = {61, "John", "Dow"};
    pld.d128 = 1.123456e102;
    pld.bs = {hello, strlen(hello) + 1};
    pld.b = true;
    pld.map = persons;

    save_payload(pld, nd);
    restore_payload(nd);

    return 0;
}
