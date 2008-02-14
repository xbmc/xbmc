/*****************************************************************
|
|   Neptune - Simple Message Queue
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptSimpleMessageQueue.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   NPT_SimpleMessageCapsule
+---------------------------------------------------------------------*/
struct NPT_SimpleMessageCapsule
{
    NPT_SimpleMessageCapsule(NPT_Message* message, 
                             NPT_MessageHandler* handler) :
        m_Message(message), m_Handler(handler) {}
    NPT_Message*        m_Message;
    NPT_MessageHandler* m_Handler;
};

/*----------------------------------------------------------------------
|   NPT_SimpleMessageQueue::NPT_SimpleMessageQueue
+---------------------------------------------------------------------*/
NPT_SimpleMessageQueue::NPT_SimpleMessageQueue()
{
}

/*----------------------------------------------------------------------
|   NPT_SimpleMessageQueue::~NPT_SimpleMessageQueue
+---------------------------------------------------------------------*/
NPT_SimpleMessageQueue::~NPT_SimpleMessageQueue()
{
    // empty the queue
    // TBD
}

/*----------------------------------------------------------------------
|   NPT_SimpleMessageQueue::QueueMessage
+---------------------------------------------------------------------*/
NPT_Result
NPT_SimpleMessageQueue::QueueMessage(NPT_Message*        message, 
                                     NPT_MessageHandler* handler)
{
    // push the message on the queue, with the handler reference
    return m_Queue.Push(new NPT_SimpleMessageCapsule(message, handler));
}

/*----------------------------------------------------------------------
|   NPT_SimpleMessageQueue::PumpMessage
+---------------------------------------------------------------------*/
NPT_Result
NPT_SimpleMessageQueue::PumpMessage(NPT_Timeout timeout /* = NPT_TIMEOUT_INFINITE */)
{
    NPT_SimpleMessageCapsule* capsule;
    NPT_Result result = m_Queue.Pop(capsule, timeout);
    if (NPT_SUCCEEDED(result) && capsule) {
        if (capsule->m_Handler && capsule->m_Message) {
            result = capsule->m_Handler->HandleMessage(capsule->m_Message);
            //result = capsule->m_Message->Dispatch(capsule->m_Handler);
        }
        delete capsule->m_Message;
        delete capsule;
        return result;
    } else {
        if (result != NPT_ERROR_LIST_EMPTY) {
            NPT_Debug("NPT_SimpleMessageQueue::PumpMessage - exit (%d)\n", 
                      result);
        }
        return result;
    }
}
