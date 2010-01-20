/*****************************************************************
|
|   Neptune - Time
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

#ifndef _NPT_TIME_H_
#define _NPT_TIME_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"

/*----------------------------------------------------------------------
|   NPT_TimeStamp
+---------------------------------------------------------------------*/
class NPT_TimeStamp
{
 public:
    // methods
    NPT_TimeStamp(unsigned long seconds = 0, unsigned long nano_seconds = 0) :
        m_Seconds(seconds), m_NanoSeconds(nano_seconds) {}
    NPT_TimeStamp(float seconds);
    operator float() const;
    NPT_TimeStamp& operator+=(const NPT_TimeStamp& time_stamp);
    NPT_TimeStamp& operator-=(const NPT_TimeStamp& time_stamp);
    bool operator==(const NPT_TimeStamp& time_stamp) const;
    bool operator!=(const NPT_TimeStamp& time_stamp) const;
    bool operator>(const NPT_TimeStamp& time_stamp) const;
    bool operator<(const NPT_TimeStamp& time_stamp) const;
    bool operator>=(const NPT_TimeStamp& time_stamp) const;
    bool operator<=(const NPT_TimeStamp& time_stamp) const;

    // friend operators
    friend NPT_TimeStamp operator+(const NPT_TimeStamp& timestamp, long seconds);
    friend NPT_TimeStamp operator-(const NPT_TimeStamp& timestamp, long seconds);

    // members
    long m_Seconds;
    long m_NanoSeconds;
};

/*----------------------------------------------------------------------
|   operator+
+---------------------------------------------------------------------*/
inline 
NPT_TimeStamp 
operator+(const NPT_TimeStamp& t1, const NPT_TimeStamp& t2) 
{
    NPT_TimeStamp t = t1;
    return t += t2;
}

/*----------------------------------------------------------------------
|   operator-
+---------------------------------------------------------------------*/
inline 
NPT_TimeStamp 
operator-(const NPT_TimeStamp& t1, const NPT_TimeStamp& t2) 
{
    NPT_TimeStamp t = t1;
    return t -= t2;
}

/*----------------------------------------------------------------------
|   NPT_TimeInterval
+---------------------------------------------------------------------*/
typedef NPT_TimeStamp NPT_TimeInterval;

/*----------------------------------------------------------------------
|   NPT_DateTime
+---------------------------------------------------------------------*/
typedef struct {
    NPT_Int32 year;
    NPT_Int32 month;
    NPT_Int32 day;
    NPT_Int32 hours;
    NPT_Int32 minutes;
    NPT_Int32 seconds;
    NPT_Int32 milliseconds;
} NPT_DateTime;

typedef NPT_Int32 NPT_TimeZone; /* minutes westward of GMT */
typedef NPT_DateTime NPT_LocalDate;

/*----------------------------------------------------------------------
|   NPT_Date
+---------------------------------------------------------------------*/
typedef struct {
    NPT_LocalDate local;
    NPT_TimeZone  timezone;
} NPT_Date;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define NPT_TIME_MIN_YEAR 1970
#define NPT_TIME_MAX_YEAR 2106

/*----------------------------------------------------------------------
|   NPT_Time
+---------------------------------------------------------------------*/
class NPT_Time
{
public:
    // methods
    static NPT_Result GetGMTDateFromTimeStamp(const NPT_TimeStamp& timestamp, 
                                              NPT_Date&            date);
    static NPT_Result GetLocalDate(NPT_LocalDate& today, NPT_TimeStamp& now);

    // helper functions
    static NPT_Result FormatDate(const NPT_Date& date, 
                                 char*           output, 
                                 NPT_Size        size);
    static NPT_Result GetDateFromString(const char* input, 
                                        NPT_Date&   date);
    static NPT_Result ParseANSIDate(const char* ansi_date, 
                                    NPT_Date&   date,
                                    bool        relaxed = true);
    static NPT_Result ParseRFC850Date(const char* rfc850_date, 
                                      NPT_Date&   date);
};

#endif // _NPT_TIME_H_
