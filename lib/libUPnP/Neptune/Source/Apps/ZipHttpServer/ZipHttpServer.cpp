/*****************************************************************
|
|      Virtual ZIP file HTTP Server
|
|      (c) 2001-2014 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.ziphttpserver")

/*----------------------------------------------------------------------
|   GetContentType
+---------------------------------------------------------------------*/
struct FileTypeMapEntry {
    const char* extension;
    const char* mime_type;
};
static const FileTypeMapEntry
DefaultFileTypeMap[] = {
    {"xml",  "text/xml"  },
    {"htm",  "text/html" },
    {"html", "text/html" },
    {"c",    "text/plain"},
    {"h",    "text/plain"},
    {"txt",  "text/plain"},
    {"css",  "text/css"  },
    {"gif",  "image/gif" },
    {"thm",  "image/jpeg"},
    {"png",  "image/png"},
    {"tif",  "image/tiff"},
    {"tiff", "image/tiff"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"jpe",  "image/jpeg"},
    {"jp2",  "image/jp2" },
    {"png",  "image/png" },
    {"bmp",  "image/bmp" },
    {"aif",  "audio/x-aiff"},
    {"aifc", "audio/x-aiff"},
    {"aiff", "audio/x-aiff"},
    {"mpa",  "audio/mpeg"},
    {"mp2",  "audio/mpeg"},
    {"mp3",  "audio/mpeg"},
    {"m4a",  "audio/mp4"},
    {"wma",  "audio/x-ms-wma"},
    {"wav",  "audio/x-wav"},
    {"mpeg", "video/mpeg"},
    {"mpg",  "video/mpeg"},
    {"mp4",  "video/mp4"},
    {"m4v",  "video/mp4"},
    {"m4f",  "video/mp4"},
    {"m4s",  "video/mp4"},
    {"ts",   "video/MP2T"}, // RFC 3555
    {"mov",  "video/quicktime"},
    {"wmv",  "video/x-ms-wmv"},
    {"asf",  "video/x-ms-asf"},
    {"avi",  "video/x-msvideo"},
    {"divx", "video/x-msvideo"},
    {"xvid", "video/x-msvideo"},
    {"doc",  "application/msword"},
    {"js",   "application/javascript"},
    {"m3u8", "application/x-mpegURL"},
    {"pdf",  "application/pdf"},
    {"ps",   "application/postscript"},
    {"eps",  "application/postscript"},
    {"zip",  "application/zip"},
    {"mpd",  "application/dash+xml"}
};

NPT_Map<NPT_String, NPT_String> FileTypeMap;

static const char*
GetContentType(const NPT_String& filename)
{
    int last_dot = filename.ReverseFind('.');
    if (last_dot > 0) {
        NPT_String extension = filename.GetChars()+last_dot+1;
        extension.MakeLowercase();
        
        NPT_String* mime_type;
        if (NPT_SUCCEEDED(FileTypeMap.Get(extension, mime_type))) {
            return mime_type->GetChars();
        }
    }

    return "application/octet-stream";
}

/*----------------------------------------------------------------------
|   ZipRequestHandler
+---------------------------------------------------------------------*/
class ZipRequestHandler : public NPT_HttpRequestHandler
{
public:
    // constructors
    ZipRequestHandler(const char* url_root,
                      const char* file_root) :
        m_UrlRoot(url_root),
        m_FileRoot(file_root) {}

    // NPT_HttpRequestHandler methods
    virtual NPT_Result SetupResponse(NPT_HttpRequest&              request, 
                                     const NPT_HttpRequestContext& context,
                                     NPT_HttpResponse&             response);

private:
    NPT_String m_UrlRoot;
    NPT_String m_FileRoot;
};

/*----------------------------------------------------------------------
|   ZipRequestHandler::SetupResponse
+---------------------------------------------------------------------*/
NPT_Result
ZipRequestHandler::SetupResponse(NPT_HttpRequest&              request, 
                                 const NPT_HttpRequestContext& /*context*/,
                                 NPT_HttpResponse&             response)
{
    NPT_HttpEntity* entity = response.GetEntity();
    if (entity == NULL) return NPT_ERROR_INVALID_STATE;

    // check the method
    if (request.GetMethod() != NPT_HTTP_METHOD_GET &&
        request.GetMethod() != NPT_HTTP_METHOD_HEAD) {
        response.SetStatus(405, "Method Not Allowed");
        return NPT_SUCCESS;
    }

    // set some default headers
    response.GetHeaders().SetHeader(NPT_HTTP_HEADER_ACCEPT_RANGES, "bytes");

    // declare HTTP/1.1 if the client asked for it
    if (request.GetProtocol() == NPT_HTTP_PROTOCOL_1_1) {
        response.SetProtocol(NPT_HTTP_PROTOCOL_1_1);
    }
    
    // default status
    response.SetStatus(404, "Not Found");

    // check that the request's path is an entry under the url root
    if (!request.GetUrl().GetPath().StartsWith(m_UrlRoot)) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // compute the path relative to the URL root
    NPT_String relative_path = NPT_Url::PercentDecode(request.GetUrl().GetPath().GetChars()+m_UrlRoot.GetLength());
    
    // check that there is no '..' in the path, for security reasons
    if (relative_path.Find("..") >= 0) {
        NPT_LOG_INFO(".. in path is not supported");
        return NPT_SUCCESS;
    }
    // check that the path does not end with a /
    if (relative_path.EndsWith("/")) {
        NPT_LOG_INFO("skipping paths that end in /");
        return NPT_SUCCESS;
    }
    NPT_List<NPT_String> path_parts = relative_path.Split("/");
    
    // walk down the path until we find a file
    NPT_String path = m_FileRoot;
    NPT_String subpath;
    NPT_List<NPT_String>::Iterator fragment = path_parts.GetFirstItem();
    bool anchor_found = false;
    bool is_zip = false;
    for (; fragment; ++fragment) {
        if (!anchor_found) {
            path += '/';
            path += *fragment;

            // get info about the file
            NPT_FileInfo info;
            NPT_File::GetInfo(path, &info);
            if (info.m_Type == NPT_FileInfo::FILE_TYPE_DIRECTORY) {
                continue;
            } else if (info.m_Type == NPT_FileInfo::FILE_TYPE_REGULAR) {
                anchor_found = true;
                if (path.EndsWith(".zip", true)) {
                    // this is a zip file
                    is_zip = true;
                }
            } else {
                return NPT_SUCCESS;
            }
        } else {
            if (!subpath.IsEmpty()) {
                subpath += '/';
            }
            subpath += *fragment;
        }
    }
    NPT_LOG_FINE_3("is_zip=%d, path=%s, subpath=%s", (int)is_zip, path.GetChars(), subpath.GetChars());
    
    // return now if no anchor was found
    if (!anchor_found) {
        return NPT_SUCCESS;
    }
    
    // deal with regular files
    if (!is_zip) {
        if (subpath.IsEmpty()) {
            // open the file
            NPT_File file(path);
            NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
            if (NPT_FAILED(result)) {
                NPT_LOG_FINE("file not found");
                return NPT_SUCCESS;
            }
            NPT_InputStreamReference file_stream;
            file.GetInputStream(file_stream);
            entity->SetInputStream(file_stream, true);
            entity->SetContentType(GetContentType(path));
            response.SetStatus(200, "OK");
        }
        return NPT_SUCCESS;
    }
    
    // load the zip file
    NPT_File file(path);
    NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("failed to open file (%d)", result);
        return result;
    }
    NPT_InputStreamReference zip_stream;
    file.GetInputStream(zip_stream);
    NPT_ZipFile* zip_file = NULL;
    result = NPT_ZipFile::Parse(*zip_stream, zip_file);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("failed to parse zip file (%d)", result);
        return result;
    }
    
    // look for the entry in the zip file
    for (unsigned int i=0; i<zip_file->GetEntries().GetItemCount(); i++) {
        NPT_ZipFile::Entry& entry = zip_file->GetEntries()[i];
        if (subpath == entry.m_Name) {
            // send the file
            NPT_InputStream* file_stream = NULL;
            result = NPT_ZipFile::GetInputStream(entry, zip_stream, file_stream);
            if (NPT_FAILED(result)) {
                NPT_LOG_WARNING_1("failed to get the file stream (%d)", result);
                delete zip_file;
                return result;
            }
            NPT_InputStreamReference file_stream_ref(file_stream);
            entity->SetInputStream(file_stream_ref, true);
            entity->SetContentType(GetContentType(subpath));
            response.SetStatus(200, "OK");
            break;
        }
    }
    
    delete zip_file;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   ZipHttpWorker
+---------------------------------------------------------------------*/
class ZipHttpServer;
class ZipHttpWorker : public NPT_Thread {
public:
    // types
    enum {
        IDLE,
        RUNNING,
        DEAD
    } State;
    
    // constructor
    ZipHttpWorker(unsigned int id, ZipHttpServer* server) :
        m_Id(id),
        m_Server(server),
        m_State(IDLE) {}
    
    // NPT_Runnable methods
    virtual void Run();
    NPT_Result Respond();
    
    // members
    unsigned int              m_Id;
    ZipHttpServer*            m_Server;
    NPT_SharedVariable        m_State;
    NPT_InputStreamReference  m_InputStream;
    NPT_OutputStreamReference m_OutputStream;
    NPT_HttpRequestContext    m_Context;
    bool                      m_Verbose;
};

/*----------------------------------------------------------------------
|   ZipHttpServer
+---------------------------------------------------------------------*/
class ZipHttpServer : public NPT_HttpServer {
public:
    ZipHttpServer(const char*  file_root,
                  const char*  url_root,
                  unsigned int port,
                  unsigned int threads);
    
    void Loop();
    void OnWorkerDone(ZipHttpWorker* worker);
    
private:
    NPT_Mutex                m_Lock;
    unsigned int             m_Threads;
    ZipRequestHandler*       m_Handler;
    NPT_List<ZipHttpWorker*> m_Workers;
    NPT_List<ZipHttpWorker*> m_ReadyWorkers;
    NPT_SharedVariable       m_AllWorkersBusy;
};

/*----------------------------------------------------------------------
|   ZipHttpServer::ZipHttpServer
+---------------------------------------------------------------------*/
ZipHttpServer::ZipHttpServer(const char*  file_root,
                             const char*  url_root,
                             unsigned int port,
                             unsigned int threads) :
        NPT_HttpServer(port),
        m_Threads(threads),
        m_AllWorkersBusy(0)
{
    m_Handler = new ZipRequestHandler(url_root, file_root);
    AddRequestHandler(m_Handler, url_root, true);
    
    for (unsigned int i=0; i<threads; i++) {
        ZipHttpWorker* worker = new ZipHttpWorker(i, this);
        m_Workers.Add(worker);
        m_ReadyWorkers.Add(worker);
        
        // start threads unless we're single threaded
        if (threads > 1) {
            worker->Start();
        }
    }
}

/*----------------------------------------------------------------------
|   ZipHttpServer::Loop
+---------------------------------------------------------------------*/
void
ZipHttpServer::Loop()
{
    for (;;) {
        // wait until at least one worker is ready
        if (m_AllWorkersBusy.GetValue() == 1) {
            NPT_LOG_FINEST("all workers busy");
        }
        NPT_LOG_FINEST("waiting for a worker");
        m_AllWorkersBusy.WaitUntilEquals(0);
        NPT_LOG_FINEST("got a worker");
        
        // pick a worker
        m_Lock.Lock();
        ZipHttpWorker* worker = NULL;
        m_ReadyWorkers.PopHead(worker);
        if (m_ReadyWorkers.GetItemCount() == 0) {
            m_AllWorkersBusy.SetValue(1);
        }
        m_Lock.Unlock();
        
        NPT_Result result = WaitForNewClient(worker->m_InputStream, worker->m_OutputStream, &worker->m_Context);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("WaitForNewClient returned %d", result);
            
            // wait a bit before continuing
            NPT_System::Sleep(NPT_TimeInterval(1.0));
        }

        if (m_Threads == 1) {
            // single threaded
            worker->Respond();
            OnWorkerDone(worker);
        } else {
            worker->m_State.SetValue(ZipHttpWorker::RUNNING);
        }
        worker = NULL;
    }

}

/*----------------------------------------------------------------------
|   ZipHttpWorker::OnWorkerDone
+---------------------------------------------------------------------*/
void
ZipHttpServer::OnWorkerDone(ZipHttpWorker* worker)
{
    NPT_LOG_FINEST_1("worker %d done", worker->m_Id);
    m_Lock.Lock();
    m_ReadyWorkers.Add(worker);
    m_AllWorkersBusy.SetValue(0);
    m_Lock.Unlock();
}

/*----------------------------------------------------------------------
|   ZipHttpWorker::Run
+---------------------------------------------------------------------*/
void
ZipHttpWorker::Run(void)
{
    NPT_LOG_FINE_1("worker %d started", m_Id);
    for (;;) {
        // wait while we're idle
        NPT_LOG_FINER_1("worker %d waiting for work", m_Id);
        m_State.WaitWhileEquals(IDLE);
        
        NPT_LOG_FINER_1("worker %d woke up", m_Id);

        if (m_State.GetValue() == DEAD) {
            NPT_LOG_FINE_1("worker %d exiting", m_Id);
            return;
        }
        
        // respond to the client
        Respond();
        
        // update our state
        m_State.SetValue(IDLE);
        
        // notify the server
        m_Server->OnWorkerDone(this);
    }
}

/*----------------------------------------------------------------------
|   ZipHttpWorker::Respond
+---------------------------------------------------------------------*/
NPT_Result
ZipHttpWorker::Respond()
{
    NPT_LOG_FINER_1("worker %d responding to request", m_Id);

    NPT_Result result = m_Server->RespondToClient(m_InputStream, m_OutputStream, m_Context);

    NPT_LOG_FINER_2("worker %d responded to request (%d)", m_Id, result);
    
    m_InputStream  = NULL;
    m_OutputStream = NULL;

    return result;
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** argv)
{
    NPT_String   file_root;
    NPT_String   url_root = "/";
    unsigned int port    = 8000;
    unsigned int threads = 5;
    bool         verbose = false;
    
    while (const char* arg = *++argv) {
        if (NPT_StringsEqual(arg, "--help") ||
            NPT_StringsEqual(arg, "-h")) {
            NPT_Console::Output("usage: ziphttpserver [--file-root <dir>] [--url-root <path>] [--port <port>] [--threads <n>] [--verbose]\n");
            return 0;
        } else if (NPT_StringsEqual(arg, "--file-root")) {
            arg = *++argv;
            if (arg == NULL) {
                NPT_Console::Output("ERROR: missing argument for --file-root option\n");
                return 1;
            }
            file_root = arg;
        } else if (NPT_StringsEqual(arg, "--url-root")) {
            arg = *++argv;
            if (arg == NULL) {
                NPT_Console::Output("ERROR: missing argument for --url-root option\n");
                return 1;
            }
            url_root = arg;
        } else if (NPT_StringsEqual(arg, "--port")) {
            arg = *++argv;
            if (arg == NULL) {
                NPT_Console::Output("ERROR: missing argument for --port option\n");
                return 1;
            }
            NPT_ParseInteger(arg, port, true);
        } else if (NPT_StringsEqual(arg, "--threads")) {
            arg = *++argv;
            if (arg == NULL) {
                NPT_Console::Output("ERROR: missing argument for --threads option\n");
                return 1;
            }
            NPT_ParseInteger(arg, threads, true);
        } else if (NPT_StringsEqual(arg, "--verbose")) {
            verbose = true;
        }
    }

    // sanity check on some parameters
    if (threads == 0 || threads > 20) {
        fprintf(stderr, "ERROR: --threads must be between 1 and 20");
        return 1;
    }
    
    // ensure the URL root start with a /
    if (!url_root.StartsWith("/")) {
        url_root = "/"+url_root;
    }
    
    // initialize the file type map
    for (unsigned int i=0; i<NPT_ARRAY_SIZE(DefaultFileTypeMap); i++) {
        FileTypeMap[DefaultFileTypeMap[i].extension] =DefaultFileTypeMap[i].mime_type;
    }
    
    if (file_root.GetLength() == 0) {
        NPT_File::GetWorkingDir(file_root);
    }
    
    if (verbose) {
        NPT_Console::OutputF("Starting server on port %d, file-root=%s, url-root=%s, threads=%d\n",
                             port, file_root.GetChars(), url_root.GetChars(), threads);
    }
    
    ZipHttpServer* server = new ZipHttpServer(file_root, url_root, port, threads);
    server->Loop();
    delete server;
    
    return 0;
}
