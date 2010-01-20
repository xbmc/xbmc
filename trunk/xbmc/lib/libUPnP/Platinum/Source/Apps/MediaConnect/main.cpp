/*****************************************************************
|
|      Platinum - Test UPnP A/V Media Connect
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "PltUPnP.h"
#include "PltMediaConnect.h"
#include "PltVersion.h"

#include <stdlib.h>

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
struct Options {
    const char* path;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(char** args)
{
    fprintf(stderr, "usage: %s <path>\n", args[0]);
    fprintf(stderr, "<path> : local path to serve\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(char** args)
{
    char** _args = args++;
    const char* arg;

    /* default values */
    Options.path = NULL;
    
    while ((arg = *args++)) {
        if (Options.path == NULL) {
            Options.path = arg;
        } else {
            fprintf(stderr, "ERROR: too many arguments\n");
            PrintUsageAndExit(_args);
        }
    }

    /* check args */
    if (Options.path == NULL) {
        fprintf(stderr, "ERROR: path missing\n");
        PrintUsageAndExit(_args);
    }
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    // setup Neptune logging
    NPT_LogManager::GetDefault().Configure("plist:.level=FINER;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=26");
    
    NPT_COMPILER_UNUSED(argc);
    
    /* parse command line */
    ParseCommandLine(argv);
    
    PLT_DeviceHostReference device(
        new PLT_MediaConnect(Options.path, "Platinum"));
    PLT_UPnP upnp;
    upnp.AddDevice(device);

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
    return 0;
}
