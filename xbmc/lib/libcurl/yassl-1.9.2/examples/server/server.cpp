/* server.cpp */


#include "../../testsuite/test.hpp"


void ServerError(SSL_CTX* ctx, SSL* ssl, SOCKET_T& sockfd, const char* msg)
{
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    tcp_close(sockfd);
    err_sys(msg);
}


#ifdef NON_BLOCKING
    void NonBlockingSSL_Accept(SSL* ssl, SSL_CTX* ctx, SOCKET_T& clientfd)
    {
        int ret = SSL_accept(ssl);
        while (ret != SSL_SUCCESS && (SSL_get_error(ssl, 0) ==
                                     SSL_ERROR_WANT_READ)) {
            printf("... server would block\n");
            #ifdef _WIN32
                Sleep(1000);
            #else
                sleep(1);
            #endif
            ret = SSL_accept(ssl);
        }
        if (ret != SSL_SUCCESS)
            ServerError(ctx, ssl, clientfd, "SSL_accept failed");
    }
#endif


THREAD_RETURN YASSL_API server_test(void* args)
{
#ifdef _WIN32
    WSADATA wsd;
    WSAStartup(0x0002, &wsd);
#endif

    SOCKET_T sockfd   = 0;
    SOCKET_T clientfd = 0;
    int      argc     = 0;
    char**   argv     = 0;

    set_args(argc, argv, *static_cast<func_args*>(args));
    tcp_accept(sockfd, clientfd, *static_cast<func_args*>(args));

    tcp_close(sockfd);

    SSL_METHOD* method = TLSv1_server_method();
    SSL_CTX*    ctx = SSL_CTX_new(method);

    //SSL_CTX_set_cipher_list(ctx, "RC4-SHA:RC4-MD5");
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, 0);
    set_serverCerts(ctx);
    DH* dh = set_tmpDH(ctx);

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, clientfd);

#ifdef NON_BLOCKING
    NonBlockingSSL_Accept(ssl, ctx, clientfd);
#else
    if (SSL_accept(ssl) != SSL_SUCCESS)
        ServerError(ctx, ssl, clientfd, "SSL_accept failed");
#endif
     
    showPeer(ssl);
    printf("Using Cipher Suite: %s\n", SSL_get_cipher(ssl));

    char command[1024];
    int input = SSL_read(ssl, command, sizeof(command));
    if (input > 0) {
        command[input] = 0;
        printf("First client command: %s\n", command);
    }

    char msg[] = "I hear you, fa shizzle!";
    if (SSL_write(ssl, msg, sizeof(msg)) != sizeof(msg))
        ServerError(ctx, ssl, clientfd, "SSL_write failed");

    DH_free(dh);
    SSL_CTX_free(ctx);
    SSL_shutdown(ssl);
    SSL_free(ssl);

    tcp_close(clientfd);

    ((func_args*)args)->return_code = 0;
    return 0;
}


#ifndef NO_MAIN_DRIVER

    int main(int argc, char** argv)
    {
        func_args args;

        args.argc = argc;
        args.argv = argv;

        server_test(&args);
        yaSSL_CleanUp();

        return args.return_code;
    }

#endif // NO_MAIN_DRIVER

