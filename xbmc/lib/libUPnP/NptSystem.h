/*****************************************************************
|
|   Neptune - System
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_SYSTEM_H_
#define _NPT_SYSTEM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptTime.h"

/*----------------------------------------------------------------------
|   NPT_System
+---------------------------------------------------------------------*/
class NPT_System
{
public:
    // methods
    static NPT_Result  GetProcessId(NPT_Integer& id);
    static NPT_Result  GetCurrentTimeStamp(NPT_TimeStamp& now);
    static NPT_Result  Sleep(const NPT_TimeInterval& duration);
    static NPT_Result  SleepUntil(const NPT_TimeStamp& when);
    static NPT_Result  SetRandomSeed(unsigned int seed);
    static NPT_Integer GetRandomInteger();
    
protected:
    // constructor
    NPT_System() {}
};

#endif // _NPT_SYSTEM_H_
