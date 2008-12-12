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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTime.h"

/*----------------------------------------------------------------------
|   NPT_TimeStamp::NPT_TimeStamp
+---------------------------------------------------------------------*/
NPT_TimeStamp::NPT_TimeStamp(float seconds)
{
    m_Seconds     = (long)seconds;
    m_NanoSeconds = (long)(1E9f*(seconds - (float)m_Seconds));
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator float
+---------------------------------------------------------------------*/
NPT_TimeStamp::operator float() const
{       
    return ((float)m_Seconds) + ((float)m_NanoSeconds)/1E9f;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator+=
+---------------------------------------------------------------------*/
NPT_TimeStamp&
NPT_TimeStamp::operator+=(const NPT_TimeStamp& t)
{
    m_Seconds += t.m_Seconds;
    m_NanoSeconds += t.m_NanoSeconds;
    if (m_NanoSeconds <= -1000000000) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    } else if (m_NanoSeconds >= 1000000000) {
        m_Seconds++;
        m_NanoSeconds -= 1000000000;
    }
    if (m_NanoSeconds < 0 && m_Seconds > 0) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    }
    return *this;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator-=
+---------------------------------------------------------------------*/
NPT_TimeStamp&
NPT_TimeStamp::operator-=(const NPT_TimeStamp& t)
{
    m_Seconds -= t.m_Seconds;
    m_NanoSeconds -= t.m_NanoSeconds;
    if (m_NanoSeconds <= -1000000000) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    } else if (m_NanoSeconds >= 1000000000) {
        m_Seconds++;
        m_NanoSeconds -= 1000000000;
    }
    if (m_NanoSeconds < 0 && m_Seconds > 0) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    }
    return *this;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator==
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator==(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds     == t.m_Seconds && 
        m_NanoSeconds == t.m_NanoSeconds;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator!=
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator!=(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds     != t.m_Seconds || 
        m_NanoSeconds != t.m_NanoSeconds;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator>
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator>(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds   > t.m_Seconds || 
        (m_Seconds == t.m_Seconds && m_NanoSeconds > t.m_NanoSeconds);
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator<
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator<(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds   < t.m_Seconds || 
        (m_Seconds == t.m_Seconds && m_NanoSeconds < t.m_NanoSeconds);
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator>=
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator>=(const NPT_TimeStamp& t) const
{
    return *this > t || *this == t;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator<=
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator<=(const NPT_TimeStamp& t) const
{
    return *this < t || *this == t;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator+
+---------------------------------------------------------------------*/
NPT_TimeStamp
operator+(const NPT_TimeStamp& timestamp, long seconds)
{
    // shortcut
    if (seconds == 0) return NPT_TimeStamp(timestamp);

    NPT_TimeStamp result = timestamp;
    result.m_Seconds += seconds;

    return result;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator-
+---------------------------------------------------------------------*/
NPT_TimeStamp
operator-(const NPT_TimeStamp& timestamp, long seconds)
{
    // shortcut
    if (seconds == 0) return NPT_TimeStamp(timestamp);

    NPT_TimeStamp result = timestamp;
    result.m_Seconds -= seconds;

    return result;
}

