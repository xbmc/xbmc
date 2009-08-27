/*****************************************************************
|
|   Platinum - HTTP tests
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
|   includes
+---------------------------------------------------------------------*/
#include "NptUtils.h"
#include "Neptune.h"
#include "NptLogging.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltDownloader.h"
#include "PltRingBufferStream.h"

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

/*----------------------------------------------------------------------
|   PLT_HttpCustomRequestHandler
+---------------------------------------------------------------------*/
class PLT_HttpCustomRequestHandler : public NPT_HttpRequestHandler
{
public:
    // constructors
    PLT_HttpCustomRequestHandler(NPT_InputStreamReference& body, 
                                 const char*               mime_type) :
        m_Body(body),
        m_MimeType(mime_type) {}

    // NPT_HttpRequetsHandler methods
    virtual NPT_Result SetupResponse(NPT_HttpRequest&              request, 
                                     const NPT_HttpRequestContext& context,
                                     NPT_HttpResponse&             response) {
        NPT_COMPILER_UNUSED(request);
        NPT_COMPILER_UNUSED(context);

        NPT_HttpEntity* entity = response.GetEntity();
        if (entity == NULL) return NPT_ERROR_INVALID_STATE;

        entity->SetContentType(m_MimeType);
        entity->SetInputStream(m_Body);

        return NPT_SUCCESS;
    }

private:
    NPT_InputStreamReference m_Body;
    NPT_String               m_MimeType;
};

/*----------------------------------------------------------------------
|   Test1
+---------------------------------------------------------------------*/
static bool
Test1(PLT_TaskManager* task_manager, const char* url, NPT_Size& size)
{
    NPT_LOG_INFO("########### TEST 1 ######################");

    NPT_MemoryStreamReference memory_stream(new NPT_MemoryStream());
    NPT_OutputStreamReference output_stream(memory_stream);
    PLT_Downloader downloader(task_manager, url, output_stream);
    downloader.Start();

    while (1) {
        switch(downloader.GetState()) {
            case PLT_DOWNLOADER_SUCCESS: {
                size = memory_stream->GetDataSize();
                return true;
            }

            case PLT_DOWNLOADER_ERROR:
                return false;

            default:
                NPT_System::Sleep(NPT_TimeInterval(0, 10000));
                break;
        }
    };

    return false;
}

/*----------------------------------------------------------------------
|   DumpBody
+---------------------------------------------------------------------*/
 static NPT_Result 
ReadBody(PLT_Downloader& downloader, NPT_InputStreamReference& stream, NPT_Size& size)
{
    NPT_LargeSize avail;
    char buffer[2048];
    NPT_Result ret = NPT_ERROR_WOULD_BLOCK;

    /* reset output param first */
    size = 0;

    /*
       we test for availability first to avoid
       getting stuck in Read forever in case blocking is true
       and the download is done writing to the stream
    */
    NPT_CHECK(stream->GetAvailable(avail));

    if (avail) {
         ret = stream->Read(buffer, 2048, &size);
         NPT_LOG_FINER_2("Read %d bytes (result = %d)\n", size, ret);
         return ret;
     } else {
         Plt_DowloaderState state = downloader.GetState();
         switch (state) {
             case PLT_DOWNLOADER_ERROR:
                 return NPT_FAILURE;

             case PLT_DOWNLOADER_SUCCESS:
                 /* no more data expected */
                 return NPT_ERROR_EOS;

             default:
                 NPT_System::Sleep(NPT_TimeInterval(0, 10000));
                 break;
         }
     }
 
     return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Test2
+---------------------------------------------------------------------*/
static bool
Test2(PLT_TaskManager* task_manager, const char* url, NPT_Size& size)
{
    NPT_LOG_INFO("########### TEST 2 ######################");

    /* reset output param first */
    size = 0;

    PLT_RingBufferStreamReference ringbuffer_stream(new PLT_RingBufferStream());
    NPT_OutputStreamReference output_stream(ringbuffer_stream);
    NPT_InputStreamReference  input_stream(ringbuffer_stream);
    PLT_Downloader downloader(task_manager, url, output_stream);
    downloader.Start();

    while (1) {
        switch(downloader.GetState()) {
            case PLT_DOWNLOADER_SUCCESS:
                ringbuffer_stream->SetEos();
                /* fallthrough */

            case PLT_DOWNLOADER_DOWNLOADING: {
                    NPT_Size bytes_read;
                    NPT_Result res = ReadBody(downloader, input_stream, bytes_read);
                    if (NPT_FAILED(res)) {
                        return (res==NPT_ERROR_EOS)?true:false;
                    }
                    size += bytes_read;
                }
                break;

            case PLT_DOWNLOADER_ERROR:
                return false;

            default:
                NPT_System::Sleep(NPT_TimeInterval(0, 10000));
                break;
        }
    };

    return false;
}

/*----------------------------------------------------------------------
|   Test3
+---------------------------------------------------------------------*/
static bool
Test3(PLT_TaskManager* task_manager, const char* url, PLT_RingBufferStreamReference& ringbuffer_stream, NPT_Size& size)
{
    NPT_LOG_INFO("########### TEST 3 ######################");

    /* reset output param first */
    size = 0;

    NPT_MemoryStreamReference memory_stream(new NPT_MemoryStream());
    NPT_OutputStreamReference output_stream(memory_stream);
    PLT_Downloader downloader(task_manager, url, output_stream);
    downloader.Start();

    /* asynchronously write onto ring buffer stream */
    char buffer[32768];
    ringbuffer_stream->WriteFully(buffer, 32768);

    /* mark as done */
    ringbuffer_stream->SetEos();

    while (1) {
        switch(downloader.GetState()) {
            case PLT_DOWNLOADER_SUCCESS:
                size = memory_stream->GetDataSize();
                return true;

            case PLT_DOWNLOADER_ERROR:
                return false;

            default:
                NPT_System::Sleep(NPT_TimeInterval(0, 10000));
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
    fprintf(stderr, "usage: %s [-p <port>] [-f <filepath>]\n", args[0]);
    fprintf(stderr, "-p : optional server port\n");
    fprintf(stderr, "-f : optional local filepath to serve\n");
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
        if (Options.port == 0 && !strcmp(arg, "-p")) {
            NPT_UInt32 port;
            if (NPT_FAILED(NPT_ParseInteger32(*tmp++, port, false))) {
                fprintf(stderr, "ERROR: invalid port\n");
                exit(1);
            }
            Options.port = port;
        } else if (Options.path.IsEmpty() && !strcmp(arg, "-f")) {
            Options.path = *tmp++;
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
main(int argc, char** argv)
{
    NPT_COMPILER_UNUSED(argc);

    NPT_HttpRequestHandler *handler, *custom_handler;
    NPT_Reference<NPT_DataBuffer> buffer;
    NPT_Size size;
    bool result;
    PLT_RingBufferStreamReference ringbuffer_stream(new PLT_RingBufferStream());

    /* parse command line */
    ParseCommandLine(argv);

    /* create http server */
    PLT_HttpServer http_server(Options.port?Options.port:80);
    NPT_String url = "http://127.0.0.1:" + NPT_String::FromInteger(http_server.GetPort());
    NPT_String custom_url = url;

    if (!Options.path.IsEmpty()) {
        /* extract folder path */
        int index1 = Options.path.ReverseFind('\\');
        int index2 = Options.path.ReverseFind('/');
        if (index1 <= 0 && index2 <=0) {
            fprintf(stderr, "ERROR: invalid path\n");
            exit(1);
        }

        NPT_FileInfo info;
        NPT_CHECK_SEVERE(NPT_File::GetInfo(Options.path, &info));

        /* add file request handler */
        handler = new NPT_HttpFileRequestHandler(
            Options.path.Left(index1>index2?index1:index2), 
            "/");
        http_server.AddRequestHandler(handler, "/", true);

        /* build url*/
        url += "/" + Options.path.SubString((index1>index2?index1:index2)+1);
    } else {
        /* create random data */
        buffer = new NPT_DataBuffer(32768);
        buffer->SetDataSize(32768);

        /* add static handler */
        handler = new NPT_HttpStaticRequestHandler(buffer->GetData(),
            buffer->GetDataSize(),
            "text/xml");
        http_server.AddRequestHandler(handler, "/test");

        /* build url*/
        url += "/test";
    }

    /* add custom handler */
    NPT_InputStreamReference stream(ringbuffer_stream);
    custom_handler = new PLT_HttpCustomRequestHandler(stream,
        "text/xml");
    http_server.AddRequestHandler(custom_handler, "/custom");
    custom_url += "/custom";

    /* start server */
    NPT_CHECK_SEVERE(http_server.Start());

    /* a task manager for the tests downloader */
    PLT_TaskManager task_manager;

    /* small delay to let the server start */
    NPT_System::Sleep(NPT_TimeInterval(1, 0));
    
    /* execute tests */
    result = Test1(&task_manager, url.GetChars(), size);
    if (!result) return -1;

    result = Test2(&task_manager, url.GetChars(), size);
    if (!result) return -1;

    result = Test3(&task_manager, custom_url.GetChars(), ringbuffer_stream, size);
    if (!result) return -1;

    NPT_System::Sleep(NPT_TimeInterval(1, 0));

    http_server.Stop();
    delete handler;
    delete custom_handler;
    return 0;
}
