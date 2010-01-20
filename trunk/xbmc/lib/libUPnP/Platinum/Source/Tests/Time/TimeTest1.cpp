/*****************************************************************
|
|   Platinum - Time Test
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/


/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "Neptune.h"
#include "Platinum.h"

#include "NptLogging.h"

NPT_SET_LOCAL_LOGGER("platinum.time.test")

/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define SHOULD_SUCCEED(r)                                        \
    do {                                                         \
        if (NPT_FAILED(r)) {                                     \
            fprintf(stderr, "FAILED: line %d\n", __LINE__);      \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                         

#define SHOULD_FAIL(r)                                           \
    do {                                                         \
        if (NPT_SUCCEEDED(r)) {                                  \
            fprintf(stderr, "should have failed line %d (%d)\n", \
                __LINE__, r);                                    \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                  

#define SHOULD_EQUAL_I(a, b)                                     \
    do {                                                         \
        if ((a) != (b)) {                                        \
            fprintf(stderr, "got %d expected %d line %d\n",      \
                (int)a, (int)b, __LINE__);                       \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                  

#define SHOULD_EQUAL_F(a, b)                                     \
    do {                                                         \
        if ((a) != (b)) {                                        \
            fprintf(stderr, "got %f, expected %f line %d\n",     \
                (float)a, (float)b, __LINE__);                   \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                  

#define SHOULD_EQUAL_S(a, b)                                     \
    do {                                                         \
        if (!NPT_StringsEqual(a,b)) {                            \
            fprintf(stderr, "got %s, expected %s line %d\n",     \
                a, b, __LINE__);                                 \
            NPT_ASSERT(0);                                           \
        }                                                        \
    } while(0)     

/*----------------------------------------------------------------------
|   TestSuiteGetTime
+---------------------------------------------------------------------*/
static void
TestSuiteGetTime()
{
    NPT_TimeStamp now, now2;
    NPT_Date      today;

    /* get utc time */
    SHOULD_SUCCEED(NPT_System::GetCurrentTimeStamp(now));

    /* convert utc time to date */
    SHOULD_SUCCEED(NPT_Time::GetGMTDateFromTimeStamp(now, today));

    /* convert local time back to utc */
    SHOULD_SUCCEED(PLT_Time::GetTimeStampFromDate(today, now2));

    /* verify utc time has not change */
    SHOULD_EQUAL_I(now.m_Seconds, now2.m_Seconds);
}

/*----------------------------------------------------------------------
|   TestSuiteSetDateTimeZone
+---------------------------------------------------------------------*/
static void
TestSuiteSetDateTimeZone()
{
    NPT_TimeStamp now, now2;
    NPT_Date      today, today2;
    NPT_TimeZone  tz;

    /* get utc time */
    SHOULD_SUCCEED(NPT_System::GetCurrentTimeStamp(now));

    /* convert utc time to date */
    SHOULD_SUCCEED(NPT_Time::GetGMTDateFromTimeStamp(now, today));

    for (tz = -60*12; tz <= 60*12; tz+=30) {
        /* convert date to another timezone */
        today2 = today;
        SHOULD_SUCCEED(PLT_Time::SetDateTimeZone(today2, tz));

        /* get timestamp from converted date */
        SHOULD_SUCCEED(PLT_Time::GetTimeStampFromDate(today2, now2));

        /* verify utc time has not change */
        SHOULD_EQUAL_I(now.m_Seconds, now2.m_Seconds);
    }
}

/*----------------------------------------------------------------------
|   TestSuiteFormatTime
+---------------------------------------------------------------------*/
static void
TestSuiteFormatTime()
{
    char          output[30];
    NPT_Date      gmt_today, tz_today;
    NPT_TimeStamp now;

    /* current time */
    SHOULD_SUCCEED(NPT_System::GetCurrentTimeStamp(now));

    /* get the date */
    SHOULD_SUCCEED(NPT_Time::GetGMTDateFromTimeStamp(now, gmt_today));

    /* print out current local date and daylight savings settings */
    /* this should convert to GMT internally if dst is set */
    SHOULD_SUCCEED(NPT_Time::FormatDate(gmt_today, output, sizeof(output)));

    NPT_LOG_INFO_1("GMT time for Today is: %s", output);

    /* convert the date to GMT-8 */
    tz_today = gmt_today;
    SHOULD_SUCCEED(PLT_Time::SetDateTimeZone(tz_today, 8*60));

    /* this should convert to GMT internally if dst is set */
    SHOULD_SUCCEED(NPT_Time::FormatDate(tz_today, output, sizeof(output)));

    NPT_LOG_INFO_1("(GMT-8) time for Today is: %s", output);
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    TestSuiteGetTime();
    TestSuiteSetDateTimeZone();
    TestSuiteFormatTime();
    return 0;
}
