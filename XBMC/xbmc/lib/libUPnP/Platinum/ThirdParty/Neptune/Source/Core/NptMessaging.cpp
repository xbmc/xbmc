/*****************************************************************
|
|   Neptune - Messaging System
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptMessaging.h"

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
NPT_Message::Type NPT_Message::MessageType = "Generic Message";

/*----------------------------------------------------------------------
|   NPT_MessageHandler::HandleMessage
+---------------------------------------------------------------------*/
NPT_Result 
NPT_MessageHandler::HandleMessage(NPT_Message* message)
{
    return message->Dispatch(this);
}
