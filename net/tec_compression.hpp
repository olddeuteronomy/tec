// Time-stamp: <Last changed 2026-02-20 16:20:58 by magnolia>
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
/**
 * @file tec_compression.hpp
 * @brief Compression parameters.
 * @author The Emacs Cat
 * @date 2025-12-23
 */

#pragma once

#include <cstddef>


namespace tec {


struct CompressionParams {
    // Compression type, 0..15
    static constexpr int kNoCompression{0};
    static constexpr int kCompressionZlib{1};

    static constexpr int kDefaultCompression{kNoCompression};

    // Compression level, 0..9
    static constexpr int kCompressionLevelMin{0};
    static constexpr int kCompressionLevelMax{9};

    static constexpr int kDefaultCompressionLevel{4};

    // Minimum data size in bytes for compression.
    static constexpr size_t kMinSize{128};
};


} // namespace tec
