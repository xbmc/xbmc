/*****************************************************************
|
|   Platinum - Test Light Device
|
|   (c) 2004 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltUPnP.h"
#include "PltLightSample.h"

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int /* argc */, char** /* argv */)
{
    PLT_UPnP upnp;

    PLT_DeviceHostReference device(new PLT_LightSampleDevice("Platinum Light Bulb"));
    upnp.AddDevice(device);

    upnp.Start();

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;
    }

    upnp.Stop();

    return 0;
}
