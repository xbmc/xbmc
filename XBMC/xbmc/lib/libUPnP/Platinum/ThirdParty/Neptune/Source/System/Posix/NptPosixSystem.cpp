/*****************************************************************
|
|      Neptune - System :: Posix Implementation
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptSystem.h"
#include "NptResults.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   NPT_PosixSystem
+---------------------------------------------------------------------*/
class NPT_PosixSystem
{
public:
    // class variables
    static NPT_PosixSystem System;
    
    // methods
    NPT_PosixSystem();
   ~NPT_PosixSystem();

    // members
    pthread_mutex_t m_SleepMutex;
    pthread_cond_t  m_SleepCondition;
};
NPT_PosixSystem NPT_PosixSystem::System;

/*----------------------------------------------------------------------
|   NPT_PosixSystem::NPT_PosixSystem
+---------------------------------------------------------------------*/
NPT_PosixSystem::NPT_PosixSystem()
{
    pthread_mutex_init(&m_SleepMutex, NULL);
    pthread_cond_init(&m_SleepCondition, NULL);
}

/*----------------------------------------------------------------------
|   NPT_PosixSystem::~NPT_PosixSystem
+---------------------------------------------------------------------*/
NPT_PosixSystem::~NPT_PosixSystem()
{
    pthread_cond_destroy(&m_SleepCondition);
    pthread_mutex_destroy(&m_SleepMutex);
}

/*----------------------------------------------------------------------
|   NPT_System::GetProcessId
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetProcessId(NPT_UInt32& id)
{
    id = getpid();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::GetCurrentTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::GetCurrentTimeStamp(NPT_TimeStamp& now)
{
    struct timeval now_tv;

    // get current time from system
    if (gettimeofday(&now_tv, NULL)) {
        now.m_Seconds     = 0;
        now.m_NanoSeconds = 0;
        return NPT_FAILURE;
    }
    
    // convert format
    now.m_Seconds     = now_tv.tv_sec;
    now.m_NanoSeconds = now_tv.tv_usec * 1000;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::Sleep
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::Sleep(const NPT_TimeInterval& duration)
{
    struct timespec time_req;
    struct timespec time_rem;
    int             result;

    // setup the time value
    time_req.tv_sec  = duration.m_Seconds;
    time_req.tv_nsec = duration.m_NanoSeconds;

    // sleep
    do {
        result = nanosleep(&time_req, &time_rem);
        time_req = time_rem;
    } while (result == -1 && errno == EINTR);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_System::SleepUntil
+---------------------------------------------------------------------*/
NPT_Result
NPT_System::SleepUntil(const NPT_TimeStamp& when)
{
    struct timespec timeout;
    struct timeval  now;
    int             result;

    // get current time from system
    if (gettimeofday(&now, NULL)) {
        return NPT_FAILURE;
    }

    // setup timeout
    timeout.tv_sec  = now.tv_sec + when.m_Seconds;
    timeout.tv_nsec = now.tv_usec * 1000 + when.m_NanoSeconds;

    if (timeout.tv_nsec >= 1000000000) {
        timeout.tv_sec  += timeout.tv_nsec / 1000000000;
        timeout.tv_nsec %= 1000000000;
    }

    // sleep
    do {
        result = pthread_cond_timedwait(&NPT_PosixSystem::System.m_SleepCondition, 
                                        &NPT_PosixSystem::System.m_SleepMutex, 
                                        &timeout);
        if (result == ETIMEDOUT) {
            return NPT_SUCCESS;
        }
    } while (result == EINTR);

    return NPT_FAILURE;
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
|   NPT_System::GetRandomInteger
+---------------------------------------------------------------------*/
NPT_UInt32 
NPT_System::GetRandomInteger()
{
    static bool seeded = false;
    if (seeded == false) {
        NPT_TimeStamp now;
        GetCurrentTimeStamp(now);
        SetRandomSeed(now.m_NanoSeconds);
        seeded = true;
    }

    return rand();
}
