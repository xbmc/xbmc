/* log.hpp                                
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


/* yaSSL log interface
 *
 */

#ifndef yaSSL_LOG_HPP
#define yaSSL_LOG_HPP

#include "socket_wrapper.hpp"

#ifdef YASSL_LOG
#include <stdio.h>
#endif

namespace yaSSL {

typedef unsigned int uint;


// Debug logger
class Log {
#ifdef YASSL_LOG
    FILE* log_;
#endif
public:
    explicit Log(const char* str = "yaSSL.log");
    ~Log();

    void Trace(const char*);
    void ShowTCP(socket_t, bool ended = false);
    void ShowData(uint, bool sent = false);
};


} // naemspace

#endif // yaSSL_LOG_HPP
