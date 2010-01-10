/*****************************************************************
|
|      Neptune - Time: StdC Implementation
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <time.h>

#include <stdio.h>
#if !defined(_WIN32_WCE)
#include <errno.h>
#else
#include <stdio.h>
#define errno GetLastError()
#endif

#include "NptTime.h"
#include "NptResults.h"
#include "NptLogging.h"
#include "NptSystem.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.system.stdc.time")

/*----------------------------------------------------------------------
|   compatibility wrappers
+---------------------------------------------------------------------*/
#if !defined(NPT_CONFIG_HAVE_GMTIME_S)
static int gmtime_s(struct tm* _tm, time_t* time)
{
    struct tm* _gmt = gmtime(time);

#if defined(_WIN32_WCE)
    if (_gmt == NULL) return ENOENT;
#else
    if (_gmt== NULL) return errno;
#endif

    *_tm  = *_gmt;
    return 0;
}
#endif // defined(NPT_CONFIG_HAVE_GMTIME_S

#if !defined(NPT_CONFIG_HAVE_LOCALTIME_S)
static int localtime_s(struct tm* _tm, time_t* time)
{   
    struct tm* _local = localtime(time);

#if defined(_WIN32_WCE)
    if (_local == NULL) return ENOENT;
#else
    if (_local== NULL) return errno;
#endif

    *_tm  = *_local;
    return 0;
}
#endif // defined(NPT_CONFIG_HAVE_LOCALTIME_S


/*----------------------------------------------------------------------
|   TmStructToNptLocalDate
+---------------------------------------------------------------------*/
static void
TmStructToNptLocalDate(const struct tm& t, NPT_LocalDate& date)
{
    date.year    = t.tm_year+1900;
    date.month   = t.tm_mon+1;
    date.day     = t.tm_mday;
    date.hours   = t.tm_hour;
    date.minutes = t.tm_min;
    date.seconds = t.tm_sec;
}

/*----------------------------------------------------------------------
|   NPT_GetGMTDateFromTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
NPT_Time::GetGMTDateFromTimeStamp(const NPT_TimeStamp& time, NPT_Date& date)
{
    time_t     ltime;
    struct tm  gmt;

    NPT_SetMemory(&gmt, 0, sizeof(gmt));

    ltime = (time_t) time.m_Seconds;
    int result = gmtime_s(&gmt, &ltime);
    if (result != 0) return NPT_ERROR_ERRNO(result);

    TmStructToNptLocalDate(gmt, date.local);
    date.local.milliseconds = time.m_NanoSeconds/1000000;
    date.timezone           = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_GetLocalDate
+---------------------------------------------------------------------*/
NPT_Result
NPT_Time::GetLocalDate(NPT_LocalDate& today, NPT_TimeStamp& now)
{
    time_t    ltime;
    struct tm local;

    NPT_SetMemory(&local, 0, sizeof(local));

    /* get the current time stamp */
    NPT_CHECK_SEVERE(NPT_System::GetCurrentTimeStamp(now));

    ltime = (time_t) now.m_Seconds;
    int result = localtime_s(&local, &ltime);
    if (result != 0) return NPT_ERROR_ERRNO(result);

    TmStructToNptLocalDate(local, today);
    today.milliseconds = now.m_NanoSeconds/1000000;

    return NPT_SUCCESS;
}
