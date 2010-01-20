/*****************************************************************
|
|   Neptune - System :: Win32 Implementation
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#if !defined(_WIN32_WCE)
#include <sys/timeb.h>
#endif

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptSystem.h"
#include "NptResults.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   NPT_System::GetProcessId
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetProcessId(NPT_UInt32& id)
{
    //id = getpid();
    id = 0;
    return NPT_SUCCESS;
}

#if defined(_WIN32_WCE)
/*----------------------------------------------------------------------
|   NPT_System::GetCurrentTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetCurrentTimeStamp(NPT_TimeStamp& now)
{
    SYSTEMTIME stime;
    FILETIME   ftime;
    __int64    time64;
    GetSystemTime(&stime);
    SystemTimeToFileTime(&stime, &ftime);

    /* convert to 64-bits 100-nanoseconds value */
    time64 = (((unsigned __int64)ftime.dwHighDateTime)<<32) | ((unsigned __int64)ftime.dwLowDateTime);
    time64 -= 116444736000000000; /* convert from the Windows epoch (Jan. 1, 1601) to the 
                                   * Unix epoch (Jan. 1, 1970) */
    
    now.m_Seconds = (NPT_Int32)(time64/10000000);
    now.m_NanoSeconds = 100*(NPT_Int32)(time64-((unsigned __int64)now.m_Seconds*10000000));

    return NPT_SUCCESS;
}
#else
/*----------------------------------------------------------------------
|   NPT_System::GetCurrentTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetCurrentTimeStamp(NPT_TimeStamp& now)
{
    struct _timeb time_stamp;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    _ftime_s(&time_stamp);
#else
    _ftime(&time_stamp);
#endif
    now.m_Seconds     = (long)time_stamp.time;
    now.m_NanoSeconds = (long)time_stamp.millitm*1000000;

    return NPT_SUCCESS;
}
#endif

/*----------------------------------------------------------------------
|   NPT_System::Sleep
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::Sleep(const NPT_TimeInterval& duration)
{
    DWORD milliseconds = 1000*duration.m_Seconds + duration.m_NanoSeconds/1000000;
    ::Sleep(milliseconds);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::SleepUntil
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::SleepUntil(const NPT_TimeStamp& when)
{
    NPT_TimeStamp now;
    GetCurrentTimeStamp(now);
    if (when > now) {
        NPT_TimeInterval duration = when-now;
        return Sleep(duration);
    } else {
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   NPT_System::SetRandomSeed
+---------------------------------------------------------------------*/
NPT_Result  
NPT_System::SetRandomSeed(unsigned int seed)
{
    srand(seed);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::NPT_System
+---------------------------------------------------------------------*/
NPT_UInt32 
NPT_System::GetRandomInteger()
{
    static bool seeded = false;
    if (seeded == false) {
        NPT_TimeStamp now;
        GetCurrentTimeStamp(now);
        srand(now.m_NanoSeconds);
        seeded = true;
    }

    return rand();
}

