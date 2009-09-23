/* echoclient.cpp  */

#include "openssl/ssl.h"  /* openssl compatibility test */
#include <stdio.h>
#include <stdlib.h>


#ifdef WIN32
    #include <winsock2.h>
#else
    #include <string.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/socket.h>
#endif /* WIN32 */


void err_sys(const char* msg)
{
    fputs("yassl client error: ", stderr);
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}

const char* loopback  = "127.0.0.1";
const short yasslPort = 11111;

using namespace yaSSL;


int main(int argc, char** argv)
{
#ifdef WIN32
    WSADATA wsd;
    WSAStartup(0x0002, &wsd);
    int sockfd;
#else
    unsigned int sockfd;
#endif /* WIN32  */

    FILE* fin  = stdin;
    FILE* fout = stdout;

    if (argc >= 2) fin  = fopen(argv[1], "r");
    if (argc >= 3) fout = fopen(argv[2], "w");

    if (!fin)  err_sys("can't open input file");
    if (!fout) err_sys("can't open output file");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    servaddr.sin_port = htons(yasslPort);
    servaddr.sin_addr.s_addr = inet_addr(loopback);
    if (connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr)) != 0)
        err_sys("tcp connect failed");

    SSL_METHOD* method = TLSv1_client_method();
    SSL_CTX*    ctx = SSL_CTX_new(method);
    SSL*        ssl = SSL_new(ctx);

    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) != SSL_SUCCESS) err_sys("SSL_connect failed");

    char send[1024];
    char reply[1024];
 
    while (fgets(send, sizeof(send), fin)) {

        int sendSz = strlen(send) + 1;
        if (SSL_write(ssl, send, sendSz) != sendSz)
            err_sys("SSL_write failed");

        if (strncmp(send, "quit", 4) == 0) {
            fputs("sending server shutdown command: quit!", fout);
            break;
        }

        if (SSL_read(ssl, reply, sizeof(reply)) > 0) 
            fputs(reply, fout);
    }

    SSL_CTX_free(ctx);
    SSL_free(ssl);

    return 0;
}
