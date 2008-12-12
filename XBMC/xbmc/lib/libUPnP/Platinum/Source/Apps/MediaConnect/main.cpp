/*****************************************************************
|
|      Platinum - Test UPnP A/V Media Connect
|
|      Copyright (c) 2004-2008, Plutinosoft, LLC.
|      Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "PltUPnP.h"
#include "PltMediaConnect.h"

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    NPT_COMPILER_UNUSED(argc);
    NPT_COMPILER_UNUSED(argv);

    PLT_UPnP upnp;
 
    PLT_DeviceHostReference device(new PLT_MediaConnect("C:\\", "Platinum: Sylvain: "));
    upnp.AddDevice(device);
//    NPT_String uuid = device->GetUUID();
//
//    PLT_CtrlPoint* ctrlPoint = new PLT_CtrlPoint(uuid);
//    PLT_MediaBrowser* browser = new PLT_MediaBrowser(ctrlPoint, NULL);
//    upnp.AddCtrlPoint(ctrlPoint);
//    ctrlPoint->Release();

    if (NPT_FAILED(upnp.Start()))
        return 1;

    char buf[256];
    while (gets(buf))
    {
        if (*buf == 'q')
        {
            break;
        }
    }

    upnp.Stop();
//    delete browser;
    return 0;
}
