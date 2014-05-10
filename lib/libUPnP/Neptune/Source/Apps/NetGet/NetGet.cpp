/*****************************************************************
|
|      Neptune Utilities - Network 'Get' Client
|
|      (c) 2001-2010 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include <stdio.h>

/*----------------------------------------------------------------------
|       PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(void)
{
    fprintf(stderr, 
            "NetGet [options] <url>\n"
            "\n"
            "  Options:\n"
            "    --verbose : print verbose information\n"
            "    --no-body-output : do not output the response body\n"
            "    --http-1-1 : use HTTP 1.1\n"
#if defined(NPT_CONFIG_ENABLE_TLS)
            "    --ssl-client-cert <filename> : load client TLS certificate from <filename> (PKCS12)\n"
            "    --ssl-client-cert-password <password> : optional password for the client cert\n"
            "    --ssl-accept-self-signed-certs : accept self-signed server certificates\n"
            "    --ssl-accept-hostname-mismatch : accept server certificates that don't match\n"
#endif
            "    --show-proxy : show the proxy that will be used for the connection\n");
}

/*----------------------------------------------------------------------
|       main
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
    bool verbose        = false;
    bool no_body_output = false;
    bool show_proxy     = false;
    bool url_set        = false;
    bool http_1_1       = false;
    NPT_HttpUrl url;
    NPT_HttpClient::Connector* connector = NULL;
#if defined(NPT_CONFIG_ENABLE_TLS)
    NPT_TlsContext*    tls_context = NULL;
    const char*        tls_cert_filename = NULL;
    const char*        tls_cert_password = NULL;
    unsigned int       tls_options = 0;
#endif

    // parse command line
    ++argv;
    const char* arg;
    while ((arg = *argv++)) {
        if (NPT_StringsEqual(arg, "--verbose")) {
            verbose = true;
        } else if (NPT_StringsEqual(arg, "--show-proxy")) {
            show_proxy = true;
        } else if (NPT_StringsEqual(arg, "--no-body-output")) {
            no_body_output = true;
        } else if (NPT_StringsEqual(arg, "--http-1-1")) {
            http_1_1 = true;
#if defined(NPT_CONFIG_ENABLE_TLS)
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
        } else if (NPT_StringsEqual(arg, "--ssl-accept-self-signed-certs")) {
            tls_options |= NPT_HttpTlsConnector::OPTION_ACCEPT_SELF_SIGNED_CERTS;
        } else if (NPT_StringsEqual(arg, "--ssl-accept-hostname-mismatch")) {
            tls_options |= NPT_HttpTlsConnector::OPTION_ACCEPT_HOSTNAME_MISMATCH;
#endif
        } else if (!url_set) {
            NPT_Result result = url.Parse(arg);
            if (NPT_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to parse URL (%d:%s)\n", result, NPT_ResultText(result));
                return 1;
            }
            url_set = true;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }

    if (show_proxy) {
        NPT_HttpProxyAddress proxy;
        NPT_HttpProxySelector* selector = NPT_HttpProxySelector::GetDefault();
        if (selector) {
            NPT_Result result = selector->GetProxyForUrl(url, proxy);
            if (NPT_FAILED(result) && result != NPT_ERROR_HTTP_NO_PROXY) {
                fprintf(stderr, "ERROR: proxy selector error (%d:%s)\n", result, NPT_ResultText(result));
                return 1;
            }
        } 
        if (proxy.GetHostName().IsEmpty()) {
            printf("PROXY: none\n");
        } else {
            printf("PROXY: %s:%d\n", proxy.GetHostName().GetChars(), proxy.GetPort());
        }
    }
    
#if defined(NPT_CONFIG_ENABLE_TLS)
    // load a client cert if needed
    if (tls_options || tls_cert_filename) {
        tls_context = new NPT_TlsContext(NPT_TlsContext::OPTION_VERIFY_LATER | NPT_TlsContext::OPTION_ADD_DEFAULT_TRUST_ANCHORS);

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
        
        connector = new NPT_HttpTlsConnector(*tls_context, tls_options);
    }
#endif

    // get the document
    NPT_HttpRequest request(url, NPT_HTTP_METHOD_GET);
    NPT_HttpClient client;
    NPT_HttpResponse* response;
    if (http_1_1) {
        request.SetProtocol(NPT_HTTP_PROTOCOL_1_1);
    }
    if (connector) {
        client.SetConnector(connector);
    }

    NPT_TimeStamp before_request;
    NPT_System::GetCurrentTimeStamp(before_request);

    NPT_Result result = client.SendRequest(request, response);
    if (NPT_FAILED(result)) {
        fprintf(stderr, "ERROR: SendRequest failed (%d:%s)\n", result, NPT_ResultText(result));
        return 1;
    }

    NPT_TimeStamp before_body;
    NPT_System::GetCurrentTimeStamp(before_body);

    // show the request info
    if (verbose) {
        printf("#REQUEST: protocol=%s\n", request.GetProtocol().GetChars());

        // show headers
        NPT_HttpHeaders& headers = request.GetHeaders();
        NPT_List<NPT_HttpHeader*>::Iterator header = headers.GetHeaders().GetFirstItem();
        while (header) {
            printf("%s: %s\n", 
                      (const char*)(*header)->GetName(),
                      (const char*)(*header)->GetValue());
            ++header;
        }
    }

    // show response info
    if (verbose) {
        printf("\n#RESPONSE: protocol=%s, code=%d, reason=%s\n",
               response->GetProtocol().GetChars(),
               response->GetStatusCode(),
               response->GetReasonPhrase().GetChars());

        // show headers
        NPT_HttpHeaders& headers = response->GetHeaders();
        NPT_List<NPT_HttpHeader*>::Iterator header = headers.GetHeaders().GetFirstItem();
        while (header) {
            printf("%s: %s\n", 
                      (const char*)(*header)->GetName(),
                      (const char*)(*header)->GetValue());
            ++header;
        }
    }
    
    // show entity
    NPT_Size body_size = 0;
    NPT_HttpEntity* entity = response->GetEntity();
    if (entity != NULL) {
        if (verbose) {
            printf("\n#ENTITY: length=%lld, type=%s, encoding=%s\n",
                   entity->GetContentLength(),
                   entity->GetContentType().GetChars(),
                   entity->GetContentEncoding().GetChars());
        }
        
        if (verbose) {
            NPT_InputStreamReference body_stream;
            entity->GetInputStream(body_stream);
            if (!body_stream.IsNull()) {
                NPT_LargeSize size;
                body_stream->GetSize(size);
                printf("Loading body stream (declared: %lld bytes)\n", size);
            }
        }
        NPT_DataBuffer body;
        result =entity->Load(body);
        if (NPT_FAILED(result)) {
            fprintf(stderr, "ERROR: failed to load entity (%d)\n", result);
        } else {
            body_size = body.GetDataSize();
            if (verbose) printf("\n#BODY: loaded %d bytes\n", (int)body_size);

            // dump the body
            if (!no_body_output) {
                NPT_OutputStreamReference output;
                NPT_File standard_out(NPT_FILE_STANDARD_OUTPUT);
                standard_out.Open(NPT_FILE_OPEN_MODE_WRITE);
                standard_out.GetOutputStream(output);
                output->Write(body.GetData(), body.GetDataSize());
            }
        }
    }

    NPT_TimeStamp after_body;
    NPT_System::GetCurrentTimeStamp(after_body);

    if (verbose) {
        unsigned int request_latency = (unsigned int)(before_body-before_request).ToMillis();
        unsigned int body_load_time  = (unsigned int)(after_body-before_body).ToMillis();
        unsigned int total_load_time = (unsigned int)(after_body-before_request).ToMillis();
        unsigned int body_throughput = 0;
        if (body_size && body_load_time) {
            body_throughput = (unsigned int)((8.0 * (double)body_size)/1000.0)/((double)body_load_time/1000.0);
        }
        unsigned int total_throughput = 0;
        if (body_size && total_load_time) {
            total_throughput = (unsigned int)((8.0*(double)body_size)/1000.0)/((double)total_load_time/1000.0);
        }
        
        printf("\n-----------------------------------------------------------\n");
        printf("TIMING:\n");
        printf("  Request Latency  = %d ms\n",   request_latency);
        printf("  Body Load Time   = %d ms\n",   body_load_time);
        printf("  Total Load Time  = %d ms\n",   total_load_time);
        printf("  Body Throughput  = %d kbps\n", body_throughput);
        printf("  Total Throughput = %d kbps\n", total_throughput);
    }
    
    delete response;
    delete connector;
#if defined(NPT_CONFIG_ENABLE_TLS)
    delete tls_context;
#endif

    return 0;
}




