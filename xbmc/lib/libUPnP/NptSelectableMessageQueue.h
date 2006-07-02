/*****************************************************************
|
|   Neptune - Selectable Message Queue
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_SELECTABLE_MESSAGE_QUEUE_H_
#define _NPT_SELECTABLE_MESSAGE_QUEUE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptMessaging.h"
#include "NptSimpleMessageQueue.h"

/*----------------------------------------------------------------------
|   NPT_SelectableMessageQueue
+---------------------------------------------------------------------*/
class NPT_SelectableMessageQueue : public NPT_SimpleMessageQueue
{
public:
    // methods
             NPT_SelectableMessageQueue();
    virtual ~NPT_SelectableMessageQueue();
    virtual NPT_Result QueueMessage(NPT_Message*        message,
                                    NPT_MessageHandler* handler);
    virtual NPT_Result PumpMessage(bool blocking = true);
    int  GetEventFd() { return m_Pipe[0]; }
 
private:
    // methods
    void FlushEvent();

    // members
    int m_Pipe[2];
};


#endif /* _NPT_SELECTABLE_MESSAGE_QUEUE_H_ */
