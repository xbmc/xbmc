/* threadpool class
 *
 * Copyright (c) 2004 David Hammerton
 * crazney@crazney.net
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "portability.h"
#include "thread.h"
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include "debug.h"

#include "private.h"
#include "threadpool.h"

#define DEFAULT_DEBUG_CHANNEL "threadpool"

/* no need for this now, as it just uses up a thread. */
/*#define USE_TIMER_THREAD */

/* PRIVATE */

/* helper functions */

#ifdef USE_TIMER_THREAD
static void TP_DeleteTimerItem(CP_SThreadPool *pTPThis,
                               CP_STPTimerQueue *pTPTQDead)
{
    if (pTPThis->pTPTQTail == pTPTQDead)
        pTPThis->pTPTQTail = pTPTQDead->prev;
    if (pTPTQDead->prev)
        pTPTQDead->prev->next = pTPTQDead->next;
    if (pTPTQDead->next)
        pTPTQDead->next->prev = pTPTQDead->prev;

    free(pTPTQDead);
}
#endif

/* Threads life */
static void TP_ThreadsLife(void *arg1)
{
    CP_SThreadPool *pTPOwner = (CP_SThreadPool *)arg1;

    ts_mutex_lock(pTPOwner->mtJobQueueMutex);
    while (1)
    {
        CP_STPJobQueue *pTPJQOurJobItem;
        while (!pTPOwner->pTPJQHead)
        {
            if (pTPOwner->uiDying)
            {
                TRACE("(tid: %i)\n", getpid());
                ts_mutex_unlock(pTPOwner->mtJobQueueMutex);
                ts_exit();
            }
            ts_condition_wait(pTPOwner->cndJobPosted, pTPOwner->mtJobQueueMutex);
        }

        pTPJQOurJobItem = pTPOwner->pTPJQHead;
        if (pTPOwner->pTPJQHead->next) pTPOwner->pTPJQHead->next->prev = NULL;
        if (pTPOwner->pTPJQTail == pTPOwner->pTPJQHead) pTPOwner->pTPJQTail = NULL;
        pTPOwner->pTPJQHead = pTPOwner->pTPJQHead->next;
        pTPOwner->uiJobCount--;

        ts_mutex_unlock(pTPOwner->mtJobQueueMutex);

        pTPJQOurJobItem->fnJobCallback(pTPJQOurJobItem->arg1, pTPJQOurJobItem->arg2);

        free(pTPJQOurJobItem);
        ts_mutex_lock(pTPOwner->mtJobQueueMutex);
    }
    ts_exit();
}

/* Timer Threads life */
#define TIME_TO_OFF(curtime, timer) \
    ((int)timer->uiTimeWait - (int)(curtime - timer->uiTimeSet))

#ifdef USE_TIMER_THREAD
static void TP_TimerThreadLife(void *arg1)
{
    CP_SThreadPool *pTPOwner = (CP_SThreadPool *)arg1;

    ts_mutex_lock(pTPOwner->mtTimerQueueMutex);
    while (1)
    {
        CP_STPTimerQueue *pTPTQCurrent, *pTPTQBest;
        int iTimeBest = 0;
        unsigned int uiCurTime;

        uiCurTime = CP_GetTickCount();
        pTPTQBest = NULL;
        pTPTQCurrent = pTPOwner->pTPTQTail;
        while (pTPTQCurrent)
        {
            if (!pTPTQBest || TIME_TO_OFF(uiCurTime, pTPTQCurrent) <= iTimeBest)
            {
                iTimeBest = TIME_TO_OFF(uiCurTime, pTPTQCurrent);
                if (iTimeBest < 0) iTimeBest = 0;
                pTPTQBest = pTPTQCurrent;
            }
            pTPTQCurrent = pTPTQCurrent->prev;
        }
        if (pTPTQBest)
        {
            struct timespec ts;
            struct timeval tvNow;
            int ret;

#ifdef THREADS_POSIX
            gettimeofday(&tvNow, NULL);

            ts.tv_sec = tvNow.tv_sec + (iTimeBest / 1000);
            ts.tv_nsec = ((tvNow.tv_usec + (iTimeBest % 1000) * 1000) * 1000);

            ret = pthread_cond_timedwait(&pTPOwner->cndTimerPosted,
                                         &pTPOwner->mtTimerQueueMutex,
                                         &ts);
            if (pTPOwner->uiDying)
            {
                ts_mutex_unlock(pTPOwner->mtTimerQueueMutex);
                ts_exit();
            }
            if (ret == ETIMEDOUT)
#elif THREADS_WIN32
#error IMPLEMENT ME
#else
#error IMPLEMENT ME
#endif
            {
                void (*fnTimerCallback)(void *, void *);
                void *arg1, *arg2;

                fnTimerCallback = pTPTQBest->fnTimerCallback;
                arg1 = pTPTQBest->arg1;
                arg2 = pTPTQBest->arg2;

                /* delete the item. can't use CP_ThreadPool_DeleteTimerItem
                 * as that sets off a chain reaction.
                 * so use some helper code
                 */
                TP_DeleteTimerItem(pTPOwner, pTPTQBest);

                ts_mutex_unlock(pTPOwner->mtTimerQueueMutex);

                /* this can block, so we should unlock */
                /* actually it can't block now, but it will be able to soon */
                CP_ThreadPool_QueueWorkItem(pTPOwner, fnTimerCallback,
                                            arg1, arg2);

                ts_mutex_lock(pTPOwner->mtTimerQueueMutex);
            }
        }
        else
        {
            ts_condition_wait(pTPOwner->cndTimerPosted,
                              pTPOwner->mtTimerQueueMutex);
            if (pTPOwner->uiDying)
            {
                ts_mutex_unlock(pTPOwner->mtTimerQueueMutex);
                ts_exit();
            }
        }
    }
    ts_exit();
}
#endif

/* Interface */

CP_SThreadPool *CP_ThreadPool_Create(unsigned int uiMaxThreads)
{
    unsigned int i;
    CP_SThreadPool *pTPNewThreadPool = malloc(sizeof(CP_SThreadPool));

    pTPNewThreadPool->uiRef = 1;
    pTPNewThreadPool->uiMaxThreads = uiMaxThreads >= 3 ? uiMaxThreads : 3;
    /* FIXME: we always need 3 threads.
     * 1. SP listen thread
     * 2. Worker function thread.
     * 3. Worked timer thread.
     */
    pTPNewThreadPool->prgptThreads = malloc(sizeof(ts_thread) *
            pTPNewThreadPool->uiMaxThreads);
    pTPNewThreadPool->uiThreadCount = pTPNewThreadPool->uiMaxThreads;
    pTPNewThreadPool->uiDying = 0;

    ts_mutex_create(pTPNewThreadPool->mtJobQueueMutex);
    ts_condition_create(pTPNewThreadPool->cndJobPosted);

    pTPNewThreadPool->pTPJQHead = NULL;
    pTPNewThreadPool->pTPJQTail = NULL;

    ts_mutex_create(pTPNewThreadPool->mtTimerQueueMutex);
    ts_condition_create(pTPNewThreadPool->cndTimerPosted);
    pTPNewThreadPool->pTPTQTail = NULL;

    /* start the threads */
#if USE_TIMER_THREAD
    ts_thread_create(pTPNewThreadPool->prgptThreads[0], &TP_TimerThreadLife,
                     pTPNewThreadPool);
    for (i = 1; i < pTPNewThreadPool->uiThreadCount; i++)
#else
    for (i = 0; i < pTPNewThreadPool->uiThreadCount; i++)
#endif
    {
        ts_thread_create(pTPNewThreadPool->prgptThreads[i], &TP_ThreadsLife,
                         pTPNewThreadPool);
    }
    return pTPNewThreadPool;
}

unsigned int CP_ThreadPool_AddRef(CP_SThreadPool *pTPThis)
{
    return ++pTPThis->uiRef;
}

unsigned int CP_ThreadPool_Release(CP_SThreadPool *pTPThis)
{
    unsigned int i;
    CP_STPJobQueue *pTPJQTmpJob;
    if (--pTPThis->uiRef) return pTPThis->uiRef;

    /* remove all jobs */
    ts_mutex_lock(pTPThis->mtJobQueueMutex);
    pTPJQTmpJob = pTPThis->pTPJQTail;
    while (pTPJQTmpJob)
    {
        pTPThis->pTPJQTail = pTPJQTmpJob->prev;
        free(pTPJQTmpJob);
        pTPJQTmpJob = pTPThis->pTPJQTail;
    }
    pTPThis->pTPJQHead = NULL;
    ts_mutex_unlock(pTPThis->mtJobQueueMutex);

    pTPThis->uiDying = 1;
    ts_condition_signal_all(pTPThis->cndJobPosted);
    ts_condition_signal_all(pTPThis->cndTimerPosted);
    for (i = 0; i < pTPThis->uiThreadCount; i++)
        ts_thread_join(pTPThis->prgptThreads[i]);
    free(pTPThis->prgptThreads);

    ts_condition_destroy(pTPThis->cndJobPosted);
    ts_mutex_destroy(pTPThis->mtJobQueueMutex);

    ts_condition_destroy(pTPThis->cndTimerPosted);
    ts_mutex_destroy(pTPThis->mtTimerQueueMutex);

    FIXME("free job queue and timer queue\n");

    free(pTPThis);
    return 0;
}

void CP_ThreadPool_QueueWorkItem(CP_SThreadPool *pTPThis, CP_TPfnJob pfnCallback,
                                 void *arg1, void *arg2)
{
    ts_mutex_lock(pTPThis->mtJobQueueMutex);

    CP_STPJobQueue *pTPJQNewJob = malloc(sizeof(CP_STPJobQueue));

    pTPJQNewJob->fnJobCallback = pfnCallback;
    pTPJQNewJob->arg1 = arg1;
    pTPJQNewJob->arg2 = arg2;
    pTPJQNewJob->prev = NULL;
    pTPJQNewJob->next = NULL;
    if (!pTPThis->pTPJQHead)
    {
        pTPThis->pTPJQHead = pTPJQNewJob;
    }
    else
    {
        pTPThis->pTPJQTail->next = pTPJQNewJob;
    }
    pTPJQNewJob->prev = pTPThis->pTPJQTail;
    pTPThis->pTPJQTail = pTPJQNewJob;
    pTPThis->uiJobCount++;

    ts_mutex_unlock(pTPThis->mtJobQueueMutex);
    ts_condition_signal(pTPThis->cndJobPosted);
}

unsigned int CP_ThreadPool_GetPendingJobs(CP_SThreadPool *pTPThis)
{
    return pTPThis->uiJobCount;
}

#if USE_TIMER_THREAD
CP_TPTimerItem CP_ThreadPool_QueueTimerItem(CP_SThreadPool *pTPThis,
                                            unsigned int uiMSTimeWait,
                                            CP_TPfnJob pfnCallback,
                                            void *arg1, void *arg2)
{
    CP_STPTimerQueue *pTPTQNew;

    pTPTQNew = malloc(sizeof(CP_STPTimerQueue));
    pTPTQNew->uiTimeSet = CP_GetTickCount();
    pTPTQNew->uiTimeWait = uiMSTimeWait;
    pTPTQNew->fnTimerCallback = pfnCallback;
    pTPTQNew->arg1 = arg1;
    pTPTQNew->arg2 = arg2;

    ts_mutex_lock(pTPThis->mtTimerQueueMutex);

    pTPTQNew->prev = pTPThis->pTPTQTail;
    if (pTPThis->pTPTQTail)
        pTPThis->pTPTQTail->next = pTPTQNew;
    pTPThis->pTPTQTail = pTPTQNew;

    ts_mutex_unlock(pTPThis->mtTimerQueueMutex);
    ts_condition_signal(pTPThis->cndTimerPosted);
    return pTPTQNew;
}

void CP_ThreadPool_DeleteTimerItem(CP_SThreadPool *pTPThis,
                                   CP_TPTimerItem tiDeadItem)
{
    /* FIXME: need to check if its still in the list of timers */
    CP_STPTimerQueue *pTPTQDead = tiDeadItem;

    ts_mutex_lock(pTPThis->mtTimerQueueMutex);

    TP_DeleteTimerItem(pTPThis, pTPTQDead);

    ts_mutex_unlock(pTPThis->mtTimerQueueMutex);
    ts_condition_signal(pTPThis->cndTimerPosted);
    return;
}

void CP_ThreadPool_ResetTimerItem(CP_SThreadPool *pTPThis,
                                  CP_TPTimerItem tiResetItem)
{
    /* FIXME: need to check if its still in the list of timers */
    CP_STPTimerQueue *pTPTQReset = tiResetItem;

    ts_mutex_lock(pTPThis->mtTimerQueueMutex);

    pTPTQReset->uiTimeSet = CP_GetTickCount();

    ts_mutex_unlock(pTPThis->mtTimerQueueMutex);
    ts_condition_signal(pTPThis->cndTimerPosted);
    return;
}
#endif
