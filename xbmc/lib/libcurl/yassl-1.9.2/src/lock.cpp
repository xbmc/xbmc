/* lock.cpp
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

/*  Locking functions
 */

#include "runtime.hpp"
#include "lock.hpp"


namespace yaSSL {


#ifdef MULTI_THREADED
    #ifdef _WIN32
        
        Mutex::Mutex()
        {
            InitializeCriticalSection(&cs_);
        }


        Mutex::~Mutex()
        {
            DeleteCriticalSection(&cs_);
        }

            
        Mutex::Lock::Lock(Mutex& lm) : mutex_(lm)
        {
            EnterCriticalSection(&mutex_.cs_); 
        }


        Mutex::Lock::~Lock()
        {
            LeaveCriticalSection(&mutex_.cs_); 
        }
            
    #else  // _WIN32
        
        Mutex::Mutex()
        {
            pthread_mutex_init(&mutex_, 0);
        }


        Mutex::~Mutex()
        {
            pthread_mutex_destroy(&mutex_);
        }


        Mutex::Lock::Lock(Mutex& lm) : mutex_(lm)
        {
            pthread_mutex_lock(&mutex_.mutex_); 
        }


        Mutex::Lock::~Lock()
        {
            pthread_mutex_unlock(&mutex_.mutex_); 
        }
         

    #endif // _WIN32
#endif // MULTI_THREADED



} // namespace yaSSL

