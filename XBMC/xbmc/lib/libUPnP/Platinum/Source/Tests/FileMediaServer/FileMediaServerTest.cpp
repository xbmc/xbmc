/*****************************************************************
|
|   Platinum - Test UPnP A/V MediaServer
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltUPnP.h"
#include "PltFileMediaServer.h"

#include <stdlib.h>

NPT_SET_LOCAL_LOGGER("platinum.media.server.file.test")

//#define BROADCAST_TEST 1

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
struct Options {
    const char* path;
    const char* friendly_name;
    bool        broadcast;
    long        port;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, "usage: FileMediaServerTest [-f <friendly_name>] [-p <port>] [-b] <path>\n");
    fprintf(stderr, "-f : optional upnp device friendly name\n");
    fprintf(stderr, "-b : optional upnp device broadcast mode\n");
    fprintf(stderr, "-p : optional http port\n");
    fprintf(stderr, "<path> : local path to serve\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(char** args)
{
    const char* arg;

    /* default values */
    Options.path     = NULL;
    Options.friendly_name = NULL;
    Options.broadcast = false;
    Options.port = 0;

    while ((arg = *args++)) {
        if (!strcmp(arg, "-f")) {
            Options.friendly_name = *args++;
        } else if (!strcmp(arg, "-p")) {
            if (NPT_FAILED(NPT_ParseInteger(*args++, Options.port))) {
                fprintf(stderr, "ERROR: invalid argument\n");
                PrintUsageAndExit();
            }
        } else if (!strcmp(arg, "-b")) {
            Options.broadcast = true;
        } else if (Options.path == NULL) {
            Options.path = arg;
        } else {
            fprintf(stderr, "ERROR: too many arguments\n");
            PrintUsageAndExit();
        }
    }

    /* check args */
    if (Options.path == NULL) {
        fprintf(stderr, "ERROR: path missing\n");
        PrintUsageAndExit();
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int /* argc */, char** argv)
{
    /* parse command line */
    ParseCommandLine(argv+1);

    PLT_UPnP upnp(1900, !Options.broadcast);

    PLT_DeviceHostReference device(
        new PLT_FileMediaServer(
            Options.path, 
            Options.friendly_name?Options.friendly_name:"Platinum UPnP Media Server",
            false,
            NULL,
            0,
            (NPT_UInt16)Options.port)
            );

    NPT_List<NPT_String> list;
    NPT_CHECK_SEVERE(PLT_UPnPMessageHelper::GetIPAddresses(list));
    NPT_String ip = *(list.GetFirstItem());

    //device->m_PresentationURL = NPT_HttpUrl(ip, 80, "/").ToString();
    device->m_ModelDescription = "Platinum File Media Server";
    device->m_ModelURL = "http://www.plutinosoft.com/";
    device->m_ModelNumber = "1.0";
    device->m_ModelName = "Platinum File Media Server";
    device->m_Manufacturer = "Plutinosoft";
    device->m_ManufacturerURL = "http://www.plutinosoft.com/";

    if (Options.broadcast) device->SetBroadcast(true);

    upnp.AddDevice(device);
    NPT_String uuid = device->GetUUID();

    NPT_CHECK_SEVERE(upnp.Start());
    NPT_LOG_INFO("Press 'q' to quit.");

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;
    }

    upnp.Stop();

    return 0;
}
