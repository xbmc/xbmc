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
#include "Neptune.h"
#include "NptTime.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.core.time")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* NPT_TIME_MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define NPT_TIME_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

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

/*----------------------------------------------------------------------
|   FormatDigits
+---------------------------------------------------------------------*/
static NPT_Result
FormatDigits(char* out, NPT_Int32 number, NPT_Int32 num_digits)
{
    if (num_digits == 0) {
        return number?NPT_SUCCESS:NPT_ERROR_OVERFLOW;
    }

    *(out+num_digits-1) = '0' + (char)(number % 10);
    return FormatDigits(out, number/10, num_digits-1);
}

/*----------------------------------------------------------------------
|   NPT_Time::FormatDate
+---------------------------------------------------------------------*/
NPT_Result
NPT_Time::FormatDate(const NPT_Date& date, char* output, NPT_Size size)
{
    NPT_Int32 timezone;

    /* make sure we have enough space */
    if (size < 30) return NPT_FAILURE;

    /* append year */
    FormatDigits(output, date.local.year, 4);
    output += 4;

    /* append separator */
    *(output++) = '-';

    /* append month */
    FormatDigits(output, date.local.month, 2);
    output += 2;

    /* append separator */
    *(output++) = '-';

    /* append day */
    FormatDigits(output, date.local.day, 2);
    output += 2;

    /* append separator */
    *(output++) = 'T';

    /* append hours */
    FormatDigits(output, date.local.hours, 2);
    output += 2;

    /* append separator */
    *(output++) = ':';

    /* append minutes */
    FormatDigits(output, date.local.minutes, 2);
    output += 2;

    /* append separator */
    *(output++) = ':';

    /* append seconds */
    FormatDigits(output, date.local.seconds, 2);
    output += 2;

    /* append milliseconds only if non zero */
    if (date.local.milliseconds) {
        /* append separator */
        *(output++) = '.';

        /* append milliseconds */
        FormatDigits(output, date.local.milliseconds, 3);
        output += 3;
    }

    /* append timezone */
    if (date.timezone) {
        if (date.timezone > 0) {
            *(output++) = '-';
            timezone = date.timezone;
        } else {
            *(output++) = '+';
            timezone = -date.timezone;
        }
        FormatDigits(output, timezone / 60, 2);
        output += 2;

        /* append separator */
        *(output++) = ':';
        FormatDigits(output, timezone - ((timezone / 60) * 60), 2);
        output += 2;
    } else {
        *(output++) = 'Z';
    }

    *output = '\0';

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
 |   NPT_Time::GetDateFromString
 +--------------------------------------------------------------------*/
NPT_Result
NPT_Time::GetDateFromString(const char* input, NPT_Date& date)
{
    NPT_Cardinal length = NPT_StringLength(input);
    NPT_Date     temp_date;
    int          tz_sign = 0;
    NPT_Int32    tz_hours = 0;
    NPT_Cardinal chars_used;
    NPT_Cardinal offset = 0;

    if (length > 35) return NPT_FAILURE;  /* max length up to the nanosecond */

    /* Year */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.year, true, &chars_used));
    if (chars_used != 4) NPT_CHECK_SEVERE(NPT_FAILURE);
    offset += chars_used;

    /* Separator */
    if (input[offset++] != '-') NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Month */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.month, true, &chars_used));
    if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
    offset += chars_used;

    /* Validate */
    if (temp_date.local.month > 12) NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Separator */
    if (input[offset++] != '-') NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Day */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.day, true, &chars_used));
    if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
    offset += chars_used;

    /* Validate */
    if (temp_date.local.day > 31) NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Separator */
    if (input[offset++] != 'T') NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Hour */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.hours, true, &chars_used));
    if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
    offset += chars_used;

    /* Validate */
    if (temp_date.local.hours >= 24) NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Separator */
    if (input[offset++] != ':') NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Minute */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.minutes, true, &chars_used));
    if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
    offset += chars_used;

    /* Validate */
    if (temp_date.local.minutes >= 60) NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Separator */
    if (input[offset++] != ':') NPT_CHECK_SEVERE(NPT_FAILURE);

    /* Second */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.seconds, true, &chars_used));
    if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
    offset += chars_used;

    /* validate */
    if (temp_date.local.seconds >= 60) NPT_CHECK_SEVERE(NPT_FAILURE);

    /* fractions of seconds? */
    if (input[offset] == '.') {
        /* Separator */
        ++offset;

        NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.local.milliseconds, true, &chars_used));
        /* if more than 3 chars, update */
        for (int i=chars_used-3;i>0;i--) {
            temp_date.local.milliseconds /= 10;
        }
        /* if more than 3 chars, update */
        for (int i=3-chars_used;i>0;i--) {
            temp_date.local.milliseconds *= 10;
        }

        offset += chars_used;
    } else {
        temp_date.local.milliseconds = 0;
    }

    /* Timezone */
    if (offset < length) {
        /* 'Z' if present should be the last character. */
        if (input[offset] == 'Z') {
            ++offset;
            temp_date.timezone = 0; /* Z means GMT */
        } else if (input[offset] == '+') {
            tz_sign = +1;
            ++offset;
        } else if (input[offset] == '-') {
            tz_sign = -1;
            ++offset;
        } else {
            return NPT_FAILURE;
        }

        if (tz_sign != 0) {
            /* Timezone hours */
            NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], tz_hours, true, &chars_used));
            if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
            offset += chars_used;

            /* Separator */
            if (input[offset++] != ':') NPT_CHECK_SEVERE(NPT_FAILURE);

            /* Timezone minutes */
            NPT_CHECK_SEVERE(NPT_ParseInteger32(&input[offset], temp_date.timezone, true, &chars_used));
            if (chars_used != 2) NPT_CHECK_SEVERE(NPT_FAILURE);
            offset += chars_used;

            /* validate */
            if (tz_hours > 14) NPT_CHECK_SEVERE(NPT_FAILURE);
            if (temp_date.timezone >= 60) NPT_CHECK_SEVERE(NPT_FAILURE);

            /* no timezone can be greater than 14 hours */
            if (tz_hours == 14 && temp_date.timezone != 0) NPT_CHECK_SEVERE(NPT_FAILURE);

            temp_date.timezone += tz_hours*60;
            temp_date.timezone *= tz_sign;
        }
    } else {
        temp_date.timezone = 0;
    }

    /* verify there's nothing else to parse */
    if (offset != length) NPT_CHECK_SEVERE(NPT_FAILURE);

    date = temp_date;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Time::ParseANSIDate
+---------------------------------------------------------------------*/
NPT_Result
NPT_Time::ParseANSIDate(const char* ansi_date, 
                        NPT_Date&   date, 
                        bool        relaxed /* = true */)
{
    /* 
     * ANSI Date string in the __DATE__ macro is 11 characters 
     * in the following format: Mmm dd yyyy
     */

    NPT_Date temp_date;

    /* Reset output param first */
    NPT_SetMemory(&date, 0, sizeof(date));

    if (NPT_StringLength(ansi_date) != 11) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    NPT_String month = NPT_String(ansi_date, 0, 3);
    NPT_String day   = NPT_String(ansi_date, 4, 2);
    NPT_String year  = NPT_String(ansi_date, 7, 4);

    /* Parse month */
    for (int i = 0; i < 12; ++i) {
        if (month.Compare(NPT_TIME_MONTHS[i], relaxed) == 0) {
            temp_date.local.month = i + 1;
            break;
        }
    }

    if (temp_date.local.month == 0) {
        NPT_LOG_SEVERE_1("Unable to parse build date month='%s'", month.GetChars());
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    /* Parse day */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(day, temp_date.local.day, relaxed));
    if (temp_date.local.day <= 0 || temp_date.local.day > 31) {
        NPT_LOG_SEVERE_1("Unable to parse build date day='%s'", day.GetChars());
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    /* Parse year */
    NPT_CHECK_SEVERE(NPT_ParseInteger32(year, temp_date.local.year, relaxed));
    if (temp_date.local.year < NPT_TIME_MIN_YEAR || temp_date.local.year > NPT_TIME_MAX_YEAR) {
        NPT_LOG_SEVERE_1("Unable to parse build date year='%s'", year.GetChars());
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    date = temp_date;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Time::ParseRFC850Date
+---------------------------------------------------------------------*/
#ifdef TODO
NPT_Result
NPT_Time::ParseRFC850Date(const char* rfc850_date, NPT_Date& date)
{
    /* RFC850 in the following format: 
       Weekday, DD-Mon-YY HH:MM:SS TIMEZONE
       */
    NPT_Result res;
    NPT_String rfc850 = rfc850_date;
    NPT_List<NPT_String> fragments = rfc850.Split(" ");

    return NPT_SUCCESS;
}
#endif
