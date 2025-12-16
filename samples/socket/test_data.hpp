
#pragma once

#include <cstdio>
#include <iostream>
#include <ostream>
#include <string>
#include <sstream>
#include <list>
#include <unordered_map>

#include "tec/tec_serialize.hpp"
#include "tec/tec_json.hpp"
#include "tec/net/tec_net_data.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             Person
*
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
*                         Payload
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct Payload: tec::NdRoot {
    using json = tec::Json;
    static constexpr auto sep{tec::Json::sep};

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

    Payload(): tec::NdRoot(0)
    {}

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
        os << tec::Json{}(p);
        return os;
    }

    std::string to_json() const override {
        std::ostringstream os;
        os
            << json{}(list, "list") << sep
            << json{}(i32, "i32") << sep
            << json{}(u64, "u64") << sep
            << json{}(str, "str") << sep
            << json{}(f32, "f32") << sep
            << json{}(d64, "d64") << sep
            << json{}(p, "person") << sep
            << json{}(d128, "d128") << sep
            << json{}(bs, "bytes") << sep
            << json{}(b, "b") << sep
            << json{}(map, "persons")
            ;
        return os.str();
    }
};

