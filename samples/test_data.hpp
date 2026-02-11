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

#pragma once

#include <list>
#include <unordered_map>

#include "tec/tec_memfile.hpp"
#include "tec/tec_serialize.hpp"
#include "tec/tec_json.hpp"
#include "tec/net/tec_net_data.hpp"


namespace test {


constexpr const char test_blob[]{
    "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0F"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\0"
};

constexpr const char test_string[]{
    "This is a UTF-8 string: 😀 Hello world!\0"
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*                             Persons
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct Person: tec::Serializable {
    using json = tec::Json;
    static constexpr const char* sep{tec::Json::sep};

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
        os << tec::Json{}(p);
        return os;
    }

    std::string to_json() const override {
        std::ostringstream os;
        os
            << json{}(age, "age") << sep
            << json{}(name, "name") << sep
            << json{}(surname, "surname")
            ;
        return os.str();
    }
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*          TestData (covers almost all serializable types)
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct TestData: tec::NdRoot {
    using json = tec::Json;
    static constexpr auto sep{tec::Json::sep};

    std::list<int> list;
    short i16;
    int i32;
    unsigned long long u64;
    std::string str;
    float f32;
    double d64;
    Person p;
    long double d128;
    tec::Blob blob;
    bool b;
    std::unordered_map<int, Person> map;

    TestData(): tec::NdRoot(0) {
        i16 = 0;
        i32 = 0;
        u64 = 0.0;
        f32 = 0.0f;
        d64 = 0.0;
        d128 = 0.0;
        b = false;
    }

    inline static void init(TestData& d) {
        // Fill test data.
        std::unordered_map<int,Person>
            persons{
            {1256, {31, "Mary", "Smith"}},
            {78, {39, "Harry", "Long"}},
            {375, {67, "Kevin", "Longsdale"}},
        };

        d.list = {1, 2, 3, 4};
        d.i16 = 16;
        d.i32 = 32;
        d.u64 = 1767623391515;
        d.str = test_string;
        d.f32 = 3.14f;
        d.d64 = 2.78;
        d.p = {61, "John", "Dow"};
        d.d128 = 1.123456e102;
        d.blob = {test_blob, strlen(test_blob)};
        d.b = true;
        d.map = persons;
    }

    tec::NetData& store(tec::NetData& nd) const override {
      nd
          << list
          << i16
          << i32
          << u64
          << str
          << f32
          << d64
          << p
          << d128
          << blob
          << b
          << map
          ;
      return nd;
    }

    tec::NetData& load(tec::NetData& nd) override {
        nd
            >> list
            >> i16
            >> i32
            >> u64
            >> str
            >> f32
            >> d64
            >> p
            >> d128
            >> blob
            >> b
            >> map
            ;
        return nd;
    }


    friend std::ostream& operator << (std::ostream& os, const TestData& p) {
        os << tec::Json{}(p);
        return os;
    }

    std::string to_json() const override {
        std::ostringstream os;
        os
            << json{}(list, "list") << sep
            << json{}(i16, "i16") << sep
            << json{}(i32, "i32") << sep
            << json{}(u64, "u64") << sep
            << json{}(str, "str") << sep
            << json{}(f32, "f32") << sep
            << json{}(d64, "d64") << sep
            << json{}(p, "person") << sep
            << json{}(d128, "d128") << sep
            << json{}(blob, "bytes") << sep
            << json{}(b, "b") << sep
            << json{}(map, "persons")
            ;
        return os.str();
    }
};




/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         GetPersons RPC structs
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//
// Request
//
struct GetPersonsIn: public tec::NdRoot {
    using json = tec::Json;
    static constexpr auto sep{tec::Json::sep};

    int max_count;

    GetPersonsIn()
        : tec::NdRoot(1)  // Request ID=1
        , max_count{0}       // Get all records.
        {}

    tec::NetData& store(tec::NetData& nd) const override {
        return nd << max_count;
    }

    tec::NetData& load(tec::NetData& nd) override {
        return nd >> max_count;
    }

    friend std::ostream& operator << (std::ostream& os, const GetPersonsIn& p) {
        os << json{}(p);
        return os;
    }

    std::string to_json() const override {
        std::ostringstream os;
        os
            << json{}(max_count, "max_count")
            ;
        return os.str();
    }
};

//
// Reply
//
struct GetPersonsOut: public tec::NdRoot {
    using json = tec::Json;
    static constexpr auto sep{tec::Json::sep};

    std::list<Person> persons;

    GetPersonsOut()
        : tec::NdRoot(1) // Reply ID=1 (Reply ID MUST BE EQUAL Request ID).
    {}

     tec::NetData& store(tec::NetData& nd) const override {
        return nd << persons;
    }

    tec::NetData& load(tec::NetData& nd) override {
        return nd >> persons;
    }

    friend std::ostream& operator << (std::ostream& os, const GetPersonsOut& p) {
        os << json{}(p);
        return os;
    }

    std::string to_json() const override {
        std::ostringstream os;
        os
            << json{}(persons, "persons")
            ;
        return os.str();
    }
};

} // namespace test
