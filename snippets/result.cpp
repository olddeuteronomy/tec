#include "tec/tec_print.hpp"
#include "tec/tec_result.hpp"
#include <string>

//! [OK]
tec::Result do_something1() {
    // ...
    // Everything is OK.
    return {};
}
//! [OK]

//! [Unspecified]
tec::Result do_something2() {
    // ...
    // IO error with unspecified error code [-1].
    return {tec::Error::Kind::IOErr};
}
//! [Unspecified]

//! [Description]
tec::Result do_something3() {
    // ...
    // IO error with description and unspecified error code [-1].
    return {"Cannot open a file", tec::Error::Kind::IOErr};
}
//! [Description]

//! [Errcode]
tec::Result do_something4() {
    // ...
    // Generic error with error code 200.
    return {200};
}
//! [Errcode]

//! [CD]
tec::Result do_something5() {
    std::string addr{"127.0.0.1"};
    // ...
    // Network error with code and description.
    return {404,
            tec::format("Cannot connect to {}", addr),
            tec::Error::Kind::NetErr};
}
//! [CD]
