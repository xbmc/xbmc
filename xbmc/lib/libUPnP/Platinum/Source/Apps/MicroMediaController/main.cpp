/*****************************************************************
|
|   Platinum - main
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#include "PltMicroMediaController.h"
#include "PltFileMediaServer.h"
#include "PltMediaRenderer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int main(void)
{
    //PLT_SetLogLevel(3);

    // Create upnp engine
    PLT_UPnP upnp(1900, true);

    // Create control point
    PLT_CtrlPointReference ctrlPoint(new PLT_CtrlPoint());

    // Create controller
    PLT_MicroMediaController controller(ctrlPoint);

#ifdef HAS_SERVER
    // create device
    PLT_DeviceHostReference server(
        new PLT_FileMediaServer(
        "C:\\Music", 
        "Platinum UPnP Media Server"));

    NPT_String ip;
    NPT_List<NPT_String> list;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list))) {
        ip = *(list.GetFirstItem());
    }
    server->m_PresentationURL = NPT_HttpUrl(ip, 80, "/").ToString();
    server->m_ModelDescription = "Platinum File Media Server";
    server->m_ModelURL = "http://www.plutinosoft.com/";
    server->m_ModelNumber = "1.0";
    server->m_ModelName = "Platinum File Media Server";
    server->m_Manufacturer = "Plutinosoft";
    server->m_ManufacturerURL = "http://www.plutinosoft.com/";

    // add device
    upnp.AddDevice(server);

    // remove device uuid from ctrlpoint
    ctrlPoint->IgnoreUUID(server->GetUUID());
#endif

#ifdef HAS_RENDERER
    // create device
    PLT_DeviceHostReference renderer(
        new PLT_MediaRenderer(NULL, "Platinum Media Renderer"));
    renderer->m_SerialNumber = "308485761705";
    renderer->m_ModelDescription = "Platinum Renderer";
    renderer->m_ModelName = "Platinum";
    renderer->m_Manufacturer = "Plutinosoft";

    // add device
    upnp.AddDevice(renderer);
    ctrlPoint->IgnoreUUID(renderer->GetUUID());
#endif

    // add control point to upnp engine and start it
    upnp.AddCtrlPoint(ctrlPoint);

    upnp.Start();

    // tell control point to perform extra broadcast discover 
    // in case our device doesn't support multicast
    //ctrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1);

    // start to process commands 
    controller.ProcessCommandLoop();

    upnp.Stop();

    return 0;
}
