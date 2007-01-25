/*****************************************************************
|
|   Neptune - Threads :: Win32 Implementation
|
|   (c) 2001-2003 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTypes.h"
#include "NptThreads.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   NPT_Win32Mutex
+---------------------------------------------------------------------*/
class NPT_Win32Mutex : public NPT_MutexInterface
{
public:
    // methods
             NPT_Win32Mutex();
    virtual ~NPT_Win32Mutex();

    // NPT_Mutex methods
    virtual NPT_Result Lock();
    virtual NPT_Result Unlock();

private:
    // members
    HANDLE m_Handle;
};

/*----------------------------------------------------------------------
|   NPT_Win32CriticalSection
+---------------------------------------------------------------------*/
class NPT_Win32CriticalSection
{
public:
    // methods
    NPT_Win32CriticalSection();
   ~NPT_Win32CriticalSection();

    // NPT_Mutex methods
    NPT_Result Lock();
    NPT_Result Unlock();

private:
    // members
    CRITICAL_SECTION m_CriticalSection;
};
