/*****************************************************************
|
|   Neptune - Queue :: Win32 Implementation
|
|   (c) 2001-2002 Gilles Boccon-Gibod
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

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptQueue.h"
#include "NptThreads.h"
#include "NptList.h"
#include "NptDebug.h"
#include "NptWin32Threads.h"

/*----------------------------------------------------------------------
|   NPT_Win32Queue
+---------------------------------------------------------------------*/
class NPT_Win32Queue : public NPT_GenericQueue
{
public:
    // methods
               NPT_Win32Queue(NPT_Cardinal max_items);
              ~NPT_Win32Queue();
    NPT_Result Push(NPT_QueueItem* item, bool blocking); 
    NPT_Result Pop(NPT_QueueItem*& item, bool blocking);


private:
    // members
    NPT_Cardinal             m_MaxItems;
    NPT_Win32CriticalSection m_Mutex;
    HANDLE                   m_CanPushOrPopCondition;
    NPT_List<NPT_QueueItem*> m_Items; // should be volatile ?
};

/*----------------------------------------------------------------------
|   NPT_Win32Queue::NPT_Win32Queue
+---------------------------------------------------------------------*/
NPT_Win32Queue::NPT_Win32Queue(NPT_Cardinal max_items) : 
    m_MaxItems(max_items)
{
    NPT_Debug(":: NPT_Win32Queue::NPT_Win32Queue\n");
    m_CanPushOrPopCondition = CreateEvent(NULL, TRUE, FALSE, NULL);
}

/*----------------------------------------------------------------------
|   NPT_Win32Queue::~NPT_Win32Queue()
+---------------------------------------------------------------------*/
NPT_Win32Queue::~NPT_Win32Queue()
{
    // destroy resources
    CloseHandle(m_CanPushOrPopCondition);
}

/*----------------------------------------------------------------------
|   NPT_Win32Queue::Push
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Queue::Push(NPT_QueueItem* item, bool blocking)
{
    // lock the mutex that protects the list
    NPT_CHECK(m_Mutex.Lock());

    // check that we have not exceeded the max
    if (m_MaxItems) {
        while (m_Items.GetItemCount() >= m_MaxItems) {
            if (!blocking) {
                m_Mutex.Unlock();
                return NPT_FAILURE;
            }

            // we must wait until some items have been removed

            // unlock the mutex so that another thread can pop
            m_Mutex.Unlock();

            // wait for the condition to signal that we can push
            DWORD result;
            result = WaitForSingleObject(m_CanPushOrPopCondition, INFINITE);
            if (result != WAIT_OBJECT_0) return NPT_FAILURE;

            // relock the mutex so that we can check the list again
            NPT_CHECK(m_Mutex.Lock());

            // reset the condition to false (the list will be full again)
            ResetEvent(m_CanPushOrPopCondition);
        }
    }

    // add the item to the list
    m_Items.Add(item);

    // if the list was previously empty, signal the condition
    // to wake up the threads waiting to pop
    if (m_Items.GetItemCount() == 1) {    
        SetEvent(m_CanPushOrPopCondition);
    }

    // unlock the mutex
    m_Mutex.Unlock();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Win32Queue::Pop
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Queue::Pop(NPT_QueueItem*& item, bool blocking)
{
    // lock the mutex that protects the list
    NPT_CHECK(m_Mutex.Lock());

    NPT_Result result;
    if (blocking) {
        while ((result = m_Items.PopHead(item)) == NPT_ERROR_LIST_EMPTY) {
            // no item in the list, wait for one

            // unlock the mutex so that another thread can push
            m_Mutex.Unlock();

            // wait for the condition to signal that we can pop
            DWORD result;
            result = WaitForSingleObject(m_CanPushOrPopCondition, INFINITE);
            if (result != WAIT_OBJECT_0) return NPT_FAILURE;

            // relock the mutex so that we can check the list again
            NPT_CHECK(m_Mutex.Lock());

            // reset the condition to false (the list will be empty again)
            ResetEvent(m_CanPushOrPopCondition);
        }
    } else {
        result = m_Items.PopHead(item);
    }
    
    if (m_MaxItems && (result == NPT_SUCCESS)) {
        // if the list was previously full, signal the condition
        // to wake up the threads waiting to push
        if (m_Items.GetItemCount() == m_MaxItems-1) {    
            SetEvent(m_CanPushOrPopCondition);
        }
    }

    // unlock the mutex
    m_Mutex.Unlock();
 
    return result;
}

/*----------------------------------------------------------------------
|   NPT_GenericQueue::CreateInstance
+---------------------------------------------------------------------*/
NPT_GenericQueue*
NPT_GenericQueue::CreateInstance(NPT_Cardinal max_items)
{
    NPT_Debug(":: NPT_GenericQueue::CreateInstance - queue max_items = %ld\n", 
           max_items);
    return new NPT_Win32Queue(max_items);
}

