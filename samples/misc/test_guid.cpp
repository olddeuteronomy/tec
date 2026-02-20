// Time-stamp: <Last changed 2026-02-20 16:28:04 by magnolia>
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

#include <iostream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_guid.hpp"


int main() {
    for (int i = 0; i < 5; ++i) {
        std::cout << tec::guid::generate() << "\n";
    }
    return 0;
}
