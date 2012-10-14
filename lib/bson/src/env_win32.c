/* env_win32.c */

/*    Copyright 2009-2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/* Networking and other niceties for WIN32. */
#include "env.h"
#include "mongo.h"
#include <string.h>

#ifdef _MSC_VER
#include <ws2tcpip.h>  /* send,recv,socklen_t etc */
#include <wspiapi.h>   /* addrinfo */
#else
#include <ws2tcpip.h>  /* send,recv,socklen_t etc */
#include <winsock2.h>
typedef int socklen_t;
#endif

#ifndef NI_MAXSERV
# define NI_MAXSERV 32
#endif

int mongo_env_close_socket( size_t socket ) {
    return closesocket( socket );
}

int mongo_env_write_socket( mongo *conn, const void *buf, int len ) {
    const char *cbuf = (const char*) buf;
    int flags = 0;

    while ( len ) {
        int sent = send( conn->sock, cbuf, len, flags );
        if ( sent == -1 ) {
            __mongo_set_error( conn, MONGO_IO_ERROR, NULL, WSAGetLastError() );
            conn->connected = 0;
            return MONGO_ERROR;
        }
        cbuf += sent;
        len -= sent;
    }

    return MONGO_OK;
}

int mongo_env_read_socket( mongo *conn, void *buf, int len ) {
    char *cbuf = (char*)buf;

    while ( len ) {
        int sent = recv( conn->sock, cbuf, len, 0 );
        if ( sent == 0 || sent == -1 ) {
            __mongo_set_error( conn, MONGO_IO_ERROR, NULL, WSAGetLastError() );
            return MONGO_ERROR;
        }
        cbuf += sent;
        len -= sent;
    }

    return MONGO_OK;
}

int mongo_env_set_socket_op_timeout( mongo *conn, int millis ) {
    if ( setsockopt( conn->sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&millis,
                     sizeof( millis ) ) == -1 ) {
        __mongo_set_error( conn, MONGO_IO_ERROR, "setsockopt SO_RCVTIMEO failed.",
                           WSAGetLastError() );
        return MONGO_ERROR;
    }

    if ( setsockopt( conn->sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&millis,
                     sizeof( millis ) ) == -1 ) {
        __mongo_set_error( conn, MONGO_IO_ERROR, "setsockopt SO_SNDTIMEO failed.",
                           WSAGetLastError() );
        return MONGO_ERROR;
    }

    return MONGO_OK;
}

int mongo_env_socket_connect( mongo *conn, const char *host, int port ) {
    char port_str[NI_MAXSERV];
    char errstr[MONGO_ERR_LEN];
    int status;

    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    struct addrinfo *ai_ptr = NULL;

    conn->sock = 0;
    conn->connected = 0;

    bson_sprintf( port_str, "%d", port );

    memset( &ai_hints, 0, sizeof( ai_hints ) );
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo( host, port_str, &ai_hints, &ai_list );
    if ( status != 0 ) {
        bson_sprintf( errstr, "getaddrinfo failed with error %d", status );
        __mongo_set_error( conn, MONGO_CONN_ADDR_FAIL, errstr, WSAGetLastError() );
        return MONGO_ERROR;
    }

    for ( ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next ) {
        conn->sock = socket( ai_ptr->ai_family, ai_ptr->ai_socktype,
                             ai_ptr->ai_protocol );

        if ( conn->sock < 0 ) {
            __mongo_set_error( conn, MONGO_SOCKET_ERROR, "socket() failed",
                               WSAGetLastError() );
            conn->sock = 0;
            continue;
        }

        status = connect( conn->sock, ai_ptr->ai_addr, (int)ai_ptr->ai_addrlen );
        if ( status != 0 ) {
            __mongo_set_error( conn, MONGO_SOCKET_ERROR, "connect() failed",
                           WSAGetLastError() );
            mongo_env_close_socket( conn->sock );
            conn->sock = 0;
            continue;
        }

        if ( ai_ptr->ai_protocol == IPPROTO_TCP ) {
            int flag = 1;

            setsockopt( conn->sock, IPPROTO_TCP, TCP_NODELAY,
                        ( char * ) &flag, sizeof( flag ) );

            if ( conn->op_timeout_ms > 0 )
                mongo_env_set_socket_op_timeout( conn, conn->op_timeout_ms );
        }

        conn->connected = 1;
        break;
    }

    freeaddrinfo( ai_list );

    if ( ! conn->connected ) {
        conn->err = MONGO_CONN_FAIL;
        return MONGO_ERROR;
    } 
    else {
        mongo_clear_errors( conn );
        return MONGO_OK;
    }
}

MONGO_EXPORT int mongo_env_sock_init( void ) {

    WSADATA wsaData;
    WORD wVers;
    static int called_once;
    static int retval;

    if (called_once) return retval;

    called_once = 1;
    wVers = MAKEWORD(1, 1);
    retval = (WSAStartup(wVers, &wsaData) == 0);

    return retval;
}
