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

//#define TEST_EMBEDDED_DEVICE 1

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int /* argc */, char** /* argv */)
{
    PLT_UPnP upnp;

    PLT_DeviceHostReference device(new PLT_LightSampleDevice("Platinum Light Bulb"));

#ifdef TEST_EMBEDDED_DEVICE
    PLT_DeviceHostReference device2(new PLT_LightSampleDevice("Platinum Light Bulb embed"));
    device->AddDevice((PLT_DeviceDataReference&)device2);
    
    PLT_DeviceHostReference device3(new PLT_LightSampleDevice("Platinum Light Bulb embed 2"));
    device->AddDevice(device3);
#endif

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
