/*****************************************************************
|
|   Platinum - Test HTTP
|
|   (c) 2004 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptUtils.h"
#include "Neptune.h"
#include "NptDirectory.h"
#include "NptLogging.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltDownloader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

NPT_SET_LOCAL_LOGGER("platinum.core.http.test")

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
struct Options {
    NPT_UInt32  port;
    NPT_String  path;
} Options;

// NPT_Result
// GetBodyStream(NPT_HttpMessage* message, PLT_RingBufferStream*& stream)
// {
//     stream = NULL;
// 
//     if (!message) return NPT_FAILURE;
// 
//     NPT_HttpEntity* entity = message->GetEntity();
//     if (!entity) return NPT_FAILURE;
// 
//     NPT_InputStreamReference input;
//     if (NPT_FAILED(entity->GetInputStream(input))) return NPT_FAILURE;
// 
//     // read body length
//     NPT_Size len = -1;
//     PLT_HttpHelper::GetContentLength(message, len);
// 
//     stream = new PLT_RingBufferStream(input, 4096, len);
//     return NPT_SUCCESS;
//}

/*----------------------------------------------------------------------
|   DumpBody
+---------------------------------------------------------------------*/
// static NPT_Result 
// DumpBody(PLT_Downloader& downloader, NPT_InputStreamReference& stream, NPT_Size& size)
// {
//     char buffer[2048];
//     NPT_Result ret = NPT_ERROR_WOULD_BLOCK;
// 
//     size = 0;
// 
//     do {
//         Plt_DowloaderState state = downloader.GetState();
// 
//         NPT_Size bytes_read = 0;
//         ret = stream->Read(buffer, 2048, &bytes_read);
// 
//         if (NPT_SUCCEEDED(ret)) {
//             PLT_Log(PLT_LOG_LEVEL_1, "Read %d bytes\n", bytes_read);
//             size += bytes_read;
//         } else if (ret == NPT_ERROR_WOULD_BLOCK) {
//             switch (state) {
//                 case PLT_DOWNLOADER_SUCCESS:
//                     return NPT_SUCCESS;
// 
//                 case PLT_DOWNLOADER_ERROR:
//                     return NPT_FAILURE;
// 
//                 default:
//                     NPT_System::Sleep(NPT_TimeInterval(0, 10000));
//                     ret = NPT_SUCCESS;
//                     break;
//             }
//         }
//     } while (NPT_SUCCEEDED(ret));
// 
//     return ret;
//}

/*----------------------------------------------------------------------
|   Test1
+---------------------------------------------------------------------*/
static bool
Test1(PLT_TaskManager* task_manager, const char* url, NPT_Size& size)
{
    NPT_LOG_INFO("########### TEST 1 ######################");

    NPT_MemoryStreamReference stream(new NPT_MemoryStream());
    PLT_Downloader downloader(task_manager, url, (NPT_OutputStreamReference&)stream);
    downloader.Start();

    while (1) {
        switch(downloader.GetState()) {
            case PLT_DOWNLOADER_SUCCESS: {
                size = stream->GetDataSize();
                return true;
            }

            case PLT_DOWNLOADER_ERROR:
                return false;

            default:
                NPT_System::Sleep(NPT_TimeInterval(0, 10000));
                // watchdog?
                break;
        }
    };

    return false;
}

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(char** args)
{
    fprintf(stderr, "usage: %s [-p <port>] <path>\n", args[0]);
    fprintf(stderr, "-p : optional server port\n");
    fprintf(stderr, "<path> : local filepath to serve\n");
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
    Options.port = 0;
    Options.path = "";

    while ((arg = *tmp++)) {
        if (!strcmp(arg, "-p")) {
            long port;
            if (NPT_FAILED(NPT_ParseInteger(*tmp++, port, false))) {
                fprintf(stderr, "ERROR: invalid port\n");
                exit(1);
            }
            Options.port = port;
        } else if (Options.path.IsEmpty()) {
            Options.path = arg;
        } else {
            fprintf(stderr, "ERROR: too many arguments\n");
            PrintUsageAndExit(args);
        }
    }

    /* check args */
    if (Options.path.IsEmpty()) {
        fprintf(stderr, "ERROR: path missing\n");
        PrintUsageAndExit(args);
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    NPT_COMPILER_UNUSED(argc);

    /* parse command line */
    ParseCommandLine(argv);

    /* extract folder path */
    int index1 = Options.path.ReverseFind('\\');
    int index2 = Options.path.ReverseFind('/');
    if (index1 <= 0 && index2 <=0) {
        fprintf(stderr, "ERROR: invalid path\n");
        exit(1);
    }

    NPT_DirectoryEntryInfo info;
    NPT_CHECK_SEVERE(NPT_DirectoryEntry::GetInfo(Options.path, info));

    /* and the http server */
    PLT_HttpServer http_server(Options.port?Options.port:80);
    NPT_HttpFileRequestHandler* handler = new NPT_HttpFileRequestHandler(
        Options.path.Left(index1>index2?index1:index2), 
        "/");
    http_server.AddRequestHandler(handler, "/", true);

    /* start it */
    NPT_CHECK_SEVERE(http_server.Start());

    NPT_String url = "http://127.0.0.1:" + NPT_String::FromInteger(http_server.GetPort());
    url += "/" + Options.path.SubString((index1>index2?index1:index2)+1);

    /* a task manager for the tests downloader */
    PLT_TaskManager task_manager;

    /* small delay to let the server start */
    NPT_System::Sleep(NPT_TimeInterval(1, 0));
    
    NPT_Size size;
    bool result = Test1(&task_manager, url.GetChars(), size);
    if (!result) return -1;

    NPT_System::Sleep(NPT_TimeInterval(1, 0));

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;
    }

    http_server.Stop();
    delete handler;
    return 0;
}
