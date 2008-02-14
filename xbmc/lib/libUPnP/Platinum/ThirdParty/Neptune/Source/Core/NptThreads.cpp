/*****************************************************************
|
|   Neptune - Threads
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptThreads.h"

/*----------------------------------------------------------------------
|   NPT_ThreadCallbackSlot::NPT_ThreadCallbackSlot
+---------------------------------------------------------------------*/
NPT_ThreadCallbackSlot::NPT_ThreadCallbackSlot() :
    m_CallbackArgs(NULL),
    m_Shutdown(false),
    m_NotificationHelper(NULL)
{
}

/*----------------------------------------------------------------------
|   NPT_ThreadCallbackSlot::Shutdown
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ThreadCallbackSlot::Shutdown()
{
    // protect against concurrent access
    //FIXME: This will not work if another Thread has called ReceiveCallback with a timeout
    NPT_AutoLock lock(m_ReadLock);

    // signal we are shut down
    m_Shutdown = true;

    // clear up any pending callbacks
    m_Pending.SetValue(0);
    m_Ack.SetValue(1);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ThreadCallbackSlot::SetNotificationHelper
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ThreadCallbackSlot::SetNotificationHelper(NotificationHelper* helper)
{
    m_NotificationHelper = helper;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ThreadCallbackSlot::ReceiveCallback
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ThreadCallbackSlot::ReceiveCallback(NPT_ThreadCallbackReceiver& receiver,
                                        NPT_Timeout                 timeout)
{
    // protect against concurrent access
    //NPT_Debug("NPT_ThreadCallbackSlot::ReceiveCallback - read locking, timeout=%d\n", timeout);
    NPT_AutoLock lock(m_ReadLock);

    if (timeout) {
        // wait until there is a pending callback
        //NPT_Debug("NPT_ThreadCallbackSlot::ReceiveCallback - waiting...\n");
        NPT_Result result = m_Pending.WaitUntilEquals(1, timeout);
        if (NPT_FAILED(result)) return result; // don't log here because the result
                                               // could be NPT_ERROR_TIMEOUT which
                                               // is an expected normal case.
        //NPT_Debug("NPT_ThreadCallbackSlot::ReceiveCallback - got it\n");
    } else {
        // see if something is pending
        int pending = 0;
        NPT_CHECK(m_Pending.GetValue(pending));
        if (pending == 0) {
            //NPT_Debug("NPT_ThreadCallbackSlot::ReceiveCallback - nothing pending\n");
            return NPT_ERROR_CALLBACK_NOTHING_PENDING;
        }
    }

    // check if we have been shutdown
    if (m_Shutdown) return NPT_ERROR_CALLBACK_HANDLER_SHUTDOWN;

    // process the callback
    //NPT_Debug("NPT_ThreadCallbackSlot::ReceiveCallback - calling back\n");
    receiver.OnCallback(const_cast<void*>(m_CallbackArgs));

    // signal that we've processed the callback
    m_Pending.SetValue(0);
    m_Ack.SetValue(1);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ThreadCallbackSlot::SendCallback
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ThreadCallbackSlot::SendCallback(void* args)
{
    // protect against concurrent access
    //NPT_Debug("NPT_ThreadCallbackSlot::SendCallback - write locking\n");
    NPT_AutoLock lock(m_WriteLock);

    // there should be nothing pending
#if defined(NPT_DEBUG)
    int pending = 0;
    NPT_CHECK(m_Pending.GetValue(pending));
    NPT_ASSERT(pending == 0);
#endif

    // check if we have been shutdown
    if (m_Shutdown) return NPT_ERROR_CALLBACK_HANDLER_SHUTDOWN;

    // put the callback args
    m_CallbackArgs = args;
    //NPT_Debug("NPT_ThreadCallbackSlot::SendCallback - signalling\n");
    m_Pending.SetValue(1);
    
    // call the helper before we wait
    if (m_NotificationHelper) {
        m_NotificationHelper->Notify();
    }

    // wait until the callback has been process, or we've been shutdown
    //NPT_Debug("NPT_ThreadCallbackSlot::SendCallback - waiting...\n");
    m_Ack.WaitUntilEquals(1);
    //NPT_Debug("NPT_ThreadCallbackSlot::SendCallback - got it\n");

    // done
    m_Ack.SetValue(0);
    m_CallbackArgs = NULL;

    return m_Shutdown?NPT_ERROR_CALLBACK_HANDLER_SHUTDOWN:NPT_SUCCESS;
}
