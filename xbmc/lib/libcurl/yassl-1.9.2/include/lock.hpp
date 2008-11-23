/* lock.hpp                                
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

/* lock.hpp provides an os specific Lock, locks mutex on entry and unlocks
 * automatically upon exit, no-ops provided for Single Threaded
*/

#ifndef yaSSL_LOCK_HPP
#define yaSSL_LOCK_HPP


namespace yaSSL {


#ifdef MULTI_THREADED
    #ifdef _WIN32
        #include <windows.h>

        class Mutex {
            CRITICAL_SECTION cs_;
        public:
            Mutex();
            ~Mutex();

            class Lock;
            friend class Lock;
    
            class Lock {
                Mutex& mutex_;
            public:
                explicit Lock(Mutex& lm);
                ~Lock();
            };
        };
    #else  // _WIN32
        #include <pthread.h>

        class Mutex {
            pthread_mutex_t mutex_;
        public:

            Mutex();
            ~Mutex();

            class Lock;
            friend class Lock;

            class Lock {
                Mutex& mutex_;
            public:
                explicit Lock(Mutex& lm);
                ~Lock();
            };
        };

    #endif // _WIN32
#else  // MULTI_THREADED (WE'RE SINGLE)

    class Mutex {
    public:
        class Lock {
        public:
            explicit Lock(Mutex&) {}
        };
    };

#endif // MULTI_THREADED



} // namespace
#endif // yaSSL_LOCK_HPP
