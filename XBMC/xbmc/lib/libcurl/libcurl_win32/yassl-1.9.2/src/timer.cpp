 /* timer.cpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* timer.cpp implements a high res and low res timer
 *
*/

#include "runtime.hpp"
#include "timer.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace yaSSL {

#ifdef _WIN32

    timer_d timer()
    {
        static bool          init(false);
        static LARGE_INTEGER freq;
    
        if (!init) {
            QueryPerformanceFrequency(&freq);
            init = true;
        }

        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);

        return static_cast<double>(count.QuadPart) / freq.QuadPart;
    }


    uint lowResTimer()
    {
        return static_cast<uint>(timer());
    }

#else // _WIN32

    timer_d timer()
    {
        struct timeval tv;
        gettimeofday(&tv, 0);

        return static_cast<double>(tv.tv_sec) 
             + static_cast<double>(tv.tv_usec) / 1000000;
    }


    uint lowResTimer()
    {
        struct timeval tv;
        gettimeofday(&tv, 0);

        return tv.tv_sec; 
    }


#endif // _WIN32
} // namespace yaSSL
