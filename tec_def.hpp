// Time-stamp: <Last changed 2025-08-26 22:18:31 by magnolia>
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
/**
 *   @file tec_def.hpp
 *   @brief __TEC internal macros.
 *
 *  Defines various compiler- and OS-depended macros.
 *
*/

#pragma once

#define __TEC__ 1

// To resolve C-macros conflicts
#ifndef NOMINMAX
  #define NOMINMAX 1
#endif

/// OS specific defines

// Check for Linux
#if defined(linux) || defined(__linux) || defined(__linux__)
  #define __TEC__LINUX__ 1
#endif

// Check for Unix
#if defined(unix) || defined(__unix) || defined(__unix__)
  #define __TEC_UNIX__ 1
#endif

// Check for Apple
#if defined(__APPLE__)
  #define __TEC_APPLE__ __APPLE__
#endif

// Check for MS Windows
#if defined(_MSC_VER) && (defined(_WIN32) || defined(_WIN64))
  #define __TEC_WINDOWS__ _MSC_VER
#endif

// Check for MINGW
#if defined(__MSVCRT__) && (defined(__MINGW32__) || defined(__MINGW64__))
  #define __TEC_MINGW__ 1
#endif

#if defined(__clang__)
// Check clang
  // This pragma has been removed to make the code cleaner:
  // #pragma clang diagnostic ignored "-Wmicrosoft-template-shadow"
  #define __TEC_CLANG__         1
  #define __TEC_COMPILER_NAME__ __VERSION__

  #define __TEC_COMPILER_VER_MAJOR__      __clang_major__
  #define __TEC_COMPILER_VER_MINOR__      __clang_minor__
  #define __TEC_COMPILER_VER_PATCHLEVEL__ __clang_patchlevel__
  #define __TEC_INT__  __INT_WIDTH__
  #define __TEC_LONG__ __LONG_WIDTH__
  #define __TEC_PTR__  __INTPTR_WIDTH__

#elif defined(__TEC_MINGW__)
// Check MINGW
  #define __TEC_COMPILER_NAME__ __VERSION__

  #define __TEC_COMPILER_VER_MAJOR__      __GNUC__
  #define __TEC_COMPILER_VER_MINOR__      __GNUC_MINOR__
  #define __TEC_COMPILER_VER_PATCHLEVEL__ __GNUC_PATCHLEVEL__
  #define __TEC_INT__  __SIZEOF_INT__<<3
  #define __TEC_LONG__ __SIZEOF_LONG__<<3
  #define __TEC_PTR__  __SIZEOF_POINTER__<<3

#elif defined(__GNUC__)
// Check gcc
  #define __TEC_GNUC__          __GNUC__
  #define __TEC_COMPILER_NAME__ "g++" " " __VERSION__

  #define __TEC_COMPILER_VER_MAJOR__      __GNUC__
  #define __TEC_COMPILER_VER_MINOR__      __GNUC_MINOR__
  #define __TEC_COMPILER_VER_PATCHLEVEL__ __GNUC_PATCHLEVEL__
  #define __TEC_INT__  __SIZEOF_INT__<<3
  #define __TEC_LONG__ __SIZEOF_LONG__<<3
  #define __TEC_PTR__  __SIZEOF_POINTER__<<3

#elif defined(__TEC_WINDOWS__)
// Windows specific
  #define __TEC_MSC__           _MSC_VER
  #define __TEC_COMPILER_NAME__ "cl"

  #define __TEC_COMPILER_VER_MAJOR__      _MSC_VER
  // TODO
  #define __TEC_COMPILER_VER_MINOR__      0
  #define __TEC_COMPILER_VER_PATCHLEVEL__ _MSC_BUILD

  #if defined(_WIN64)
    #define __TEC_INT__  32
    #define __TEC_LONG__ 32
    #define __TEC_PTR__  64
  #else
    #define __TEC_INT__  32
    #define __TEC_LONG__ 32
    #define __TEC_PTR__  32
  #endif

#endif // OS specific defines end here


#if !defined(__TEC_INT__) || !defined(__TEC_PTR__) || !defined(__TEC_LONG__)
  #error Missing feature-test macro for 32/64-bit on this compiler.
#endif
