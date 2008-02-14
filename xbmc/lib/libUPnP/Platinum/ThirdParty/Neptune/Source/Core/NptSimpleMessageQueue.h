/*****************************************************************
|
|   Neptune - Simple Message Queue
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_SIMPLE_MESSAGE_QUEUE_H_
#define _NPT_SIMPLE_MESSAGE_QUEUE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptThreads.h"
#include "NptMessaging.h"
#include "NptQueue.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
struct NPT_SimpleMessageCapsule;

/*----------------------------------------------------------------------
|   NPT_SimpleMessageQueue
+---------------------------------------------------------------------*/
class NPT_SimpleMessageQueue : public NPT_MessageQueue
{
 public:
    // members
    NPT_SimpleMessageQueue();
    virtual ~NPT_SimpleMessageQueue();

    // NPT_MessageQueue methods
    virtual NPT_Result QueueMessage(NPT_Message*        message, 
                                    NPT_MessageHandler* handler);
    virtual NPT_Result PumpMessage(NPT_Timeout timeout = NPT_TIMEOUT_INFINITE);

 private:
    // members
    NPT_Queue<NPT_SimpleMessageCapsule> m_Queue;
};

#endif // _NPT_SIMPLE_MESSAGE_QUEUE_H_
