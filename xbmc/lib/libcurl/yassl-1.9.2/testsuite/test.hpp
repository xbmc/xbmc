// test.hpp

#ifndef yaSSL_TEST_HPP
#define yaSSL_TEST_HPP

#include "runtime.hpp"
#include "openssl/ssl.h"   /* openssl compatibility test */
#include "error.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//#define NON_BLOCKING  // test server and client example (not echos)

#ifdef _WIN32
    #include <winsock2.h>
    #include <process.h>
    #ifdef TEST_IPV6            // don't require newer SDK for IPV4
	    #include <ws2tcpip.h>
        #include <wspiapi.h>
    #endif
    #define SOCKET_T unsigned int
#else
    #include <string.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #ifdef TEST_IPV6
        #include <netdb.h>
    #endif
    #include <pthread.h>
#ifdef NON_BLOCKING
    #include <fcntl.h>
#endif
    #define SOCKET_T int
#endif /* _WIN32 */


#ifdef _MSC_VER
    // disable conversion warning
    // 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy
    #pragma warning(disable:4244 4996)
#endif


#if !defined(_SOCKLEN_T) && \
 (defined(_WIN32) || defined(__NETWARE__) || defined(__APPLE__))
    typedef int socklen_t;
#endif


// Check type of third arg to accept
#if defined(__hpux)
// HPUX uses int* for third parameter to accept
    typedef int*       ACCEPT_THIRD_T;
#elif defined(__NETWARE__)
// NetWare uses size_t* for third parameter to accept
    typedef size_t*       ACCEPT_THIRD_T;
#else
    typedef socklen_t* ACCEPT_THIRD_T;
#endif


#ifdef TEST_IPV6
    typedef sockaddr_in6 SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET6
#else
    typedef sockaddr_in  SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET
#endif
   

// Check if _POSIX_THREADS should be forced
#if !defined(_POSIX_THREADS) && (defined(__NETWARE__) || defined(__hpux))
// HPUX does not define _POSIX_THREADS as it's not _fully_ implemented
// Netware supports pthreads but does not announce it
#define _POSIX_THREADS
#endif


#ifndef _POSIX_THREADS
    typedef unsigned int  THREAD_RETURN;
    typedef HANDLE        THREAD_TYPE;
    #define YASSL_API __stdcall
#else
    typedef void*         THREAD_RETURN;
    typedef pthread_t     THREAD_TYPE;
    #define YASSL_API 
#endif


struct tcp_ready {
#ifdef _POSIX_THREADS
    pthread_mutex_t mutex_;
    pthread_cond_t  cond_;
    bool            ready_;   // predicate

    tcp_ready() : ready_(false)
    {
        pthread_mutex_init(&mutex_, 0);
        pthread_cond_init(&cond_, 0);
    }

    ~tcp_ready()
    {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&cond_);
    }
#endif
};    


struct func_args {
    int    argc;
    char** argv;
    int    return_code;
    tcp_ready* signal_;

    func_args(int c = 0, char** v = 0) : argc(c), argv(v) {}

    void SetSignal(tcp_ready* p) { signal_ = p; }
};

typedef THREAD_RETURN YASSL_API THREAD_FUNC(void*);

void start_thread(THREAD_FUNC, func_args*, THREAD_TYPE*);
void join_thread(THREAD_TYPE);

// yaSSL
const char* const    yasslIP      = "127.0.0.1";
const unsigned short yasslPort    =  11111;


// client
const char* const cert = "../certs/client-cert.pem";
const char* const key  = "../certs/client-key.pem";

const char* const certSuite = "../../certs/client-cert.pem";
const char* const keySuite  = "../../certs/client-key.pem";

const char* const certDebug = "../../../certs/client-cert.pem";
const char* const keyDebug  = "../../../certs/client-key.pem";


// server
const char* const svrCert = "../certs/server-cert.pem";
const char* const svrKey  = "../certs/server-key.pem";

const char* const svrCert2 = "../../certs/server-cert.pem";
const char* const svrKey2  = "../../certs/server-key.pem";

const char* const svrCert3 = "../../../certs/server-cert.pem";
const char* const svrKey3  = "../../../certs/server-key.pem";


// server dsa
const char* const dsaCert = "../certs/dsa-cert.pem";
const char* const dsaKey  = "../certs/dsa512.der";

const char* const dsaCert2 = "../../certs/dsa-cert.pem";
const char* const dsaKey2  = "../../certs/dsa512.der";

const char* const dsaCert3 = "../../../certs/dsa-cert.pem";
const char* const dsaKey3  = "../../../certs/dsa512.der";


// CA 
const char* const caCert  = "../certs/ca-cert.pem";
const char* const caCert2 = "../../certs/ca-cert.pem";
const char* const caCert3 = "../../../certs/ca-cert.pem";


using namespace yaSSL;


inline void err_sys(const char* msg)
{
    printf("yassl error: %s\n", msg);
    exit(EXIT_FAILURE);
}


static int PasswordCallBack(char* passwd, int sz, int rw, void* userdata)
{
    strncpy(passwd, "12345678", sz);
    return 8;
}


inline void store_ca(SSL_CTX* ctx)
{
    // To allow testing from serveral dirs
    if (SSL_CTX_load_verify_locations(ctx, caCert, 0) != SSL_SUCCESS)
        if (SSL_CTX_load_verify_locations(ctx, caCert2, 0) != SSL_SUCCESS)
            if (SSL_CTX_load_verify_locations(ctx, caCert3, 0) != SSL_SUCCESS)
                err_sys("failed to use certificate: certs/cacert.pem");

    // load client CA for server verify
    if (SSL_CTX_load_verify_locations(ctx, cert, 0) != SSL_SUCCESS)
        if (SSL_CTX_load_verify_locations(ctx, certSuite, 0) != SSL_SUCCESS)
            if (SSL_CTX_load_verify_locations(ctx, certDebug,0) != SSL_SUCCESS)
                err_sys("failed to use certificate: certs/client-cert.pem");
}


// client
inline void set_certs(SSL_CTX* ctx)
{
    store_ca(ctx);
    SSL_CTX_set_default_passwd_cb(ctx, PasswordCallBack);

    // To allow testing from serveral dirs
    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM)
        != SSL_SUCCESS)
        if (SSL_CTX_use_certificate_file(ctx, certSuite, SSL_FILETYPE_PEM)
            != SSL_SUCCESS)
            if (SSL_CTX_use_certificate_file(ctx, certDebug, SSL_FILETYPE_PEM)
                != SSL_SUCCESS)
                err_sys("failed to use certificate: certs/client-cert.pem");
    
    // To allow testing from several dirs
    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM)
         != SSL_SUCCESS) 
         if (SSL_CTX_use_PrivateKey_file(ctx, keySuite, SSL_FILETYPE_PEM)
            != SSL_SUCCESS) 
                if (SSL_CTX_use_PrivateKey_file(ctx,keyDebug,SSL_FILETYPE_PEM)
                    != SSL_SUCCESS) 
                    err_sys("failed to use key file: certs/client-key.pem");
}


// server
inline void set_serverCerts(SSL_CTX* ctx)
{
    store_ca(ctx);
    SSL_CTX_set_default_passwd_cb(ctx, PasswordCallBack);

    // To allow testing from serveral dirs
    if (SSL_CTX_use_certificate_file(ctx, svrCert, SSL_FILETYPE_PEM)
        != SSL_SUCCESS)
        if (SSL_CTX_use_certificate_file(ctx, svrCert2, SSL_FILETYPE_PEM)
            != SSL_SUCCESS)
            if (SSL_CTX_use_certificate_file(ctx, svrCert3, SSL_FILETYPE_PEM)
                != SSL_SUCCESS)
                err_sys("failed to use certificate: certs/server-cert.pem");
    
    // To allow testing from several dirs
    if (SSL_CTX_use_PrivateKey_file(ctx, svrKey, SSL_FILETYPE_PEM)
         != SSL_SUCCESS) 
         if (SSL_CTX_use_PrivateKey_file(ctx, svrKey2, SSL_FILETYPE_PEM)
            != SSL_SUCCESS) 
                if (SSL_CTX_use_PrivateKey_file(ctx, svrKey3,SSL_FILETYPE_PEM)
                    != SSL_SUCCESS) 
                    err_sys("failed to use key file: certs/server-key.pem");
}


// dsa server
inline void set_dsaServerCerts(SSL_CTX* ctx)
{
    store_ca(ctx);

    // To allow testing from serveral dirs
    if (SSL_CTX_use_certificate_file(ctx, dsaCert, SSL_FILETYPE_PEM)
        != SSL_SUCCESS)
        if (SSL_CTX_use_certificate_file(ctx, dsaCert2, SSL_FILETYPE_PEM)
            != SSL_SUCCESS)
            if (SSL_CTX_use_certificate_file(ctx, dsaCert3, SSL_FILETYPE_PEM)
                != SSL_SUCCESS)
                err_sys("failed to use certificate: certs/dsa-cert.pem");
    
    // To allow testing from several dirs
    if (SSL_CTX_use_PrivateKey_file(ctx, dsaKey, SSL_FILETYPE_ASN1)
         != SSL_SUCCESS) 
         if (SSL_CTX_use_PrivateKey_file(ctx, dsaKey2, SSL_FILETYPE_ASN1)
            != SSL_SUCCESS) 
                if (SSL_CTX_use_PrivateKey_file(ctx, dsaKey3,SSL_FILETYPE_ASN1)
                    != SSL_SUCCESS) 
                    err_sys("failed to use key file: certs/dsa512.der");
}


inline void set_args(int& argc, char**& argv, func_args& args)
{
    argc = args.argc;
    argv = args.argv;
    args.return_code = -1; // error state
}


inline void tcp_set_nonblocking(SOCKET_T& sockfd)
{
#ifdef NON_BLOCKING
    #ifdef _WIN32
        unsigned long blocking = 1;
        int ret = ioctlsocket(sockfd, FIONBIO, &blocking);
    #else
        int flags = fcntl(sockfd, F_GETFL, 0);
        int ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    #endif
#endif
}


inline void tcp_socket(SOCKET_T& sockfd, SOCKADDR_IN_T& addr)
{
    sockfd = socket(AF_INET_V, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));

#ifdef TEST_IPV6
    addr.sin6_family = AF_INET_V;
    addr.sin6_port = htons(yasslPort);
    addr.sin6_addr = in6addr_loopback;

    /* // for external testing later 
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET_V;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    getaddrinfo(yasslIP6, yasslPortStr, &hints, info);
    // then use info connect(sockfd, info->ai_addr, info->ai_addrlen)

    if (*info == 0)
        err_sys("getaddrinfo failed");
        */   // end external testing later
#else
    addr.sin_family = AF_INET_V;
    addr.sin_port = htons(yasslPort);
    addr.sin_addr.s_addr = inet_addr(yasslIP);
#endif

}


inline void tcp_close(SOCKET_T& sockfd)
{
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
    sockfd = (SOCKET_T) -1;
}


inline void tcp_connect(SOCKET_T& sockfd)
{
    SOCKADDR_IN_T addr;
    tcp_socket(sockfd, addr);

    if (connect(sockfd, (const sockaddr*)&addr, sizeof(addr)) != 0) {
        tcp_close(sockfd);
        err_sys("tcp connect failed");
    }
}


inline void tcp_listen(SOCKET_T& sockfd)
{
    SOCKADDR_IN_T addr;
    tcp_socket(sockfd, addr);

    if (bind(sockfd, (const sockaddr*)&addr, sizeof(addr)) != 0) {
        tcp_close(sockfd);
        err_sys("tcp bind failed");
    }
    if (listen(sockfd, 3) != 0) {
        tcp_close(sockfd);
        err_sys("tcp listen failed");
    }
}



inline void tcp_accept(SOCKET_T& sockfd, SOCKET_T& clientfd, func_args& args)
{
    tcp_listen(sockfd);

    SOCKADDR_IN_T client;
    socklen_t client_len = sizeof(client);

#if defined(_POSIX_THREADS) && defined(NO_MAIN_DRIVER)
    // signal ready to tcp_accept
    tcp_ready& ready = *args.signal_;
    pthread_mutex_lock(&ready.mutex_);
    ready.ready_ = true;
    pthread_cond_signal(&ready.cond_);
    pthread_mutex_unlock(&ready.mutex_);
#endif

    clientfd = accept(sockfd, (sockaddr*)&client, (ACCEPT_THIRD_T)&client_len);

    if (clientfd == (SOCKET_T) -1) {
        tcp_close(sockfd);
        err_sys("tcp accept failed");
    }

#ifdef NON_BLOCKING
    tcp_set_nonblocking(clientfd);
#endif
}


inline void showPeer(SSL* ssl)
{
    X509* peer = SSL_get_peer_certificate(ssl);
    if (peer) {
        char* issuer  = X509_NAME_oneline(X509_get_issuer_name(peer), 0, 0);
        char* subject = X509_NAME_oneline(X509_get_subject_name(peer), 0, 0);

        printf("peer's cert info:\n issuer : %s\n subject: %s\n", issuer,
                                                                  subject);
        free(subject);
        free(issuer);
    }
    else
        printf("peer has no cert!\n");
}



inline DH* set_tmpDH(SSL_CTX* ctx)
{
    static unsigned char dh1024_p[] =
    {
        0xE6, 0x96, 0x9D, 0x3D, 0x49, 0x5B, 0xE3, 0x2C, 0x7C, 0xF1, 0x80, 0xC3,
        0xBD, 0xD4, 0x79, 0x8E, 0x91, 0xB7, 0x81, 0x82, 0x51, 0xBB, 0x05, 0x5E,
        0x2A, 0x20, 0x64, 0x90, 0x4A, 0x79, 0xA7, 0x70, 0xFA, 0x15, 0xA2, 0x59,
        0xCB, 0xD5, 0x23, 0xA6, 0xA6, 0xEF, 0x09, 0xC4, 0x30, 0x48, 0xD5, 0xA2,
        0x2F, 0x97, 0x1F, 0x3C, 0x20, 0x12, 0x9B, 0x48, 0x00, 0x0E, 0x6E, 0xDD,
        0x06, 0x1C, 0xBC, 0x05, 0x3E, 0x37, 0x1D, 0x79, 0x4E, 0x53, 0x27, 0xDF,
        0x61, 0x1E, 0xBB, 0xBE, 0x1B, 0xAC, 0x9B, 0x5C, 0x60, 0x44, 0xCF, 0x02,
        0x3D, 0x76, 0xE0, 0x5E, 0xEA, 0x9B, 0xAD, 0x99, 0x1B, 0x13, 0xA6, 0x3C,
        0x97, 0x4E, 0x9E, 0xF1, 0x83, 0x9E, 0xB5, 0xDB, 0x12, 0x51, 0x36, 0xF7,
        0x26, 0x2E, 0x56, 0xA8, 0x87, 0x15, 0x38, 0xDF, 0xD8, 0x23, 0xC6, 0x50,
        0x50, 0x85, 0xE2, 0x1F, 0x0D, 0xD5, 0xC8, 0x6B,
    };

    static unsigned char dh1024_g[] =
    {
      0x02,
    };

    DH* dh;
    if ( (dh = DH_new()) ) {
        dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), 0);
        dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), 0);
    }
    if (!dh->p || !dh->g) {
        DH_free(dh);
        dh = 0;
    }
    SSL_CTX_set_tmp_dh(ctx, dh);
    return dh;
}


inline int verify_callback(int preverify_ok, X509_STORE_CTX* ctx)
{
    X509* err_cert = X509_STORE_CTX_get_current_cert(ctx);
    int   err      = X509_STORE_CTX_get_error(ctx);
    int   depth    = X509_STORE_CTX_get_error_depth(ctx);

    // test allow self signed
    if (depth == 0 && err == TaoCrypt::SIG_OTHER_E)
        return 1;

    return 0;
}


#endif // yaSSL_TEST_HPP

