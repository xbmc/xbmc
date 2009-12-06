/* echoserver.cpp */

#include "../../testsuite/test.hpp"


#ifndef NO_MAIN_DRIVER
    #define ECHO_OUT

    THREAD_RETURN YASSL_API echoserver_test(void*);
    int main(int argc, char** argv)
    {
        func_args args;

        args.argc = argc;
        args.argv = argv;

        echoserver_test(&args);
        yaSSL_CleanUp();

        return args.return_code;
    }

#endif // NO_MAIN_DRIVER



void EchoError(SSL_CTX* ctx, SSL* ssl, SOCKET_T& s1, SOCKET_T& s2,
               const char* msg)
{
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    tcp_close(s1);
    tcp_close(s2);
    err_sys(msg);
}


THREAD_RETURN YASSL_API echoserver_test(void* args)
{
#ifdef _WIN32
    WSADATA wsd;
    WSAStartup(0x0002, &wsd);
#endif

    SOCKET_T sockfd = 0;
    int      argc = 0;
    char**   argv = 0;

    set_args(argc, argv, *static_cast<func_args*>(args));

#ifdef ECHO_OUT
    FILE* fout = stdout;
    if (argc >= 2) fout = fopen(argv[1], "w");
    if (!fout) err_sys("can't open output file");
#endif

    tcp_listen(sockfd);

    SSL_METHOD* method = SSLv23_server_method();
    SSL_CTX*    ctx    = SSL_CTX_new(method);

    set_serverCerts(ctx);
    DH* dh = set_tmpDH(ctx);

    bool shutdown(false);

#if defined(_POSIX_THREADS) && defined(NO_MAIN_DRIVER)
    // signal ready to tcp_accept
    func_args& server_args = *((func_args*)args);
    tcp_ready& ready = *server_args.signal_;
    pthread_mutex_lock(&ready.mutex_);
    ready.ready_ = true;
    pthread_cond_signal(&ready.cond_);
    pthread_mutex_unlock(&ready.mutex_);
#endif

    while (!shutdown) {
        SOCKADDR_IN_T client;
        socklen_t   client_len = sizeof(client);
        SOCKET_T    clientfd   = accept(sockfd, (sockaddr*)&client,
                                      (ACCEPT_THIRD_T)&client_len);
        if (clientfd == (SOCKET_T) -1) {
            SSL_CTX_free(ctx);
            tcp_close(sockfd);
            err_sys("tcp accept failed");
        }

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, clientfd);
        if (SSL_accept(ssl) != SSL_SUCCESS) {
            printf("SSL_accept failed\n");
            SSL_free(ssl);
            tcp_close(clientfd);
            continue; 
        }
       
        char command[1024];
        int echoSz(0);
        while ( (echoSz = SSL_read(ssl, command, sizeof(command))) > 0) {

            if ( strncmp(command, "quit", 4) == 0) {
                printf("client sent quit command: shutting down!\n");
                shutdown = true;
                break;
            }
            else if ( strncmp(command, "GET", 3) == 0) {
                char type[]   = "HTTP/1.0 200 ok\r\nContent-type:"
                                " text/html\r\n\r\n";
                char header[] = "<html><body BGCOLOR=\"#ffffff\">\n<pre>\n";
                char body[]   = "greetings from yaSSL\n";
                char footer[] = "</body></html>\r\n\r\n";

                strncpy(command, type, sizeof(type));
                echoSz = sizeof(type) - 1;

                strncpy(&command[echoSz], header, sizeof(header));
                echoSz += sizeof(header) - 1;
                strncpy(&command[echoSz], body, sizeof(body));
                echoSz += sizeof(body) - 1;
                strncpy(&command[echoSz], footer, sizeof(footer));
                echoSz += sizeof(footer);

                if (SSL_write(ssl, command, echoSz) != echoSz)
                    EchoError(ctx, ssl, sockfd, clientfd, "SSL_write failed");
               
                break;
            }
            command[echoSz] = 0;

        #ifdef ECHO_OUT
            fputs(command, fout);
        #endif

            if (SSL_write(ssl, command, echoSz) != echoSz)
                EchoError(ctx, ssl, sockfd, clientfd, "SSL_write failed");
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        tcp_close(clientfd);
    }

    tcp_close(sockfd);

    DH_free(dh);
    SSL_CTX_free(ctx);

    ((func_args*)args)->return_code = 0;
    return 0;
}
