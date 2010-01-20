/*****************************************************************
|
|      Neptune - Queue :: PSP Implementation
|
|      (c) 2001-2002 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTypes.h"
#include "NptQueue.h"
//#include "NptThreads.h"
#include "NptList.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|       NPT_PSPQueue
+---------------------------------------------------------------------*/
class NPT_PSPQueue : public NPT_GenericQueue
{
public:
    // methods
               NPT_PSPQueue(NPT_Cardinal max_items);
              ~NPT_PSPQueue();
    NPT_Result Push(NPT_QueueItem* item); 
    NPT_Result Pop(NPT_QueueItem*& item, NPT_Boolean blocking);


private:
    // members
    NPT_Cardinal             m_MaxItems;
    //NPT_Mutex          	 	 m_Mutex;
    //pthread_cond_t           m_CanPushOrPopCondition;
    NPT_List<NPT_QueueItem*> m_Items;
};

/*----------------------------------------------------------------------
|       NPT_PSPQueue::NPT_PSPQueue
+---------------------------------------------------------------------*/
NPT_PSPQueue::NPT_PSPQueue(NPT_Cardinal max_items) : 
    m_MaxItems(max_items)
{
    NPT_Debug(":: NPT_PSPQueue::NPT_PSPQueue\n");

    //pthread_cond_init(&m_CanPushOrPopCondition, NULL);
}

/*----------------------------------------------------------------------
|       NPT_PSPQueue::~NPT_PSPQueue()
+---------------------------------------------------------------------*/
NPT_PSPQueue::~NPT_PSPQueue()
{
    // destroy resources
    //pthread_cond_destroy(&m_CanPushOrPopCondition);
}

/*----------------------------------------------------------------------
|       NPT_PSPQueue::Push
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPQueue::Push(NPT_QueueItem* item)
{
    // lock the mutex that protects the list
    m_Items.Lock();

    // check that we have not exceeded the max
    //if (m_MaxItems) {
    //    while (m_Items.GetItemCount() >= m_MaxItems) {
    //        // wait until some items have been removed
    //        //NPT_Debug(":: NPT_PSPQueue::Push - waiting for queue to empty\n");
    //        pthread_cond_wait(&m_CanPushOrPopCondition, &m_Mutex);
    //    }
    //}

    // add the item to the list
    m_Items.Add(item);

    // if the list was previously empty, signal the condition
    // to wake up the waiting thread
    //if (m_Items.GetItemCount() == 1) {    
    //    pthread_cond_signal(&m_CanPushOrPopCondition);
    //}

    // unlock the mutex
    m_Items.Unlock();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPQueue::Pop
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPQueue::Pop(NPT_QueueItem*& item, NPT_Boolean blocking)
{
    // lock the mutex that protects the list
    m_Items.Lock();

    NPT_Result result;
    //if (blocking) {
    //    while ((result = m_Items.PopHead(item)) == NPT_ERROR_LIST_EMPTY) {
    //        // no item in the list, wait for one
    //        //NPT_Debug(":: NPT_PSPQueue::Pop - waiting for queue to fill up\n");
    //        pthread_cond_wait(&m_CanPushOrPopCondition, &m_Mutex);
    //    }
    //} else {
        result = m_Items.PopHead(item);
    //}
    
    // if the list was previously full, signal the condition
    // to wake up the waiting thread
    //if (m_MaxItems && (result == NPT_SUCCESS)) {
    //    if (m_Items.GetItemCount() == m_MaxItems-1) {    
    //        pthread_cond_signal(&m_CanPushOrPopCondition);
    //    }
    //}

    // unlock the mutex
    m_Items.Unlock();
 
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
    return new NPT_PSPQueue(max_items);
}




