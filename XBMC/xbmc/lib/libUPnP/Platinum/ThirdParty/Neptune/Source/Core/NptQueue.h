/*****************************************************************
|
|   Neptune - Queue
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_QUEUE_H_
#define _NPT_QUEUE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptConstants.h"

/*----------------------------------------------------------------------
|   NPT_QueueItem
+---------------------------------------------------------------------*/
class NPT_QueueItem;

/*----------------------------------------------------------------------
|   NPT_GenericQueue
+---------------------------------------------------------------------*/
class NPT_GenericQueue
{
 public:
    // class methods
    static NPT_GenericQueue* CreateInstance(NPT_Cardinal max_items = 0);

    // methods
    virtual           ~NPT_GenericQueue() {}
    virtual NPT_Result Push(NPT_QueueItem* item,
                            bool           blocking = true) = 0; 
    virtual NPT_Result Pop(NPT_QueueItem*& item, 
                           bool            blocking = true) = 0;

 protected:
    // methods
    NPT_GenericQueue() {}
};

/*----------------------------------------------------------------------
|   NPT_Queue
+---------------------------------------------------------------------*/
template <class T>
class NPT_Queue
{
 public:
    // methods
    NPT_Queue(NPT_Cardinal max_items = 0) :
        m_Delegate(NPT_GenericQueue::CreateInstance(max_items)) {}
    virtual ~NPT_Queue<T>() { delete m_Delegate; }
    virtual NPT_Result Push(T* item, bool blocking = true) {
        return m_Delegate->Push(reinterpret_cast<NPT_QueueItem*>(item),
                                blocking);
    }
    virtual NPT_Result Pop(T*& item, bool blocking = true) {
        return m_Delegate->Pop(reinterpret_cast<NPT_QueueItem*&>(item), 
                               blocking);
    }

 protected:
    // members
    NPT_GenericQueue* m_Delegate;
};

#endif // _NPT_QUEUE_H_
