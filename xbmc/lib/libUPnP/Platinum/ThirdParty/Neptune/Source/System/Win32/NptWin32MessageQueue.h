/*****************************************************************
|
|      Neptune - Win32 Message Queue
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <windows.h>
#include <TCHAR.h>
#include "NptStrings.h"
#include "NptMessaging.h"
#include "NptSimpleMessageQueue.h"

/*----------------------------------------------------------------------
|       NPT_Win32MessageQueue
+---------------------------------------------------------------------*/
class NPT_Win32WindowMessageQueue : public NPT_SimpleMessageQueue
{
public:
    NPT_Win32WindowMessageQueue();
    ~NPT_Win32WindowMessageQueue();

    // NPT_SimpleMessageQueue methods
    virtual NPT_Result QueueMessage(NPT_Message*        message,
                                    NPT_MessageHandler* handler);
    virtual NPT_Result PumpMessage(bool blocking = true);

    NPT_Result HandleMessage(NPT_Message* message, NPT_MessageHandler* handler);

private:
    static LRESULT CALLBACK WindowProcedure(HWND   window, 
                                            UINT   message,
                                            WPARAM wparam, 
                                            LPARAM lparam);
    HWND        m_WindowHandle;
    TCHAR       m_ClassName[16];
    HINSTANCE   m_hInstance;
};

