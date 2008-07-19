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

/* PRIVATE */

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

/* type definitions */
typedef struct CP_SThreadPoolTAG CP_SThreadPool;
typedef struct CP_STPTimerQueueTAG *CP_TPTimerItem;

/* function pointer definitions */
typedef void (*CP_TPfnJob)(void *, void *);

/* Interface */
CP_SThreadPool *CP_ThreadPool_Create(unsigned int uiMaxThreads);
unsigned int CP_ThreadPool_AddRef(CP_SThreadPool *pTPThis);
unsigned int CP_ThreadPool_Release(CP_SThreadPool *pTPThis);
void CP_ThreadPool_QueueWorkItem(CP_SThreadPool *pTPThis,
                                 CP_TPfnJob pfnCallback,
                                 void *arg1, void *arg2);
unsigned int CP_ThreadPool_GetPendingJobs(CP_SThreadPool *pTPThis);
CP_TPTimerItem CP_ThreadPool_QueueTimerItem(CP_SThreadPool *pTPThis,
                                            unsigned int uiMSTimeWait,
                                            CP_TPfnJob pfnCallback,
                                            void *arg1, void *arg2);
void CP_ThreadPool_DeleteTimerItem(CP_SThreadPool *pTPThis,
                                   CP_TPTimerItem tiDeadItem);
void CP_ThreadPool_ResetTimerItem(CP_SThreadPool *pTPThis,
                                  CP_TPTimerItem tiResetItem);

#ifdef __cplusplus
}
#endif

#endif /* _THREADPOOL_H */

