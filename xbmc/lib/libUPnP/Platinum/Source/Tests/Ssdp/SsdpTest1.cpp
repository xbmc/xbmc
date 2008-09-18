/*****************************************************************
|
|   Platinum - SSDP Test Program 1
|
|   Copyright (c) 2004-2008 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptUtils.h"
#include "Neptune.h"
#include "PltLog.h"
#include "Platinum.h"

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int, char**)
{
    PLT_SetLogLevel(PLT_LOG_LEVEL_4);

    PLT_UPnP upnp;
    PLT_CtrlPointReference ctrl_point(new PLT_CtrlPoint());
    upnp.AddCtrlPoint(ctrl_point);

    if (NPT_FAILED(upnp.Start()))
        return 1;

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;
    }

    upnp.Stop();

    return 0;
}
