/*****************************************************************
|
|   Platinum - Test UPnP A/V MediaRenderer
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltUPnP.h"
#include "PltMediaRenderer.h"

#include <stdlib.h>

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
struct Options {
    const char* friendly_name;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(char** args)
{
    fprintf(stderr, "usage: %s [-f <friendly_name>]\n", args[0]);
    fprintf(stderr, "-f : optional upnp server friendly name\n");
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
    char**      tmp = args+1;

    /* default values */
    Options.friendly_name = NULL;

    while ((arg = *tmp++)) {
        if (!strcmp(arg, "-f")) {
            Options.friendly_name = *tmp++;
        } else {
            fprintf(stderr, "ERROR: too many arguments\n");
            PrintUsageAndExit(args);
        }
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int /* argc */, char** argv)
{   
    PLT_UPnP upnp;

    /* parse command line */
    ParseCommandLine(argv);

    PLT_DeviceHostReference device(
        new PLT_MediaRenderer(
            NULL,
            Options.friendly_name?Options.friendly_name:"Platinum Media Renderer"));
    upnp.AddDevice(device);
    bool added = true;

    upnp.Start();

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;

        if (*buf == 's') {
            if (added) {
                upnp.RemoveDevice(device);
            } else {
                upnp.AddDevice(device);
            }
            added = !added;
        }
    }

    upnp.Stop();
    return 0;
}
