/*****************************************************************
|
|      Neptune - System :: Win32 Implementation
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <psptypes.h>
#include <kernel.h>
#include <rtcsvc.h>
#include <pspnet/sys/time.h>

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptSystem.h"
#include "NptResults.h"
#include "NptDebug.h"
#include "NptConstants.h"

/*----------------------------------------------------------------------
|       NPT_PSPSystem
+---------------------------------------------------------------------*/
class NPT_PSPSystem : public NPT_SystemInterface
{
public:
    // methods
                NPT_PSPSystem();
               ~NPT_PSPSystem();
    NPT_Result  GetProcessId(NPT_Integer& id);
    NPT_Result  GetCurrentTimeStamp(NPT_TimeStamp& now);
    NPT_Result  Sleep(NPT_UInt32 microseconds);
    NPT_Result  SetRandomSeed(unsigned int seed);
    NPT_Integer GetRandomInteger();
    
private:
    bool m_bRandomized;
};

/*----------------------------------------------------------------------
|       NPT_PSPSystem::NPT_PSPSystem
+---------------------------------------------------------------------*/
NPT_PSPSystem::NPT_PSPSystem()
: m_bRandomized(false)
{
}

/*----------------------------------------------------------------------
|       NPT_PSPSystem::~NPT_PSPSystem
+---------------------------------------------------------------------*/
NPT_PSPSystem::~NPT_PSPSystem()
{
}

/*----------------------------------------------------------------------
|       NPT_PSPSystem::GetProcessId
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSystem::GetProcessId(NPT_Integer& id)
{
    //id = getpid();
    id = 0;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPSystem::GetCurrentTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSystem::GetCurrentTimeStamp(NPT_TimeStamp& now)
{
    // get the current time
	ScePspDateTime clock;
	sceRtcGetCurrentClock(&clock, 0);
	
	// converts to POSIX time to get the number of seconds since 1970
	time_t time;
	sceRtcGetTime_t(&clock, &time);
	
	// converts back to Psp date, it should be truncated
	// we'll use that to get the number of nanoseconds
	ScePspDateTime clock_trunk;
	sceRtcSetTime_t(&clock_trunk, time);
	
    //_ftime(&time_stamp);
    now.m_Seconds     = time;
    now.m_NanoSeconds = (sceRtcGetMicrosecond(&clock)-sceRtcGetMicrosecond(&clock_trunk))*1000;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPSystem::Sleep
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPSystem::Sleep(NPT_UInt32 microseconds)
{
    sceKernelDelayThread(microseconds);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPSystem::SetRandomSeed
+---------------------------------------------------------------------*/
NPT_Result  
NPT_PSPSystem::SetRandomSeed(unsigned int seed)
{
    srand(seed);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_System::NPT_System
+---------------------------------------------------------------------*/
NPT_Integer 
NPT_PSPSystem::GetRandomInteger()
{
    if (m_bRandomized == false) {
        NPT_TimeStamp now;
        GetCurrentTimeStamp(now);
        SetRandomSeed( now.m_Seconds );
        m_bRandomized = true;
    }
    NPT_Integer val = 0;
    val = rand();
    return val;
}

/*----------------------------------------------------------------------
|       NPT_System::NPT_System
+---------------------------------------------------------------------*/
NPT_System::NPT_System()
{
    m_Delegate = new NPT_PSPSystem();
}

