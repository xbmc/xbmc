/* relaylib.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#if WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#endif

#if __UNIX__
#include <arpa/inet.h>
#elif __BEOS__
#include <be/net/netdb.h>   
#endif

#include <string.h>
#include "relaylib.h"
#include "srtypes.h"
#include "http.h"
#include "socklib.h"
#include "threadlib.h"
#include "debug.h"
#include "compat.h"
#include "rip_manager.h"
#include "cbuf2.h"

#if defined (WIN32)
#ifdef errno
#undef errno
#endif
#define errno WSAGetLastError()
#endif

/*****************************************************************************
 * Global variables
 *****************************************************************************/
RELAY_LIST* g_relay_list = NULL;
unsigned long g_relay_list_len = 0;
HSEM g_relay_list_sem;

/*****************************************************************************
 * Private vars
 *****************************************************************************/
static HSEM m_sem_not_connected;
static char m_http_header[MAX_HEADER_LEN];
static SOCKET m_listensock = SOCKET_ERROR;
static BOOL m_running = FALSE;
static BOOL m_initdone = FALSE;
static THREAD_HANDLE m_hthread;
static THREAD_HANDLE m_hthread2;

static int m_max_connections;
static int m_have_metadata;

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void thread_accept(void *notused);
static error_code try_port(u_short port, char *if_name, char *relay_ip);
static void thread_send(void *notused);

#define BUFSIZE (1024)

#define HTTP_HEADER_TIMEOUT 2
#define HTTP_HEADER_DELIM "\n"
#define ICY_METADATA_TAG "Icy-MetaData:"

static void
destroy_all_hostsocks(void)
{
    RELAY_LIST* ptr;

    threadlib_waitfor_sem (&g_relay_list_sem);
    while (g_relay_list != NULL) {
        ptr = g_relay_list;
        closesocket(ptr->m_sock);
        g_relay_list = ptr->m_next;
        if (ptr->m_buffer != NULL)
            free (ptr->m_buffer);
        free(ptr);
    }
    g_relay_list_len = 0;
    threadlib_signal_sem (&g_relay_list_sem);
}

static int
tag_compare (char *str, char *tag)
{
    int i, a,b;
    int len;

    len = strlen(tag);

    for (i=0; i<len; i++) {
	a = tolower (str[i]);
	b = tolower (tag[i]);
	if ((a != b) || (a == 0))
	    return 1;
    }
    return 0;
}

static int
header_receive (int sock, int *icy_metadata)
{
    fd_set fds;
    struct timeval tv;
    int r;
    char buf[BUFSIZE+1];
    char *md;

    *icy_metadata = 0;
        
    while (1) {
	// use select to prevent deadlock on malformed http header
	// that lacks CRLF delimiter
	FD_ZERO(&fds);
        FD_SET(sock, &fds);
        tv.tv_sec = HTTP_HEADER_TIMEOUT;
        tv.tv_usec = 0;
        r = select(sock + 1, &fds, NULL, NULL, &tv);
        if (r != 1) {
	    debug_printf ("header_receive: could not select\n");
	    break;
	}

	r = recv(sock, buf, BUFSIZE, 0);
	if (r <= 0) {
	    debug_printf ("header_receive: could not select\n");
	    break;
	}

	buf[r] = 0;
	md = strtok (buf, HTTP_HEADER_DELIM);
	while (md) {
	    debug_printf ("Got token: %s\n", md);
	    // Finished when we are at end of header: only CRLF will be there.
	    if ((md[0] == '\r') && (md[1] == 0))
		return 0;
	
	    // Check for desired tag
	    if (tag_compare (md, ICY_METADATA_TAG) == 0) {
		for (md += strlen(ICY_METADATA_TAG); md[0] && (isdigit(md[0]) == 0); md++);
		
		if (md[0])
		    *icy_metadata = atoi(md);

		debug_printf ("client flag ICY-METADATA is %d\n", 
			      *icy_metadata);
	    }
	    
	    md = strtok (NULL, HTTP_HEADER_DELIM);
	}
    }

    return 1;
}


// Quick function to "eat" incoming data from a socket
// All data is discarded
// Returns 0 if successful or SOCKET_ERROR if error
static int
swallow_receive (int sock)
{
    fd_set fds;
    struct timeval tv;
    int ret = 0;
    char buf[BUFSIZE];
    BOOL hasmore = TRUE;
        
    FD_ZERO(&fds);
    while (hasmore) {
        // Poll the socket to see if it has anything to read
        hasmore = FALSE;
        FD_SET(sock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ret = select(sock + 1, &fds, NULL, NULL, &tv);
        if (ret == 1) {
            // Read and throw away data, ignoring errors
            ret = recv(sock, buf, BUFSIZE, 0);
            if (ret > 0) {
                hasmore = TRUE;
            }
            else if (ret == SOCKET_ERROR) {
                break;
            }
        }
        else if (ret == SOCKET_ERROR) {
            break;
        }
    }
        
    return ret;
}


// Makes a socket non-blocking
void
make_nonblocking (int sock)
{
    int opt;

#ifndef WIN32
    opt = fcntl(sock, F_GETFL);
    if (opt != SOCKET_ERROR) {
        fcntl(sock, F_SETFL, opt | O_NONBLOCK);
    }
#else
    opt = 1;
    ioctlsocket(sock, FIONBIO, &opt);
#endif
}

BOOL relaylib_isrunning()
{
    return m_running;
}

error_code
relaylib_set_response_header(char *http_header)
{
    if (!http_header)
        return SR_ERROR_INVALID_PARAM;

    strcpy(m_http_header, http_header);

    return SR_SUCCESS;
}

#ifndef WIN32
void
catch_pipe(int code)
{
    //m_sock = 0;
    //m_connected = FALSE;
    // JCBUG, not sure what to do about this
}
#endif

error_code
relaylib_init (BOOL search_ports, u_short relay_port, u_short max_port, 
	       u_short *port_used, char *if_name, int max_connections, 
	       char *relay_ip, int have_metadata)
{
    int ret;
#ifdef WIN32
    WSADATA wsd;
#endif

    debug_printf ("relaylib_init()\n");

#ifdef WIN32
    if (WSAStartup(MAKEWORD(2,2), &wsd) != 0) {
	debug_printf ("relaylib_init(): SR_ERROR_CANT_BIND_ON_PORT\n");
        return SR_ERROR_CANT_BIND_ON_PORT;
    }
#endif

    if (relay_port < 1 || !port_used) {
	debug_printf ("relaylib_init(): SR_ERROR_INVALID_PARAM\n");
        return SR_ERROR_INVALID_PARAM;
    }

#ifndef WIN32
    // catch a SIGPIPE if send fails
    signal(SIGPIPE, catch_pipe);
#endif

    if (m_initdone != TRUE) {
        m_sem_not_connected = threadlib_create_sem();
        g_relay_list_sem = threadlib_create_sem();
        threadlib_signal_sem(&g_relay_list_sem);

        // NOTE: we need to signal it here in case we try to destroy
        // relaylib before the thread starts!
        threadlib_signal_sem(&m_sem_not_connected);
        m_initdone = TRUE;
    }
    m_max_connections = max_connections;
    m_have_metadata = have_metadata;
    *port_used = 0;
    if (!search_ports)
        max_port = relay_port;

    for(;relay_port <= max_port; relay_port++) {
        ret = try_port ((u_short)relay_port, if_name, relay_ip);
        if (ret == SR_ERROR_CANT_BIND_ON_PORT)
            continue;           // Keep searching.

        if (ret == SR_SUCCESS) {
            *port_used = relay_port;
            debug_printf ("Relay: Listening on port %d\n", relay_port);
            return SR_SUCCESS;
        } else {
            return ret;
        }
    }

    return SR_ERROR_CANT_BIND_ON_PORT;
}

error_code
try_port (u_short port, char *if_name, char *relay_ip)
{
    struct hostent *he;
    struct sockaddr_in local;

    m_listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_listensock == SOCKET_ERROR) {
	debug_printf ("try_port(%d) failed socket() call\n", port);
        return SR_ERROR_SOCK_BASE;
    }
    make_nonblocking(m_listensock);

    if ('\0' == *relay_ip) {
	if (read_interface(if_name,&local.sin_addr.s_addr) != 0)
	    local.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
	he = gethostbyname(relay_ip);
	memcpy(&local.sin_addr, he->h_addr_list[0], he->h_length);
    }

    local.sin_family = AF_INET;
    local.sin_port = htons(port);

#ifndef WIN32
    {
        // Prevent port error when restarting quickly after a previous exit
        int opt = 1;
        setsockopt(m_listensock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
#endif
                        
    if (bind(m_listensock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
    {
	debug_printf ("try_port(%d) failed bind() call\n", port);
        closesocket(m_listensock);
        m_listensock = SOCKET_ERROR;
        return SR_ERROR_CANT_BIND_ON_PORT;
    }
        
    if (listen(m_listensock, 1) == SOCKET_ERROR)
    {
	debug_printf ("try_port(%d) failed listen() call\n", port);
        closesocket(m_listensock);
        m_listensock = SOCKET_ERROR;
        return SR_ERROR_SOCK_BASE;
    }

    debug_printf ("try_port(%d) succeeded\n", port);
    return SR_SUCCESS;
}

void
relaylib_shutdown ()
{
    debug_printf("relaylib_shutdown:start\n");
    if (!relaylib_isrunning()) {
        debug_printf("***relaylib_shutdown:return\n");
        return;
    }
    m_running = FALSE;
    threadlib_signal_sem(&m_sem_not_connected);
    if (closesocket(m_listensock) == SOCKET_ERROR) {   
        // JCBUG, what can we do?
    }
    m_listensock = SOCKET_ERROR;                // Accept thread will watch for this and not try to accept anymore
    memset(m_http_header, 0, MAX_HEADER_LEN);
    debug_printf("waiting for relay close\n");
    threadlib_waitforclose(&m_hthread);
    threadlib_waitforclose(&m_hthread2);
    destroy_all_hostsocks();
    threadlib_destroy_sem(&m_sem_not_connected);
    m_initdone = FALSE;

    debug_printf("relaylib_shutdown:done!\n");
}

error_code
relaylib_start ()
{
    int ret;

    m_running = TRUE;
    // Spawn on a thread so it's non-blocking
    if ((ret = threadlib_beginthread(&m_hthread, thread_accept, 0)) != SR_SUCCESS)
        return ret;

    if ((ret = threadlib_beginthread(&m_hthread2, thread_send, 0)) != SR_SUCCESS)
        return ret;

    return SR_SUCCESS;
}

void
thread_accept (void *notused)
{
    int ret;
    int newsock;
    BOOL good;
    struct sockaddr_in client;
    int iAddrSize = sizeof(client);
    RELAY_LIST* newhostsock;
    int icy_metadata;
    char* client_http_header;

    debug_printf("thread_accept:start\n");

    while (m_running)
    {
        fd_set fds;
        struct timeval tv;

        // this event will keep use from accepting while we 
	//   have a connection active
        // when a connection gets dropped, or when streamripper shuts down
        //   this event will get signaled
	debug_printf("thread_accept:waiting on m_sem_not_connected\n");
        threadlib_waitfor_sem (&m_sem_not_connected);
        if (!m_running) {
	    debug_printf("thread_accept:exit (no longer m_running)\n");
            break;
	}

        // Poll once per second, instead of blocking forever in 
	// accept(), so that we can regain control if relaylib_shutdown() 
	// called
        FD_ZERO (&fds);
        while (m_listensock != SOCKET_ERROR)
        {
            FD_SET (m_listensock, &fds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;
	    debug_printf("thread_accept:calling select()\n");
            ret = select (m_listensock + 1, &fds, NULL, NULL, &tv);
	    debug_printf("thread_accept:select() returned\n");
            if (ret == 1) {
                unsigned long num_connected;
                /* If connections are full, do nothing.  Note that 
                    m_max_connections is 0 for infinite connections allowed. */
                threadlib_waitfor_sem (&g_relay_list_sem);
                num_connected = g_relay_list_len;
                threadlib_signal_sem (&g_relay_list_sem);
                if (m_max_connections > 0 && num_connected >= (unsigned long) m_max_connections) {
                    continue;
                }
                /* Check for connections */
		debug_printf ("Calling accept()\n");
                newsock = accept (m_listensock, (struct sockaddr *)&client, &iAddrSize);
                if (newsock != SOCKET_ERROR) {
                    // Got successful accept

                    debug_printf ("Relay: Client %d new from %s:%hd\n", newsock,
                                  inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                    // Socket is new and its buffer had better have 
		    // room to hold the entire HTTP header!
                    good = FALSE;
                    if (header_receive (newsock, &icy_metadata) == 0) {
			int header_len;
			make_nonblocking (newsock);
			client_http_header = client_relay_header_generate(icy_metadata);
			header_len = strlen (client_http_header);
			ret = send (newsock, client_http_header, strlen(client_http_header), 0);
			debug_printf ("Relay: Sent response header to client %d (%d)\n", 
			    ret, header_len);
			client_relay_header_release (client_http_header);
			if (ret == header_len) {
                            newhostsock = malloc (sizeof(RELAY_LIST));
                            if (newhostsock != NULL) {
                                // Add new client to list (headfirst)
                                threadlib_waitfor_sem (&g_relay_list_sem);
                                newhostsock->m_is_new = 1;
                                newhostsock->m_sock = newsock;
                                newhostsock->m_next = g_relay_list;
				if (m_have_metadata) {
                                    newhostsock->m_icy_metadata = icy_metadata;
				} else {
                                    newhostsock->m_icy_metadata = 0;
				}

                                g_relay_list = newhostsock;
                                g_relay_list_len++;
                                threadlib_signal_sem (&g_relay_list_sem);
                                good = TRUE;
                            }
                        }
                    }

                    if (!good)
                    {
                        closesocket (newsock);
                        debug_printf ("Relay: Client %d disconnected (Unable to receive HTTP header)\n", newsock);
                    }
                }
            }
            else if (ret == SOCKET_ERROR)
            {
                // Something went wrong with select
                break;
            }
        }
        threadlib_signal_sem (&m_sem_not_connected);     // go back to accept
    }

    m_running = FALSE;
}

/* Sock is ready to receive, so send it from cbuf to relay */
static BOOL 
send_to_relay (RELAY_LIST* ptr)
{
    int ret;
    int err_errno;
    BOOL good = TRUE;
    int done = 0;

    /* For new clients, initialize cbuf pointers */
    if (ptr->m_is_new) {
	int burst_amount = 32*1024;
	//	int burst_amount = 64*1024;
	//	int burst_amount = 128*1024;
	ptr->m_offset = 0;
	ptr->m_left_to_send = 0;

	cbuf2_init_relay_entry (&g_cbuf2, ptr, burst_amount);
	
	ptr->m_is_new = 0;
    }

    while (!done) {
	/* If our private buffer is empty, copy some from the cbuf */
	if (!ptr->m_left_to_send) {
	    error_code rc;
	    ptr->m_offset = 0;
	    rc = cbuf2_extract_relay (&g_cbuf2, ptr);
	    
	    if (rc == SR_ERROR_BUFFER_EMPTY) {
		break;
	    }
	}
	/* Send from the private buffer to the client */
	debug_printf ("Relay: Sending Client %d to the client\n", 
		      ptr->m_left_to_send );
	ret = send (ptr->m_sock, ptr->m_buffer+ptr->m_offset, 
		    ptr->m_left_to_send, 0);
	debug_printf ("Relay: Sending to Client returned %d\n", ret );
	if (ret == SOCKET_ERROR) {
	    /* Sometimes windows gives me an errno of 0
	       Sometimes windows gives me an errno of 183 
	       See this thread for details: 
	       http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&selm=8956d3e8.0309100905.6ba60e7f%40posting.google.com
	    */
	    err_errno = errno;
	    if (err_errno == EWOULDBLOCK || err_errno == 0 
		|| err_errno == 183)
	    {
#if defined (WIN32)
		// Client is slow.  Retry later.
		WSASetLastError (0);
#endif
	    } else {
		debug_printf ("Relay: socket error is %d\n",errno);
		good = FALSE;
	    }
	    done = 1;
	} else { 
	    // Send was successful
	    ptr->m_offset += ret;
	    ptr->m_left_to_send -= ret;
	    if (ptr->m_left_to_send < 0) {
		/* GCS: can this ever happen??? */
		debug_printf ("ptr->m_left_to_send < 0\n");
		ptr->m_left_to_send = 0;
		done = 1;
	    }
	}
    }
    return good;
}

void 
relaylib_disconnect (RELAY_LIST* prev, RELAY_LIST* ptr)
{
    RELAY_LIST* next = ptr->m_next;
    int sock = ptr->m_sock;

    closesocket (sock);
                                   
    // Carefully delete this client from list without 
    // affecting list order
    if (prev != NULL) {
	prev->m_next = next;
    } else {
	g_relay_list = next;
    }
    if (ptr->m_buffer != NULL)
	free (ptr->m_buffer);
    free (ptr);
    g_relay_list_len --;
}

void 
thread_send (void *notused)
{
    RELAY_LIST* prev;
    RELAY_LIST* ptr;
    RELAY_LIST* next;
    int sock;
    BOOL good;
    error_code err = SR_SUCCESS;

    while (m_running) {
	threadlib_waitfor_sem (&g_relay_list_sem);
	ptr = g_relay_list;
	if (ptr != NULL) {
	    prev = NULL;
	    while (ptr != NULL) {
		sock = ptr->m_sock;
		next = ptr->m_next;

		if (swallow_receive(sock) != 0) {
		    good = FALSE;
		} else {
		    good = send_to_relay (ptr);
		}
	       
		if (!good) {
		    debug_printf ("Relay: Client %d disconnected (%s)\n", 
				  sock, strerror(errno));
		    relaylib_disconnect (prev, ptr);
		} else if (ptr != NULL) {
		    prev = ptr;
		}
		ptr = next;
	    }
	} else {
	    err = SR_ERROR_HOST_NOT_CONNECTED;
	}
	threadlib_signal_sem (&g_relay_list_sem);
	Sleep (50);
    }
}
