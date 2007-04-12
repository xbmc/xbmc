/*****************************************************************
|
|      Neptune - Selectable Message Queue
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <unistd.h>

#include "NptSelectableMessageQueue.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|    NPT_SelectableMessageQueue::NPT_SelectableMessageQueue
+---------------------------------------------------------------------*/
NPT_SelectableMessageQueue::NPT_SelectableMessageQueue()
{
    m_Pipe[0] = -1;
    m_Pipe[1] = -1;
    pipe(m_Pipe);
}
 
/*----------------------------------------------------------------------
|    NPT_SelectableMessageQueue::~NPT_SelectableMessageQueue
+---------------------------------------------------------------------*/
NPT_SelectableMessageQueue::~NPT_SelectableMessageQueue()
{
    close(m_Pipe[0]);
    close(m_Pipe[1]);
}
 
/*----------------------------------------------------------------------
|    NPT_SelectableMessageQueue::QueueMessage
+---------------------------------------------------------------------*/
NPT_Result
NPT_SelectableMessageQueue::QueueMessage(NPT_Message*        message,
                                         NPT_MessageHandler* handler)
{
    // first, queue the message
    NPT_Result result = NPT_SimpleMessageQueue::QueueMessage(message, handler);

    // then write a byte on the pipe to signal that there is a message
    write(m_Pipe[1], "\0", 1);

    return result;
}

/*----------------------------------------------------------------------
|    NPT_SelectableMessageQueue::PumpMessage
+---------------------------------------------------------------------*/
NPT_Result
NPT_SelectableMessageQueue::PumpMessage(bool blocking)
{
    NPT_Result result = NPT_SimpleMessageQueue::PumpMessage(blocking);
    if (NPT_SUCCEEDED(result)) {
        // flush the event
        FlushEvent();
    }
    return result;
}

/*----------------------------------------------------------------------
|    NPT_SelectableMessageQueue::FlushEvent
+---------------------------------------------------------------------*/
void
NPT_SelectableMessageQueue::FlushEvent()
{
    char buffer[1];
    read(m_Pipe[0], buffer, 1);
}
 

