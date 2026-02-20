// Time-stamp: <Last changed 2026-02-20 16:17:32 by magnolia>
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
 *   \file tec_win_utils.hpp
 *   \brief MS Windows helpers and utilities.
 *
 *  EXPERIMENTAL. MS Windows-related system calls.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep


#if !(defined (__TEC_WINDOWS__) || defined(__TEC_MINGW__))
#error This file can be used on MS Windows only!
#else
// Windows stuff goes here
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <lmcons.h>

#include <string>


namespace tec {


inline std::string  getusername()
{
    TCHAR name[UNLEN + 1];
    DWORD size = UNLEN + 1;

    if( GetUserName(name, &size) )
        return name;
    else
        return _T("");
}


inline std::string getcomputername()
{
    TCHAR name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if( GetComputerName(name, &size) )
        return name;
    else
        return _T("");
}


} // ::tec

#endif // __TEC_WINDOWS__ || __TEC_MINGW__
