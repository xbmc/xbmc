/*****************************************************************
|
|   Platinum - SSDP Test Program 1
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
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
main(int argc, char** argv)
{
    PLT_SetLogLevel(PLT_LOG_LEVEL_4);

    PLT_UPnP upnp;
    upnp.AddCtrlPoint(PLT_CtrlPointReference(new PLT_CtrlPoint()));

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
