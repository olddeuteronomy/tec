#include "tec/tec_serialize.hpp"
#include "tec/tec_json.hpp"

//! [person]
struct Person: tec::Serializable {
    using json = tec::Json;
    static constexpr auto sep{tec::Json::sep};

    short age;
    std::string name;
    std::string surname;

    friend std::ostream& operator << (std::ostream& os, const Person& p) {
        os << json{}(p);
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
//! [person]

