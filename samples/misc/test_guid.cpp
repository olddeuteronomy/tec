
#include <iostream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_guid.hpp"


int main() {
    for (int i = 0; i < 5; ++i) {
        std::cout << tec::guid::generate() << "\n";
    }
    return 0;
}
