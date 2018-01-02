/*****************************************************************
|
|      Neptune - Time: Posix Implementation
|
|      (c) 2002-2009 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <time.h>
#include <errno.h>

#include "NptTime.h"
#include "NptResults.h"
#include "NptLogging.h"
#include "NptSystem.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
//NPT_SET_LOCAL_LOGGER("neptune.system.posix.time")

/*----------------------------------------------------------------------
|   compatibility wrappers
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_HAVE_GMTIME) && !defined(NPT_CONFIG_HAVE_GMTIME_R)
static int gmtime_r(time_t* time, struct tm* _tm)
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

#if defined(NPT_CONFIG_HAVE_LOCALTIME) && !defined(NPT_CONFIG_HAVE_LOCALTIME_R)
static int localtime_r(time_t* time, struct tm* _tm)
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

#if defined(NPT_CONFIG_HAVE_TM_GMTOFF)
/*----------------------------------------------------------------------
|   NPT_DateTime::GetLocalTimeZone
+---------------------------------------------------------------------*/
NPT_Int32
NPT_DateTime::GetLocalTimeZone()
{
    struct tm tm_local;
    NPT_SetMemory(&tm_local, 0, sizeof(tm_local));
    time_t epoch = 0;
    localtime_r(&epoch, &tm_local);
    
    return (NPT_Int32)(tm_local.tm_gmtoff/60);
}
#else
/*----------------------------------------------------------------------
|   NPT_DateTime::GetLocalTimeZone
+---------------------------------------------------------------------*/
NPT_Int32
NPT_DateTime::GetLocalTimeZone()
{
    time_t epoch = 0;
    
    struct tm tm_gmt;
    NPT_SetMemory(&tm_gmt, 0, sizeof(tm_gmt));
    gmtime_r(&epoch, &tm_gmt);

    struct tm tm_local;
    NPT_SetMemory(&tm_local, 0, sizeof(tm_local));
    localtime_r(&epoch, &tm_local);
    
    time_t time_gmt   = mktime(&tm_gmt);
    time_t time_local = mktime(&tm_local);
    
    return (NPT_Int32)((time_local-time_gmt)/60);
}
#endif
