/* client.cpp  */

#include "../../testsuite/test.hpp"

//#define TEST_RESUME


void ClientError(SSL_CTX* ctx, SSL* ssl, SOCKET_T& sockfd, const char* msg)
{
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    tcp_close(sockfd);
    err_sys(msg);
}


#ifdef NON_BLOCKING
    void NonBlockingSSL_Connect(SSL* ssl, SSL_CTX* ctx, SOCKET_T& sockfd)
    {
        int ret = SSL_connect(ssl);
        while (ret =! SSL_SUCCESS && (SSL_get_error(ssl, 0) ==
                                     SSL_ERROR_WANT_READ)) {
            printf("... client would block\n");
            #ifdef _WIN32
                Sleep(1000);
            #else
                sleep(1);
            #endif
            ret = SSL_connect(ssl);
        }
        if (ret != SSL_SUCCESS)
            ClientError(ctx, ssl, sockfd, "SSL_connect failed");
    }
#endif


void client_test(void* args)
{
#ifdef _WIN32
    WSADATA wsd;
    WSAStartup(0x0002, &wsd);
#endif

    SOCKET_T sockfd = 0;
    int      argc = 0;
    char**   argv = 0;

    set_args(argc, argv, *static_cast<func_args*>(args));
    tcp_connect(sockfd);
#ifdef NON_BLOCKING
    tcp_set_nonblocking(sockfd);
#endif

    SSL_METHOD* method = TLSv1_client_method();
    SSL_CTX*    ctx = SSL_CTX_new(method);

    set_certs(ctx);
    SSL* ssl = SSL_new(ctx);

    SSL_set_fd(ssl, sockfd);


#ifdef NON_BLOCKING
    NonBlockingSSL_Connect(ssl, ctx, sockfd);
#else
    // if you get an error here see note at top of README
    if (SSL_connect(ssl) != SSL_SUCCESS)   
        ClientError(ctx, ssl, sockfd, "SSL_connect failed");
#endif
    showPeer(ssl);

    const char* cipher = 0;
    int index = 0;
    char list[1024];
    strncpy(list, "cipherlist", 11);
    while ( (cipher = SSL_get_cipher_list(ssl, index++)) ) {
        strncat(list, ":", 2);
        strncat(list, cipher, strlen(cipher) + 1);
    }
    printf("%s\n", list);
    printf("Using Cipher Suite: %s\n", SSL_get_cipher(ssl));

    char msg[] = "hello yassl!";
    if (SSL_write(ssl, msg, sizeof(msg)) != sizeof(msg))
        ClientError(ctx, ssl, sockfd, "SSL_write failed");

    char reply[1024];
    int input = SSL_read(ssl, reply, sizeof(reply));
    if (input > 0) {
        reply[input] = 0;
        printf("Server response: %s\n", reply);
    }

#ifdef TEST_RESUME
    SSL_SESSION* session   = SSL_get_session(ssl);
    SSL*         sslResume = SSL_new(ctx);
#endif

    SSL_shutdown(ssl);
    SSL_free(ssl);
    tcp_close(sockfd);

#ifdef TEST_RESUME
    tcp_connect(sockfd);
    SSL_set_fd(sslResume, sockfd);
    SSL_set_session(sslResume, session);

    if (SSL_connect(sslResume) != SSL_SUCCESS)
        ClientError(ctx, sslResume, sockfd, "SSL_resume failed");
    showPeer(sslResume);

    if (SSL_write(sslResume, msg, sizeof(msg)) != sizeof(msg))
        ClientError(ctx, sslResume, sockfd, "SSL_write failed");

    input = SSL_read(sslResume, reply, sizeof(reply));
    if (input > 0) {
        reply[input] = 0;
        printf("Server response: %s\n", reply);
    }

    SSL_shutdown(sslResume);
    SSL_free(sslResume);
    tcp_close(sockfd);
#endif // TEST_RESUME

    SSL_CTX_free(ctx);
    ((func_args*)args)->return_code = 0;
}


#ifndef NO_MAIN_DRIVER

    int main(int argc, char** argv)
    {
        func_args args;

        args.argc = argc;
        args.argv = argv;

        client_test(&args);
        yaSSL_CleanUp();

        return args.return_code;
    }

#endif // NO_MAIN_DRIVER

