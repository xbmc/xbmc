/*****************************************************************
|
|   Platinum - UPnP Engine
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
#include "Neptune.h"
#include "PltTime.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("platinum.core.time")

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define PLT_TIME_IS_LEAP_YEAR(y) ((((y)%4 == 0) && ((y)%100 != 0)) || ((y)%400 == 0)) 
#define PLT_TIME_CHECK_BOUNDS(_var, _low, _high) do {                                       \
    if ((_var<_low) || (_var>_high)) {                                                      \
        NPT_LOG_SEVERE_3("PLT_TIME_CHECK_BOUNDS failed, var=%d [%d-%d]", _var, _low, _high);\
        return NPT_FAILURE;                                                                 \
    }                                                                                       \
} while (0)

/*----------------------------------------------------------------------
|   GetElapsedLeapYearsSinceEpoq
+---------------------------------------------------------------------*/
static NPT_Result
GetElapsedLeapYearsSinceEpoq(NPT_UInt32 year, NPT_UInt32* elapsed_leap_years)
{
    NPT_UInt32 years_since_1900;

    /* preliminary check */
    if (year < NPT_TIME_MIN_YEAR) return NPT_FAILURE;

    /* we do not include the current year */
    years_since_1900 = year - 1 - 1900;
    *elapsed_leap_years = years_since_1900/4 - years_since_1900/100 
                        + (years_since_1900+300)/400 
                        - 17; /* 17 = leap years from 1900 to 1970 */
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Time::SetDateTimeZone
+---------------------------------------------------------------------*/
NPT_Result
PLT_Time::SetDateTimeZone(NPT_Date& date, NPT_TimeZone tz)
{
    NPT_TimeStamp ts;

    /* convert in timestamp */
    NPT_CHECK_SEVERE(PLT_Time::GetTimeStampFromDate(date, ts));

    /* add the timezone */
    ts.m_Seconds -= tz*60;

    /* convert into GMT date */
    NPT_CHECK_SEVERE(NPT_Time::GetGMTDateFromTimeStamp(ts, date));

    /* adjust the timezone */
    date.timezone = tz;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Time::GetTimeStampFromDate
+---------------------------------------------------------------------*/
NPT_Result
PLT_Time::GetTimeStampFromDate(const NPT_Date& date, NPT_TimeStamp& timestamp)
{
    NPT_UInt32 leap_year_count;
    NPT_UInt32 day_count;

    static const NPT_Int32 elapsed_days_per_month[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };

    /* sanity check */
    PLT_TIME_CHECK_BOUNDS(date.local.year, NPT_TIME_MIN_YEAR, NPT_TIME_MAX_YEAR);
    PLT_TIME_CHECK_BOUNDS(date.local.month, 1, 12);
    PLT_TIME_CHECK_BOUNDS(date.local.day, 1, 31);
    PLT_TIME_CHECK_BOUNDS(date.local.hours, 0, 23);
    PLT_TIME_CHECK_BOUNDS(date.local.minutes, 0, 59);
    PLT_TIME_CHECK_BOUNDS(date.local.seconds, 0, 59);
    PLT_TIME_CHECK_BOUNDS(date.local.milliseconds, 0, 999);
    PLT_TIME_CHECK_BOUNDS(date.timezone, -12*60, 12*60);

    /* make sure that we're after 1970 00:00:00 GMT */
    if ((date.local.year == NPT_TIME_MIN_YEAR)   &&
        (date.timezone < 0)                      && 
        (date.local.month == 1)                  &&
        (date.local.day == 1)                    &&
        (date.local.hours*60 + date.local.minutes + date.timezone < 0)) {  
        return NPT_FAILURE;
    }

    /* compute the number of days elapsed in the year */
    day_count = elapsed_days_per_month[date.local.month-1] + date.local.day - 1;

    /* adjust if leap year and we're after February */
    if (PLT_TIME_IS_LEAP_YEAR(date.local.year) && (date.local.month > 2)) day_count++;
    
    /* compute the total number of elapsed days */
    NPT_CHECK_SEVERE(GetElapsedLeapYearsSinceEpoq(date.local.year, &leap_year_count));
    day_count += (date.local.year-NPT_TIME_MIN_YEAR)*365 + leap_year_count;

    /* number of seconds */
    timestamp.m_Seconds = (day_count*24 + date.local.hours)*3600
                  + date.local.minutes*60 + date.local.seconds;
    timestamp.m_NanoSeconds = date.local.milliseconds*1000000;

    /* adjust the timezone */
    timestamp.m_Seconds += date.timezone*60;

    return NPT_SUCCESS;
}

