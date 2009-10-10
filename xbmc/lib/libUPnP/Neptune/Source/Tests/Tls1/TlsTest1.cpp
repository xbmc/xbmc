/*****************************************************************
|
|      TLS Test Program 1
|
|      (c) 2001-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "NptDebug.h"

#include "TlsClientPrivate1.h"
#include "TlsClientPrivate2.h"

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#define CHECK(x)                                        \
    do {                                                \
      if (!(x)) {                                       \
        fprintf(stderr, "ERROR line %d \n", __LINE__);  \
      }                                                 \
    } while(0)

const char*
GetCipherSuiteName(unsigned int id)
{
    switch (id) {
        case 0: return "NOT SET";
        case NPT_TLS_RSA_WITH_RC4_128_MD5:     return "RSA-WITH-RC4-128-MD5";
        case NPT_TLS_RSA_WITH_RC4_128_SHA:     return "RSA-WITH-RC4-128-SHA";
        case NPT_TLS_RSA_WITH_AES_128_CBC_SHA: return "RSA-WITH-AES-128-CBC-SHA";
        case NPT_TLS_RSA_WITH_AES_256_CBC_SHA: return "RSA-WITH-AES-256-CBC-SHA";
        default: return "UNKNOWN";
    }
}

static void 
TestPrivateKeys()
{
    NPT_TlsContext context;
    NPT_Result     result;

    NPT_DataBuffer key_data;
    NPT_Base64::Decode(TestClient_rsa_priv_base64_1, NPT_StringLength(TestClient_rsa_priv_base64_1), key_data);
    
    result = context.LoadKey(NPT_TLS_KEY_FORMAT_RSA_PRIVATE, key_data.GetData(), key_data.GetDataSize(), NULL);
    CHECK(result == NPT_SUCCESS);

    result = context.LoadKey(NPT_TLS_KEY_FORMAT_PKCS8, TestClient_p8_1, TestClient_p8_1_len, NULL);
    CHECK(result != NPT_SUCCESS);
    result = context.LoadKey(NPT_TLS_KEY_FORMAT_PKCS8, TestClient_p8_1, TestClient_p8_1_len, "neptune");
    CHECK(result == NPT_SUCCESS);
}

int 
main(int /*argc*/, char** /*argv*/)
{
    TestPrivateKeys();
    
    /* test a connection */
    const char* hostname = "koala.bok.net";
    printf("[1] Connecting to %s...\n", hostname);
    NPT_Socket* client_socket = new NPT_TcpClientSocket();
    NPT_IpAddress server_ip;
    server_ip.ResolveName(hostname);
    NPT_SocketAddress server_addr(server_ip, 443);
    NPT_Result result = client_socket->Connect(server_addr);
    printf("[2] Connection result = %d (%s)\n", result, NPT_ResultText(result));
    if (NPT_FAILED(result)) {
        printf("!ERROR\n");
        return 1;
    }
    
    NPT_InputStreamReference input;
    NPT_OutputStreamReference output;
    client_socket->GetInputStream(input);
    client_socket->GetOutputStream(output);
    NPT_TlsContextReference context(new NPT_TlsContext());
    NPT_TlsClientSession session(context, input, output);
    printf("[3] Performing Handshake\n");
    result = session.Handshake();
    printf("[4] Handshake Result = %d (%s)\n", result, NPT_ResultText(result));
    if (NPT_FAILED(result)) {
        printf("!ERROR\n");
        return 1;
    }
    NPT_DataBuffer session_id;
    result = session.GetSessionId(session_id);
    CHECK(result == NPT_SUCCESS);
    CHECK(session_id.GetDataSize() > 0);
    printf("[5] Session ID: ");
    printf(NPT_HexString(session_id.GetData(), session_id.GetDataSize()).GetChars());
    printf("\n");
    
    NPT_TlsCertificateInfo cert_info;
    result = session.GetPeerCertificateInfo(cert_info);
    CHECK(result == NPT_SUCCESS);
    printf("[6] Fingerprints:\n");
    printf("MD5: %s\n", NPT_HexString(cert_info.fingerprint.md5, sizeof(cert_info.fingerprint.md5), ":").GetChars());
    printf("SHA1: %s\n", NPT_HexString(cert_info.fingerprint.sha1, sizeof(cert_info.fingerprint.sha1), ":").GetChars());
    printf("Subject Certificate:\n");
    printf("  Common Name         = %s\n", cert_info.subject.common_name.GetChars());
    printf("  Organization        = %s\n", cert_info.subject.organization.GetChars());
    printf("  Organizational Name = %s\n", cert_info.subject.organizational_name.GetChars());
    printf("Issuer Certificate:\n");
    printf("  Common Name         = %s\n", cert_info.issuer.common_name.GetChars());
    printf("  Organization        = %s\n", cert_info.issuer.organization.GetChars());
    printf("  Organizational Name = %s\n", cert_info.issuer.organizational_name.GetChars());
    printf("\n");
    printf("[7] Cipher Type = %d (%s)\n", session.GetCipherSuiteId(), GetCipherSuiteName(session.GetCipherSuiteId()));
    
    NPT_InputStreamReference  ssl_input;
    NPT_OutputStreamReference ssl_output;
    session.GetInputStream(ssl_input);
    session.GetOutputStream(ssl_output);
    
    printf("[8] Getting / Document\n");
    ssl_output->WriteString("GET / HTTP/1.0\n\n");
    for (;;) {
        unsigned char buffer[1];
        NPT_Size bytes_read = 0;
        result = ssl_input->Read(&buffer[0], 1, &bytes_read);
        if (NPT_SUCCEEDED(result)) {
            CHECK(bytes_read == 1);
            printf("%c", buffer[0]);
        } else {
            if (result != NPT_ERROR_EOS) {
                printf("!ERROR: Read() returned %d (%s)\n", result, NPT_ResultText(result)); 
            }
            break;
        }
    }
    printf("[9] SUCCESS\n");
}
