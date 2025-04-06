#include "tec/tec_print.hpp"
#include "tec/tec_status.hpp"
#include <string>

//! [OK]
tec::Status do_something1() {
    // ...
    // Everything is OK.
    return {};
}
//! [OK]

//! [Unspecified]
tec::Status do_something2() {
    // ...
    // IO error with unspecified error code [-1].
    return {tec::Error::Kind::IOErr};
}
//! [Unspecified]

//! [Description]
tec::Status do_something3() {
    // ...
    // IO error with description and unspecified error code [-1].
    return {"Cannot open a file", tec::Error::Kind::IOErr};
}
//! [Description]

//! [Errcode]
tec::Status do_something4() {
    // ...
    // Generic error with error code 200.
    return {200};
}
//! [Errcode]

//! [CD]
tec::Status do_something5() {
    std::string addr{"127.0.0.1"};
    // ...
    // Network error with code and description.
    return {404,
            tec::format("Cannot connect to {}", addr),
            tec::Error::Kind::NetErr};
}
//! [CD]
