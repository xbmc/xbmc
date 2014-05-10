/*****************************************************************
|
|      Neptune Utilities - Network Benchmark utility
|
|      (c) 2001-2013 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include <stdio.h>

/*----------------------------------------------------------------------
|   Config
+---------------------------------------------------------------------*/
const unsigned int STATS_WINDOW_SIZE = 100;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(void)
{
    fprintf(stderr, 
            "NetBench [options] <url>\n"
            "\n"
            "  Options:\n"
            "    --threads <n> : use <n> independent threads for requests\n"
            "    --max-requests <n> : stop after <n> requests\n"
            "    --max-time <n> : stop after <n> seconds\n"
            "    --ssl-client-cert <filename> : load client TLS certificate from <filename> (PKCS12)\n"
            "    --ssl-client-cert-password <password> : optional password for the client cert\n"
    );
}

/*----------------------------------------------------------------------
|   Worker
+---------------------------------------------------------------------*/
class Worker : public NPT_Thread
{
public:
    Worker(const char*     url,
           NPT_TlsContext* tls_context,
           unsigned int    tls_options) :
        m_Url(url),
        m_Connector(NULL),
        m_Iterations(0),
        m_Failures(0),
        m_Done(false),
        m_ShouldStop(false)
    {
        if (tls_context) {
            m_Connector = new NPT_HttpTlsConnector(*tls_context, tls_options);
            m_Client.SetConnector(m_Connector);
        }
    }
    
    ~Worker() {
        delete m_Connector;
    }
    
    void Run() {
        // get the document
        NPT_HttpRequest request(m_Url, NPT_HTTP_METHOD_GET);
        
        while (!m_ShouldStop) {
            NPT_HttpResponse* response = NULL;

            NPT_Result result = m_Client.SendRequest(request, response);
            if (NPT_FAILED(result)) {
                ++m_Failures;
                continue;
            }

            // load body
            NPT_HttpEntity* entity = response->GetEntity();
            if (entity != NULL) {
                NPT_DataBuffer body;
                result = entity->Load(body);
                if (NPT_FAILED(result)) {
                    ++m_Failures;
                    continue;
                }
            }
        
            ++m_Iterations;
            delete response;
        }
    }
    
    NPT_HttpUrl                m_Url;
    NPT_HttpClient             m_Client;
    NPT_HttpClient::Connector* m_Connector;
    NPT_UInt32                 m_Iterations;
    NPT_UInt32                 m_Failures;
    volatile bool              m_Done;
    volatile bool              m_ShouldStop;
};

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    // check command line
    if (argc < 2) {
        PrintUsageAndExit();
        return 1;
    }

    // init options
    const char*  tls_cert_filename = NULL;
    const char*  tls_cert_password = NULL;
    unsigned int tls_options       = NPT_HttpTlsConnector::OPTION_ACCEPT_SELF_SIGNED_CERTS | NPT_HttpTlsConnector::OPTION_ACCEPT_HOSTNAME_MISMATCH;
    const char*  url               = NULL;
    unsigned int threads           = 1;
    unsigned int max_requests      = 0;
    unsigned int max_time          = 0;
    
    // parse command line
    ++argv;
    const char* arg;
    while ((arg = *argv++)) {
        if (NPT_StringsEqual(arg, "--threads")) {
            NPT_ParseInteger(*argv++, threads);
            if (threads < 1) threads = 1;
        } else if (NPT_StringsEqual(arg, "--max-requests")) {
            NPT_ParseInteger(*argv++, max_requests);
        } else if (NPT_StringsEqual(arg, "--max-time")) {
            NPT_ParseInteger(*argv++, max_time);
        } else if (NPT_StringsEqual(arg, "--ssl-client-cert")) {
            tls_cert_filename = *argv++;
            if (tls_cert_filename == NULL) {
                fprintf(stderr, "ERROR: missing argument after --ssl-client-cert option\n");
                return 1;
            }
        } else if (NPT_StringsEqual(arg, "--ssl-client-cert-password")) {
            tls_cert_password = *argv++;
            if (tls_cert_password == NULL) {
                fprintf(stderr, "ERROR: missing argument after --ssl-client-cert-password option\n");
                return 1;
            }
        } else if (url == NULL) {
            url = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }
   
    // load a client cert if needed
    NPT_TlsContext* tls_context = NULL;
    if (tls_options || tls_cert_filename) {
        tls_context = new NPT_TlsContext(NPT_TlsContext::OPTION_VERIFY_LATER | NPT_TlsContext::OPTION_ADD_DEFAULT_TRUST_ANCHORS/* | NPT_TlsContext::OPTION_NO_SESSION_CACHE*/);
        if (tls_cert_filename) {
            NPT_DataBuffer cert;
            NPT_Result result = NPT_File::Load(tls_cert_filename, cert);
            if (NPT_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to load client cert from file %s (%d)\n", tls_cert_filename, result);
                return 1;
            }
            result = tls_context->LoadKey(NPT_TLS_KEY_FORMAT_PKCS12, cert.GetData(), cert.GetDataSize(), tls_cert_password);
            if (NPT_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to parse client cert (%d)\n", result);
                return 1;
            }
        }
    }
    
    NPT_Array<Worker*> workers;
    for (unsigned int i=0; i<threads; i++) {
        Worker* worker = new Worker(url, tls_context, tls_options);
        workers.Add(worker);
        worker->Start();
    }

    NPT_TimeStamp start_time;
    NPT_System::GetCurrentTimeStamp(start_time);
    
    struct {
        unsigned int  request_count;
        unsigned int  failure_count;
        NPT_TimeStamp timestamp;
    } stats[STATS_WINDOW_SIZE];
    unsigned int cursor = 0;
    for (unsigned int loop = 0; true; loop++) {
        unsigned int total_requests = 0;
        unsigned int total_failures = 0;
        bool         all_done = true;
        for (unsigned int i=0; i<threads; i++) {
            total_requests += workers[i]->m_Iterations;
            total_failures += workers[i]->m_Failures;
            if (!workers[i]->m_Done) all_done = false;
        }
        NPT_TimeStamp now;
        NPT_System::GetCurrentTimeStamp(now);
        stats[cursor].timestamp     = now;
        stats[cursor].request_count = total_requests;
        stats[cursor].failure_count = total_failures;
        
        int newest = cursor;
        int oldest = (cursor+1)%STATS_WINDOW_SIZE;
        if (loop < STATS_WINDOW_SIZE) {
            oldest = 0;
        }
        unsigned int  reqs_in_window  = stats[newest].request_count - stats[oldest].request_count;
        NPT_TimeStamp window_duration = stats[newest].timestamp - stats[oldest].timestamp;
        double rate = 0.0;
        if (window_duration.ToMillis()) {
            rate = 1000.0*(double)reqs_in_window/(double)window_duration.ToMillis();
        }
        printf("\rReqs: %d - Fail: %d - Rate: %.2f tps", total_requests, total_failures, (float)rate);
        fflush(stdout);

        cursor = (cursor+1)%STATS_WINDOW_SIZE;
        
        if (max_time && (now-start_time).ToSeconds() >= max_time) {
            break;
        }
        if (max_requests && total_requests >= max_requests) {
            break;
        }
        if (all_done) {
            break;
        }
        NPT_System::Sleep(0.1);
    }
    printf("\n");

    for (unsigned int i=0; i<threads; i++) {
        workers[i]->m_ShouldStop = true;
    }
    
    for (unsigned int i=0; i<threads; i++) {
        workers[i]->Wait();
        delete workers[i];
    }
    
    delete tls_context;
    
    return 0;
}




