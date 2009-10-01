/*
 *   stunnel       Universal SSL tunnel
 *   Copyright (c) 1998-2004 Michal Trojnara <Michal.Trojnara@mirt.net>
 *                 All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   In addition, as a special exception, Michal Trojnara gives
 *   permission to link the code of this program with the OpenSSL
 *   library (or with modified versions of OpenSSL that use the same
 *   license as OpenSSL), and distribute linked combinations including
 *   the two.  You must obey the GNU General Public License in all
 *   respects for all of the code used other than OpenSSL.  If you modify
 *   this file, you may extend this exception to your version of the
 *   file, but you are not obligated to do so.  If you do not wish to
 *   do so, delete this exception statement from your version.
 */

/* Undefine if you have problems with make_sockets() */
#define INET_SOCKET_PAIR

#include "common.h"
#include "prototypes.h"

#ifndef SHUT_RD
#define SHUT_RD 0
#endif
#ifndef SHUT_WR
#define SHUT_WR 1
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

/* TCP wrapper */
#ifdef USE_LIBWRAP
#include <tcpd.h>
int allow_severity=LOG_NOTICE;
int deny_severity=LOG_WARNING;
#endif

#if SSLEAY_VERSION_NUMBER >= 0x0922
static unsigned char *sid_ctx=(unsigned char *)"stunnel SID";
    /* const allowed here */
#endif

extern SSL_CTX *ctx; /* global SSL context defined in ssl.c */

static int do_client(CLI *);
static int init_local(CLI *);
static int init_remote(CLI *);
static int init_ssl(CLI *);
static int transfer(CLI *);
static void cleanup(CLI *, int);

static void print_cipher(CLI *);
static int auth_libwrap(CLI *);
static int auth_user(CLI *);
static int connect_local(CLI *c);
#ifndef USE_WIN32
static int make_sockets(int [2]);
#endif
static int connect_remote(CLI *c);
static void reset(int, char *);

int max_clients;
#ifndef USE_WIN32
int max_fds;
#endif

/* Allocate local data structure for the new thread */
void *alloc_client_session(LOCAL_OPTIONS *opt, int rfd, int wfd) {
    CLI *c;

    c=calloc(1, sizeof(CLI));
    if(!c) {
        log(LOG_ERR, "Memory allocation failed");
        return NULL;
    }
    c->opt=opt;
    c->local_rfd.fd=rfd;
    c->local_wfd.fd=wfd;
    return c;
}

void *client(void *arg) {
    CLI *c=arg;

#ifdef DEBUG_STACK_SIZE
    stack_info(1); /* initialize */
#endif
    log(LOG_DEBUG, "%s started", c->opt->servname);
#ifndef USE_WIN32
    if(c->opt->option.remote && c->opt->option.program)
        c->local_rfd.fd=c->local_wfd.fd=connect_local(c);
            /* connect and exec options specified together */
            /* spawn local program instead of stdio */
#endif
    c->remote_fd.fd=-1;
    c->ssl=NULL;
    cleanup(c, do_client(c));
#ifdef USE_FORK
    if(!c->opt->option.remote) /* 'exec' specified */
        exec_status(); /* null SIGCHLD handler was used */
#else
    enter_critical_section(CRIT_CLIENTS); /* for multi-cpu machines */
    log(LOG_DEBUG, "%s finished (%d left)", c->opt->servname,
        --num_clients);
    leave_critical_section(CRIT_CLIENTS);
#endif
    free(c);
#ifdef DEBUG_STACK_SIZE
    stack_info(0); /* display computed value */
#endif
    return NULL;
}

static int do_client(CLI *c) {
    int result;

    if(init_local(c))
        return -1;
    if(!options.option.client && !c->opt->protocol) {
        /* Server mode and no protocol negotiation needed */
        if(init_ssl(c))
            return -1;
        if(init_remote(c))
            return -1;
    } else {
        if(init_remote(c))
            return -1;
        if(negotiate(c)<0) {
            log(LOG_ERR, "Protocol negotiations failed");
            return -1;
        }
        if(init_ssl(c))
            return -1;
    }
    result=transfer(c);
    log(LOG_NOTICE,
        "Connection %s: %d bytes sent to SSL, %d bytes sent to socket",
         result ? "reset" : "closed", c->ssl_bytes, c->sock_bytes);
    return result;
}

static int init_local(CLI *c) {
    int addrlen;

    addrlen=sizeof(c->addr);

    if(getpeername(c->local_rfd.fd, (struct sockaddr *)&c->addr, &addrlen)<0) {
        strcpy(c->accepting_address, "NOT A SOCKET");
        c->local_rfd.is_socket=0;
        c->local_wfd.is_socket=0; /* TODO: It's not always true */
#ifdef USE_WIN32
        if(get_last_socket_error()!=ENOTSOCK) {
#else
        if(c->opt->option.transparent || get_last_socket_error()!=ENOTSOCK) {
#endif
            sockerror("getpeerbyname");
            return -1;
        }
        /* Ignore ENOTSOCK error so 'local' doesn't have to be a socket */
    } else {
        safe_ntoa(c->accepting_address, c->addr.sin_addr);
        c->local_rfd.is_socket=1;
        c->local_wfd.is_socket=1; /* TODO: It's not always true */
        /* It's a socket: lets setup options */
        if(set_socket_options(c->local_rfd.fd, 1)<0)
            return -1;
        if(auth_libwrap(c)<0)
            return -1;
        if(auth_user(c)<0) {
            log(LOG_WARNING, "Connection from %s:%d REFUSED by IDENT",
                c->accepting_address, ntohs(c->addr.sin_port));
            return -1;
        }
        log(LOG_NOTICE, "%s connected from %s:%d", c->opt->servname,
            c->accepting_address, ntohs(c->addr.sin_port));
    }
    return 0; /* OK */
}

static int init_remote(CLI *c) {
    int fd;

    /* create connection to host/service */
    if(c->opt->local_ip)
        c->bind_ip=*c->opt->local_ip;
#ifndef USE_WIN32
    else if(c->opt->option.transparent)
        c->bind_ip=c->addr.sin_addr.s_addr;
#endif
    else
        c->bind_ip=0;
    /* Setup c->remote_fd, now */
    if(c->opt->option.remote) {
        c->resolved_addresses=NULL;
        fd=connect_remote(c);
        if(c->resolved_addresses) /* allocated */
            free(c->resolved_addresses);
    } else /* NOT in remote mode */
        fd=connect_local(c);
    if(fd<0) {
        log(LOG_ERR, "Failed to initialize remote connection");
        return -1;
    }
#ifndef USE_WIN32
    if(fd>=max_fds) {
        log(LOG_ERR, "Remote file descriptor out of range (%d>=%d)",
            fd, max_fds);
        closesocket(fd);
        return -1;
    }
#endif
    log(LOG_DEBUG, "Remote FD=%d initialized", fd);
    c->remote_fd.fd=fd;
    c->remote_fd.is_socket=1; /* Always! */
    if(set_socket_options(fd, 2)<0)
        return -1;
    return 0; /* OK */
}

static int init_ssl(CLI *c) {
    int i, err;

    if(!(c->ssl=SSL_new(ctx))) {
        sslerror("SSL_new");
        return -1;
    }
#if SSLEAY_VERSION_NUMBER >= 0x0922
    SSL_set_session_id_context(c->ssl, sid_ctx, strlen(sid_ctx));
#endif
    if(options.option.client) {
        /* Attempt to use the most recent id in the session cache */
#ifndef HAVE_YASSL /* yassl add, ctx members available */
        if(ctx->session_cache_head)
            if(!SSL_set_session(c->ssl, ctx->session_cache_head))
                log(LOG_WARNING, "Cannot set SSL session id to most recent used");
#endif /* yassl end add */
        SSL_set_fd(c->ssl, c->remote_fd.fd);
        SSL_set_connect_state(c->ssl);
    } else {
        if(c->local_rfd.fd==c->local_wfd.fd)
            SSL_set_fd(c->ssl, c->local_rfd.fd);
        else {
           /* Does it make sence to have SSL on STDIN/STDOUT? */
            SSL_set_rfd(c->ssl, c->local_rfd.fd);
            SSL_set_wfd(c->ssl, c->local_wfd.fd);
        }
        SSL_set_accept_state(c->ssl);
    }

    /* Setup some values for transfer() function */
    if(options.option.client) {
        c->sock_rfd=&(c->local_rfd);
        c->sock_wfd=&(c->local_wfd);
        c->ssl_rfd=c->ssl_wfd=&(c->remote_fd);
    } else {
        c->sock_rfd=c->sock_wfd=&(c->remote_fd);
        c->ssl_rfd=&(c->local_rfd);
        c->ssl_wfd=&(c->local_wfd);
    }

    while(1) {
        if(options.option.client)
            i=SSL_connect(c->ssl);
        else
            i=SSL_accept(c->ssl);
        err=SSL_get_error(c->ssl, i);
        if(err==SSL_ERROR_NONE)
            break; /* ok -> done */
        if(err==SSL_ERROR_WANT_READ) {
            if(waitforsocket(c->ssl_rfd->fd, 0, c->opt->timeout_busy)==1)
                continue; /* ok -> retry */
            return -1; /* timeout or error */
        }
        if(err==SSL_ERROR_WANT_WRITE) {
            if(waitforsocket(c->ssl_wfd->fd, 1, c->opt->timeout_busy)==1)
                continue; /* ok -> retry */
            return -1; /* timeout or error */
        }
        if(err==SSL_ERROR_SYSCALL) {
            switch(get_last_socket_error()) {
            case EINTR:
            case EAGAIN:
                continue; 
            }
        }
        if(options.option.client)
            sslerror("SSL_connect");
        else
            sslerror("SSL_accept");
        return -1;
    }
    print_cipher(c);
    return 0; /* OK */
}

static int transfer(CLI *c) { /* transfer data */
    fd_set rd_set, wr_set;
    int num, err, fdno;
    int check_SSL_pending;
    int ssl_closing;
        /* 0=not closing SSL, 1=initiate SSL_shutdown,
         * 2=retry SSL_shutdown, 3=SSL_shutdown done */
    int ready;
    struct timeval tv;

    /* fdno=max(c->sock_rfd->fd, c->sock_wfd->fd,
     *     fdno=c->ssl_rfd->fd, fdno=c->ssl_wfd->fd)+1 */
    fdno=c->sock_rfd->fd;
    if(c->sock_wfd->fd>fdno) fdno=c->sock_wfd->fd;
    if(c->ssl_rfd->fd>fdno) fdno=c->ssl_rfd->fd;
    if(c->ssl_wfd->fd>fdno) fdno=c->ssl_wfd->fd;
    fdno+=1;

    c->sock_ptr=c->ssl_ptr=0;
    sock_rd=sock_wr=ssl_rd=ssl_wr=1;
    c->sock_bytes=c->ssl_bytes=0;
    ssl_closing=0;

    while(((sock_rd||c->sock_ptr)&&ssl_wr)||((ssl_rd||c->ssl_ptr)&&sock_wr)) {

        FD_ZERO(&rd_set); /* Setup rd_set */
        if(sock_rd && c->sock_ptr<BUFFSIZE) /* socket input buffer not full*/
            FD_SET(c->sock_rfd->fd, &rd_set);
        if(ssl_rd && (
                c->ssl_ptr<BUFFSIZE || /* SSL input buffer not full */
                ((c->sock_ptr||ssl_closing) && SSL_want_read(c->ssl))
                /* I want to SSL_write or SSL_shutdown but read from the
                 * underlying socket needed for the SSL protocol */
                )) {
            FD_SET(c->ssl_rfd->fd, &rd_set);
        }

        FD_ZERO(&wr_set); /* Setup wr_set */
        if(sock_wr && c->ssl_ptr) /* SSL input buffer not empty */
            FD_SET(c->sock_wfd->fd, &wr_set);
        if (ssl_wr && (
                c->sock_ptr || /* socket input buffer not empty */
                ssl_closing==1 || /* initiate SSL_shutdown */
                ((c->ssl_ptr<BUFFSIZE || ssl_closing==2) &&
                    SSL_want_write(c->ssl))
                /* I want to SSL_read or SSL_shutdown but write to the
                 * underlying socket needed for the SSL protocol */
                )) {
            FD_SET(c->ssl_wfd->fd, &wr_set);
        }

        tv.tv_sec=sock_rd ||
            (ssl_wr&&c->sock_ptr) || (sock_wr&&c->ssl_ptr) ?
            c->opt->timeout_idle : c->opt->timeout_close;
        tv.tv_usec=0;
        ready=sselect(fdno, &rd_set, &wr_set, NULL, &tv);
        if(ready<0) { /* Break the connection for others */
            sockerror("select");
            return -1;
        }
        if(!ready) { /* Timeout */
            if(sock_rd) { /* No traffic for a long time */
                log(LOG_DEBUG, "select timeout: connection reset");
                return -1;
            } else { /* Timeout waiting for SSL close_notify */
                log(LOG_DEBUG, "select timeout waiting for SSL close_notify");
                break; /* Leave the while() loop */
            }
        }

        if(ssl_closing==1 /* initiate SSL_shutdown */ || (ssl_closing==2 && (
                (SSL_want_read(c->ssl) && FD_ISSET(c->ssl_rfd->fd, &rd_set)) ||
                (SSL_want_write(c->ssl) && FD_ISSET(c->ssl_wfd->fd, &wr_set))
                ))) {
            switch(SSL_shutdown(c->ssl)) { /* Send close_notify */
            case 1: /* the shutdown was successfully completed */
                log(LOG_INFO, "SSL_shutdown successfully sent close_notify");
                ssl_wr=0; /* SSL write closed */
                /* TODO: It's not really closed.  We need to distinguish
                 * closed SSL and closed underlying file descriptor */
                ssl_closing=3; /* done! */
                break;
            case 0: /* the shutdown is not yet finished */
                log(LOG_DEBUG, "SSL_shutdown retrying");
                ssl_closing=2; /* next time just retry SSL_shutdown */
                break;
            case -1: /* a fatal error occurred */
                sslerror("SSL_shutdown");
                return -1;
            }
        }

        /* Set flag to try and read any buffered SSL data if we made */
        /* room in the buffer by writing to the socket */
        check_SSL_pending = 0;

        if(sock_wr && FD_ISSET(c->sock_wfd->fd, &wr_set)) {
            switch(num=writesocket(c->sock_wfd->fd, c->ssl_buff, c->ssl_ptr)) {
            case -1: /* error */
                switch(get_last_socket_error()) {
                case EINTR:
                    log(LOG_DEBUG,
                        "writesocket interrupted by a signal: retrying");
                    break;
                case EWOULDBLOCK:
                    log(LOG_NOTICE, "writesocket would block: retrying");
                    break;
                default:
                    sockerror("writesocket");
                    return -1;
                }
                break;
            case 0:
                log(LOG_DEBUG, "No data written to the socket: retrying");
                break;
            default:
                memmove(c->ssl_buff, c->ssl_buff+num, c->ssl_ptr-num);
                if(c->ssl_ptr==BUFFSIZE)
                    check_SSL_pending=1;
                c->ssl_ptr-=num;
                c->sock_bytes+=num;
                if(!ssl_rd && !c->ssl_ptr) {
                    shutdown(c->sock_wfd->fd, SHUT_WR);
                    log(LOG_DEBUG,
                        "Socket write shutdown (no more data to send)");
                    sock_wr=0;
                }
            }
        }

        if(ssl_wr && ( /* SSL sockets are still open */
                (c->sock_ptr && FD_ISSET(c->ssl_wfd->fd, &wr_set)) ||
                /* See if application data can be written */
                (SSL_want_read(c->ssl) && FD_ISSET(c->ssl_rfd->fd, &rd_set))
                /* I want to SSL_write but read from the underlying */
                /* socket needed for the SSL protocol */
                )) {
            num=SSL_write(c->ssl, c->sock_buff, c->sock_ptr);

            err=SSL_get_error(c->ssl, num);
            switch(err) {
            case SSL_ERROR_NONE:
                memmove(c->sock_buff, c->sock_buff+num, c->sock_ptr-num);
                c->sock_ptr-=num;
                c->ssl_bytes+=num;
                if(!ssl_closing && !sock_rd && !c->sock_ptr && ssl_wr) {
                    log(LOG_DEBUG,
                        "SSL write shutdown (no more data to send)");
                    ssl_closing=1;
                }
                break;
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_X509_LOOKUP:
                log(LOG_DEBUG, "SSL_write returned WANT_: retrying");
                break;
            case SSL_ERROR_SYSCALL:
                if(num<0) { /* really an error */
                    switch(get_last_socket_error()) {
                    case EINTR:
                        log(LOG_DEBUG,
                            "SSL_write interrupted by a signal: retrying");
                        break;
                    case EAGAIN:
                        log(LOG_DEBUG,
                            "SSL_write returned EAGAIN: retrying");
                        break; 
                    default:
                        sockerror("SSL_write (ERROR_SYSCALL)");
                        return -1;
                    }
                }
                break;
            case SSL_ERROR_ZERO_RETURN: /* close_notify received */
                log(LOG_DEBUG, "SSL closed on SSL_write");
                ssl_rd=ssl_wr=0;
                break;
            case SSL_ERROR_SSL:
                sslerror("SSL_write");
                return -1;
            default:
                log(LOG_ERR, "SSL_write/SSL_get_error returned %d", err);
                return -1;
            }
        }

        if(sock_rd && FD_ISSET(c->sock_rfd->fd, &rd_set)) {
            switch(num=readsocket(c->sock_rfd->fd,
                c->sock_buff+c->sock_ptr, BUFFSIZE-c->sock_ptr)) {
            case -1:
                switch(get_last_socket_error()) {
                case EINTR:
                    log(LOG_DEBUG,
                        "readsocket interrupted by a signal: retrying");
                    break;
                case EWOULDBLOCK:
                    log(LOG_NOTICE, "readsocket would block: retrying");
                    break;
                default:
                    sockerror("readsocket");
                    return -1;
                }
                break;
            case 0: /* close */
                log(LOG_DEBUG, "Socket closed on read");
                sock_rd=0;
                if(!ssl_closing && !c->sock_ptr && ssl_wr) {
                    log(LOG_DEBUG,
                        "SSL write shutdown (output buffer empty)");
                    ssl_closing=1;
                }
                break;
            default:
                c->sock_ptr+=num;
            }
        }

        if(ssl_rd && ( /* SSL sockets are still open */
                (c->ssl_ptr<BUFFSIZE && FD_ISSET(c->ssl_rfd->fd, &rd_set)) ||
                /* See if there's any application data coming in */
                (SSL_want_write(c->ssl) && FD_ISSET(c->ssl_wfd->fd, &wr_set)) ||
                /* I want to SSL_read but write to the underlying */
                /* socket needed for the SSL protocol */
                (check_SSL_pending && SSL_pending(c->ssl))
                /* Write made space from full buffer */
                )) {
            num=SSL_read(c->ssl, c->ssl_buff+c->ssl_ptr, BUFFSIZE-c->ssl_ptr);

            err=SSL_get_error(c->ssl, num);
            switch(err) {
            case SSL_ERROR_NONE:
                c->ssl_ptr+=num;
                break;
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_X509_LOOKUP:
                log(LOG_DEBUG, "SSL_read returned WANT_: retrying");
                break;
            case SSL_ERROR_SYSCALL:
                if(num<0) { /* not EOF */
                    switch(get_last_socket_error()) {
                    case EINTR:
                        log(LOG_DEBUG,
                            "SSL_read interrupted by a signal: retrying");
                        break;
                    case EAGAIN:
                        log(LOG_DEBUG,
                            "SSL_read returned EAGAIN: retrying");
                        break; 
                    default:
                        sockerror("SSL_read (ERROR_SYSCALL)");
                        return -1;
                    }
                } else { /* EOF */
                    log(LOG_DEBUG, "SSL socket closed on SSL_read");
                    ssl_rd=ssl_wr=0;
                }
                break;
            case SSL_ERROR_ZERO_RETURN: /* close_notify received */
                log(LOG_DEBUG, "SSL closed on SSL_read");
                ssl_rd=0;
                if(!ssl_closing && !c->sock_ptr && ssl_wr) {
                    log(LOG_DEBUG,
                        "SSL write shutdown (output buffer empty)");
                    ssl_closing=1;
                }
                if(!c->ssl_ptr && sock_wr) {
                    shutdown(c->sock_wfd->fd, SHUT_WR);
                    log(LOG_DEBUG,
                        "Socket write shutdown (output buffer empty)");
                    sock_wr=0;
                }
                break;
            case SSL_ERROR_SSL:
                sslerror("SSL_read");
                return -1;
            default:
                log(LOG_ERR, "SSL_read/SSL_get_error returned %d", err);
                return -1;
            }
        }
    }
    return 0; /* OK */
}

static void cleanup(CLI *c, int error) {
        /* Cleanup SSL */
    if(c->ssl) { /* SSL initialized */
        SSL_set_shutdown(c->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
        SSL_free(c->ssl);
        ERR_remove_state(0);
    }
        /* Cleanup remote socket */
    if(c->remote_fd.fd>=0) { /* Remote socket initialized */
        if(error && c->remote_fd.is_socket)
            reset(c->remote_fd.fd, "linger (remote)");
        closesocket(c->remote_fd.fd);
    }
        /* Cleanup local socket */
    if(c->local_rfd.fd>=0) { /* Local socket initialized */
        if(c->local_rfd.fd==c->local_wfd.fd) {
            if(error && c->local_rfd.is_socket)
                reset(c->local_rfd.fd, "linger (local)");
            closesocket(c->local_rfd.fd);
        } else { /* STDIO */
            if(error && c->local_rfd.is_socket)
                reset(c->local_rfd.fd, "linger (local_rfd)");
            if(error && c->local_wfd.is_socket)
                reset(c->local_wfd.fd, "linger (local_wfd)");
       }
    }
}

static void print_cipher(CLI *c) { /* print negotiated cipher */
#if SSLEAY_VERSION_NUMBER <= 0x0800
    log(LOG_INFO, "%s opened with SSLv%d, cipher %s",
        c->opt->servname, ssl->session->ssl_version, SSL_get_cipher(c->ssl));
#else
    SSL_CIPHER *cipher;
    char buf[STRLEN];
    int len;

    cipher=SSL_get_current_cipher(c->ssl);
    SSL_CIPHER_description(cipher, buf, STRLEN);
    len=strlen(buf);
    if(len>0)
        buf[len-1]='\0';
    log(LOG_INFO, "Negotiated ciphers: %s", buf);
#endif
}

static int auth_libwrap(CLI *c) {
#ifdef USE_LIBWRAP
    struct request_info request;
    int result;

    enter_critical_section(CRIT_NTOA); /* libwrap is not mt-safe */
    request_init(&request,
        RQ_DAEMON, c->opt->servname, RQ_FILE, c->local_rfd.fd, 0);
    fromhost(&request);
    result=hosts_access(&request);
    leave_critical_section(CRIT_NTOA);

    if (!result) {
        log(LOG_WARNING, "Connection from %s:%d REFUSED by libwrap",
            c->accepting_address, ntohs(c->addr.sin_port));
        log(LOG_DEBUG, "See hosts_access(5) for details");
        return -1; /* FAILED */
    }
#endif
    return 0; /* OK */
}

static int auth_user(CLI *c) {
    struct servent *s_ent;    /* structure for getservbyname */
    struct sockaddr_in ident; /* IDENT socket name */
    int fd;                   /* IDENT socket descriptor */
    char name[STRLEN];
    int retval;

    if(!c->opt->username)
        return 0; /* -u option not specified */
    if((fd=socket(AF_INET, SOCK_STREAM, 0))<0) {
        sockerror("socket (auth_user)");
        return -1;
    }
    alloc_fd(fd);
    memcpy(&ident, &c->addr, sizeof(ident));
    s_ent=getservbyname("auth", "tcp");
    if(!s_ent) {
        log(LOG_WARNING, "Unknown service 'auth': using default 113");
        ident.sin_port=htons(113);
    } else {
        ident.sin_port=s_ent->s_port;
    }
    if(connect(fd, (struct sockaddr *)&ident, sizeof(ident))<0) {
        switch(get_last_socket_error()) {
        case EINPROGRESS: /* retry */
            log(LOG_DEBUG, "connect #1 (auth_user): EINPROGRESS: retrying");
            break;
        case EWOULDBLOCK: /* retry */
            log(LOG_DEBUG, "connect #1 (auth_user): EWOULDBLOCK: retrying");
            break;
        default:
            sockerror("connect #1 (auth_user)");
            closesocket(fd);
            return -1;
        }
        if(waitforsocket(fd, 1 /* write */, c->opt->timeout_busy)<1) {
            closesocket(fd);
            return -1;
        }
        if(connect(fd, (struct sockaddr *)&ident, sizeof(ident))<0) {
            switch(get_last_socket_error()) {
            case EINVAL: /* WIN32 is strange... */
                log(LOG_DEBUG, "connect #2 (auth_user): EINVAL: ok");
            case EISCONN: /* ok */
                break; /* success */
            default:
                sockerror("connect #2 (auth_user))");
                closesocket(fd);
                return -1;
            }
        }
    }
    log(LOG_DEBUG, "IDENT server connected");
    if(fdprintf(c, fd, "%u , %u",
            ntohs(c->addr.sin_port), ntohs(c->opt->localport))<0) {
        sockerror("fdprintf (auth_user)");
        closesocket(fd);
        return -1;
    }
    if(fdscanf(c, fd, "%*[^:]: USERID :%*[^:]:%s", name)!=1) {
        log(LOG_ERR, "Incorrect data from IDENT server");
        closesocket(fd);
        return -1;
    }
    closesocket(fd);
    retval=strcmp(name, c->opt->username) ? -1 : 0;
    safestring(name);
    log(LOG_INFO, "IDENT resolved remote user to %s", name);
    return retval;
}

static int connect_local(CLI *c) { /* spawn local process */
#if defined (USE_WIN32) || defined (__vms)
    log(LOG_ERR, "LOCAL MODE NOT SUPPORTED ON WIN32 and OpenVMS PLATFORM");
    return -1;
#else /* USE_WIN32, __vms */
    char env[3][STRLEN], name[STRLEN];
    int fd[2], pid;
    X509 *peer;
#ifdef HAVE_PTHREAD_SIGMASK
    sigset_t newmask;
#endif
    if (c->opt->option.pty) {
        char tty[STRLEN];

        if(pty_allocate(fd, fd+1, tty, STRLEN)) {
            return -1;
        }
        log(LOG_DEBUG, "%s allocated", tty);
    } else {
        if(make_sockets(fd))
            return -1;
    }
    pid=fork();
    c->pid=(unsigned long)pid;
    switch(pid) {
    case -1:    /* error */
        closesocket(fd[0]);
        closesocket(fd[1]);
        ioerror("fork");
        return -1;
    case  0:    /* child */
        closesocket(fd[0]);
        dup2(fd[1], 0);
        dup2(fd[1], 1);
        if(!options.option.foreground)
            dup2(fd[1], 2);
        closesocket(fd[1]);
        safecopy(env[0], "REMOTE_HOST=");
        safeconcat(env[0], c->accepting_address);
        putenv(env[0]);
        if(c->opt->option.transparent) {
            putenv("LD_PRELOAD=" LIBDIR "/libstunnel.so");
            /* For Tru64 _RLD_LIST is used instead */
            putenv("_RLD_LIST=" LIBDIR "/libstunnel.so:DEFAULT");
        }
        if(c->ssl) {
            peer=SSL_get_peer_certificate(c->ssl);
            if(peer) {
                safecopy(env[1], "SSL_CLIENT_DN=");
                X509_NAME_oneline(X509_get_subject_name(peer), name, STRLEN);
                safestring(name);
                safeconcat(env[1], name);
                putenv(env[1]);
                safecopy(env[2], "SSL_CLIENT_I_DN=");
                X509_NAME_oneline(X509_get_issuer_name(peer), name, STRLEN);
                safestring(name);
                safeconcat(env[2], name);
                putenv(env[2]);
                X509_free(peer);
            }
        }
#ifdef HAVE_PTHREAD_SIGMASK
        sigemptyset(&newmask);
        sigprocmask(SIG_SETMASK, &newmask, NULL);
#endif
        execvp(c->opt->execname, c->opt->execargs);
        ioerror(c->opt->execname); /* execv failed */
        _exit(1);
    default:
        break;
    }
    /* parent */
    log(LOG_INFO, "Local mode child started (PID=%lu)", c->pid);
    closesocket(fd[1]);
#ifdef FD_CLOEXEC
    fcntl(fd[0], F_SETFD, FD_CLOEXEC);
#endif
    return fd[0];
#endif /* USE_WIN32,__vms */
}

#ifndef USE_WIN32

static int make_sockets(int fd[2]) { /* make a pair of connected sockets */
#ifdef INET_SOCKET_PAIR
    struct sockaddr_in addr;
    int addrlen;
    int s; /* temporary socket awaiting for connection */

    if((s=socket(AF_INET, SOCK_STREAM, 0))<0) {
        sockerror("socket#1");
        return -1;
    }
    if((fd[1]=socket(AF_INET, SOCK_STREAM, 0))<0) {
        sockerror("socket#2");
        return -1;
    }
    addrlen=sizeof(addr);
    memset(&addr, 0, addrlen);
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    addr.sin_port=0; /* dynamic port allocation */
    if(bind(s, (struct sockaddr *)&addr, addrlen))
        log_error(LOG_DEBUG, get_last_socket_error(), "bind#1");
    if(bind(fd[1], (struct sockaddr *)&addr, addrlen))
        log_error(LOG_DEBUG, get_last_socket_error(), "bind#2");
    if(listen(s, 5)) {
        sockerror("listen");
        return -1;
    }
    if(getsockname(s, (struct sockaddr *)&addr, &addrlen)) {
        sockerror("getsockname");
        return -1;
    }
    if(connect(fd[1], (struct sockaddr *)&addr, addrlen)) {
        sockerror("connect");
        return -1;
    }
    if((fd[0]=accept(s, (struct sockaddr *)&addr, &addrlen))<0) {
        sockerror("accept");
        return -1;
    }
    closesocket(s); /* don't care about the result */
#else
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, fd)) {
        sockerror("socketpair");
        return -1;
    }
#endif
    return 0;
}
#endif

static int connect_remote(CLI *c) { /* connect to remote host */
    struct sockaddr_in addr;
    u32 *list;
    int error;
    int s; /* destination socket */
    u16 dport;
   
    memset(&addr, 0, sizeof(addr));
    addr.sin_family=AF_INET;

    if(c->opt->option.delayed_lookup) {
        if(name2nums(c->opt->remote_address, "127.0.0.1",
                &c->resolved_addresses, &dport)==0) {
            /* No host resolved */
            return -1;
        }
        list=c->resolved_addresses;
    } else { /* Use pre-resolved addresses */
        list=c->opt->remotenames;
        dport=c->opt->remoteport;
    }

    /* connect each host from the list */
    for(; *list+1; list++) { /* same as (signed)*list!=-1 */
        if((s=socket(AF_INET, SOCK_STREAM, 0))<0) {
            sockerror("remote socket");
            return -1;
        }
        if(alloc_fd(s))
            return -1;

        if(c->bind_ip) { /* explicit local bind or transparent proxy */
            addr.sin_addr.s_addr=c->bind_ip;
            addr.sin_port=htons(0);
            if(bind(s, (struct sockaddr *)&addr, sizeof(addr))<0) {
                sockerror("bind transparent");
                closesocket(s);
                return -1;
            }
        }

        /* try to connect for the 1st time */
        addr.sin_port=dport;
        addr.sin_addr.s_addr=*list;
        safe_ntoa(c->connecting_address, addr.sin_addr);
        log(LOG_DEBUG, "%s connecting %s:%d", c->opt->servname,
            c->connecting_address, ntohs(addr.sin_port));
        if(!connect(s, (struct sockaddr *)&addr, sizeof(addr)))
            return s; /* no error -> success */
        error=get_last_socket_error();
        switch(error) {
        case EINPROGRESS: /* retry */
            log(LOG_DEBUG, "remote connect #1: EINPROGRESS: retrying");
            break;
        case EWOULDBLOCK: /* retry */
            log(LOG_DEBUG, "remote connect #1: EWOULDBLOCK: retrying");
            break;
        default:
            log(LOG_ERR, "remote connect #1 (%s:%d): %s (%d)",
                c->connecting_address, ntohs(addr.sin_port),
                my_strerror(error), error);
            closesocket(s);
            continue; /* Next IP */
        }

        /* wait until the connecting socket is ready for write */
        if(waitforsocket(s, 1 /* write */, c->opt->timeout_busy)<1) {
            closesocket(s);
            continue; /* timeout or error */
        }

        /* try to connect for the 2nd time */
        if(!connect(s, (struct sockaddr *)&addr, sizeof(addr)))
            return s; /* no error -> success */
        error=get_last_socket_error();
        switch(error) {
        case EINVAL: /* WIN32 is strange... */
            log(LOG_DEBUG, "remote connect #2: EINVAL: ok");
        case EISCONN: /* ok */
            return s; /* success */
        default:
            log(LOG_ERR, "remote connect #2 (%s:%d): %s (%d)",
                c->connecting_address, ntohs(addr.sin_port),
                my_strerror(error), error);
            closesocket(s);
            continue; /* Next IP */
        }
    }
    return -1;
}

static void reset(int fd, char *txt) {
    /* Set lingering on a socket if needed*/
    struct linger l;

    l.l_onoff=1;
    l.l_linger=0;
    if(setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&l, sizeof(l)))
        log_error(LOG_DEBUG, get_last_socket_error(), txt);
}

/* End of client.c */
