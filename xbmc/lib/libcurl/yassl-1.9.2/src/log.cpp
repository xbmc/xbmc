/* log.cpp
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

/*  Debug logging functions
 */


#include "runtime.hpp"
#include "log.hpp"

#ifdef YASSL_LOG
    #include <time.h>
    #include <stdio.h>
    #include <string.h>
#endif



namespace yaSSL {


#ifdef YASSL_LOG

    enum { MAX_MSG = 81 };

    Log::Log(const char* str)
    {
        log_ = fopen(str, "w");
        Trace("********** Logger Attached **********");
    }


    Log::~Log()
    {
        Trace("********** Logger Detached **********");
        fclose(log_);
    }


    // Trace a message
    void Log::Trace(const char* str)
    {
        if (!log_) return;

        time_t clicks = time(0);
        char   timeStr[32];

        // get rid of newline
        strncpy(timeStr, ctime(&clicks), sizeof(timeStr));
        unsigned int len = strlen(timeStr);
        timeStr[len - 1] = 0;

        char msg[MAX_MSG];

        strncpy(msg, timeStr, sizeof(timeStr));
        strncat(msg, ":", 1);
        strncat(msg, str, MAX_MSG - sizeof(timeStr) - 2);
        strncat(msg, "\n", 1);
        msg[MAX_MSG - 1] = 0;

        fputs(msg, log_);
    }


    #if defined(_WIN32) || defined(__MACH__) || defined(__hpux__)
        typedef int socklen_t;
    #endif


    // write tcp address
    void Log::ShowTCP(socket_t fd, bool ended)
    {
        sockaddr_in peeraddr;
        socklen_t   len = sizeof(peeraddr);
        if (getpeername(fd, (sockaddr*)&peeraddr, &len) != 0)
            return;

        const char* p = reinterpret_cast<const char*>(&peeraddr.sin_addr);
        char msg[MAX_MSG];
        char number[16];
    
        if (ended)
            strncpy(msg, "yaSSL conn DONE  w/ peer ", 26);
        else
            strncpy(msg, "yaSSL conn BEGUN w/ peer ", 26);
        for (int i = 0; i < 4; ++i) {
            sprintf(number, "%u", static_cast<unsigned short>(p[i]));
            strncat(msg, number, 8);
            if (i < 3)
                strncat(msg, ".", 1);
        }
        strncat(msg, " port ", 8);
        sprintf(number, "%d", htons(peeraddr.sin_port));
        strncat(msg, number, 8);

        msg[MAX_MSG - 1] = 0;
        Trace(msg);
    }


    // log processed data
    void Log::ShowData(uint bytes, bool sent)
    {
        char msg[MAX_MSG];
        char number[16];

        if (sent)
            strncpy(msg, "Sent     ", 10); 
        else
            strncpy(msg, "Received ", 10);
        sprintf(number, "%u", bytes);
        strncat(msg, number, 8);
        strncat(msg, " bytes of application data", 27);

        msg[MAX_MSG - 1] = 0;
        Trace(msg);
    }


#else // no YASSL_LOG


    Log::Log(const char*) {}
    Log::~Log() {}
    void Log::Trace(const char*) {}
    void Log::ShowTCP(socket_t, bool) {}
    void Log::ShowData(uint, bool) {}


#endif // YASSL_LOG
} // namespace
