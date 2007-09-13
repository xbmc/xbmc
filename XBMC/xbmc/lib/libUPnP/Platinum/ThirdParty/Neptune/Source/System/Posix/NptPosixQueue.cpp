/*****************************************************************
|
|      Neptune - Queue :: Posix Implementation
|
|      (c) 2001-2002 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <pthread.h>

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptQueue.h"
#include "NptThreads.h"
#include "NptList.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|       NPT_PosixQueue
+---------------------------------------------------------------------*/
class NPT_PosixQueue : public NPT_GenericQueue
{
public:
    // methods
               NPT_PosixQueue(NPT_Cardinal max_items);
              ~NPT_PosixQueue();
    NPT_Result Push(NPT_QueueItem* item, bool blocking); 
    NPT_Result Pop(NPT_QueueItem*& item, bool blocking);


private:
    // members
    NPT_Cardinal             m_MaxItems;
    pthread_mutex_t          m_Mutex;
    pthread_cond_t           m_CanPushOrPopCondition;
    NPT_List<NPT_QueueItem*> m_Items;
};

/*----------------------------------------------------------------------
|       NPT_PosixQueue::NPT_PosixQueue
+---------------------------------------------------------------------*/
NPT_PosixQueue::NPT_PosixQueue(NPT_Cardinal max_items) : 
    m_MaxItems(max_items)
{
    NPT_Debug(":: NPT_PosixQueue::NPT_PosixQueue\n");

    pthread_mutex_init(&m_Mutex, NULL);
    pthread_cond_init(&m_CanPushOrPopCondition, NULL);
}

/*----------------------------------------------------------------------
|       NPT_PosixQueue::~NPT_PosixQueue()
+---------------------------------------------------------------------*/
NPT_PosixQueue::~NPT_PosixQueue()
{
    // destroy resources
    pthread_cond_destroy(&m_CanPushOrPopCondition);
    pthread_mutex_destroy(&m_Mutex);
}

/*----------------------------------------------------------------------
|       NPT_PosixQueue::Push
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixQueue::Push(NPT_QueueItem* item, bool blocking)
{
    // lock the mutex that protects the list
    if (pthread_mutex_lock(&m_Mutex)) {
        return NPT_FAILURE;
    }

    // check that we have not exceeded the max
    if (m_MaxItems) {
        if (!blocking) {
            pthread_mutex_unlock(&m_Mutex);
            return NPT_FAILURE;
        }

        while (m_Items.GetItemCount() >= m_MaxItems) {
            // wait until some items have been removed
            //NPT_Debug(":: NPT_PosixQueue::Push - waiting for queue to empty\n");
            pthread_cond_wait(&m_CanPushOrPopCondition, &m_Mutex);
        }
    }

    // add the item to the list
    m_Items.Add(item);

    // if the list was previously empty, signal the condition
    // to wake up the waiting thread
    if (m_Items.GetItemCount() == 1) {    
        pthread_cond_signal(&m_CanPushOrPopCondition);
    }

    // unlock the mutex
    pthread_mutex_unlock(&m_Mutex);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PosixQueue::Pop
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixQueue::Pop(NPT_QueueItem*& item, bool blocking)
{
    // lock the mutex that protects the list
    if (pthread_mutex_lock(&m_Mutex)) {
        return NPT_FAILURE;
    }

    NPT_Result result;
    if (blocking) {
        while ((result = m_Items.PopHead(item)) == NPT_ERROR_LIST_EMPTY) {
            // no item in the list, wait for one
            //NPT_Debug(":: NPT_PosixQueue::Pop - waiting for queue to fill up\n");
            pthread_cond_wait(&m_CanPushOrPopCondition, &m_Mutex);
        }
    } else {
        result = m_Items.PopHead(item);
    }
    
    // if the list was previously full, signal the condition
    // to wake up the waiting thread
    if (m_MaxItems && (result == NPT_SUCCESS)) {
        if (m_Items.GetItemCount() == m_MaxItems-1) {    
            pthread_cond_signal(&m_CanPushOrPopCondition);
        }
    }

    // unlock the mutex
    pthread_mutex_unlock(&m_Mutex);
 
    return result;
}

/*----------------------------------------------------------------------
|       NPT_GenericQueue::CreateInstance
+---------------------------------------------------------------------*/
NPT_GenericQueue*
NPT_GenericQueue::CreateInstance(NPT_Cardinal max_items)
{
    NPT_Debug(":: NPT_GenericQueue::CreateInstance - queue max_items = %ld\n", 
           max_items);
    return new NPT_PosixQueue(max_items);
}




