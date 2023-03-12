/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 *   \file tec_win_utils.hpp
 *   \brief MS Windows stuff.
 *
 *  TODO: Detailed description
 *
*/

#pragma once

#include "../tec_def.hpp"

#if !defined (__TEC_WINDOWS__)
#error This file can be used on MS Windows only!

#else
// Windows stuff goes here

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <lmcons.h>


#include "../tec_utils.hpp"


namespace tec {


inline String getusername()
{
    TCHAR name[UNLEN + 1];
    DWORD size = UNLEN + 1;

    if( GetUserName(name, &size) )
        return name;
    else
        return _T("unnamed");
}


inline String getcomputername()
{
    TCHAR name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if( GetComputerName(name, &size) )
        return name;
    else
        return _T("noname");
}


} // ::tec

#endif // __TEC_WINDOWS__
