/*****************************************************************
|
|      Neptune - System :: Null/Stub Implementation
|
|      (c) 2001-2016 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptSystem.h"

/*----------------------------------------------------------------------
|   NPT_System::GetProcessId
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetProcessId(NPT_UInt32& id)
{
    id = 0;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::GetCurrentTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetCurrentTimeStamp(NPT_TimeStamp& now)
{
    now.SetNanos(0);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::Sleep
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::Sleep(const NPT_TimeInterval& /* duration */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::SleepUntil
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::SleepUntil(const NPT_TimeStamp& /* when */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::SetRandomSeed
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::SetRandomSeed(unsigned int /* seed */)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::GetRandomInteger
+---------------------------------------------------------------------*/
NPT_UInt32
NPT_System::GetRandomInteger()
{
    return 0;
}
