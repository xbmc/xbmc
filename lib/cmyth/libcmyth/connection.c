/*
 *  Copyright (C) 2004-2012, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file connection.c
 * Functions to handle creating connections to a MythTV backend and
 * interacting with those connections.  
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <cmyth_local.h>

static char * cmyth_conn_get_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	unsigned int version;
	char token[9]; // 8 characters + the terminating NULL character
} myth_protomap_t;

static myth_protomap_t protomap[] = {
	{62, "78B5631E"},
	{63, "3875641D"},
	{64, "8675309J"},
	{65, "D2BB94C2"},
	{66, "0C0FFEE0"},
	{67, "0G0G0G0"},
	{68, "90094EAD"},
	{69, "63835135"},
	{70, "53153836"},
	{71, "05e82186"},
	{72, "D78EFD6F"},
	{73, "D7FE8D6F"},
	{0, ""}
};

/*
 * cmyth_conn_destroy(cmyth_conn_t conn)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Tear down and release storage associated with a connection.  This
 * should only be called by cmyth_conn_release().  All others should
 * call ref_release() to release a connection.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_conn_destroy(cmyth_conn_t conn)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !\n", __FUNCTION__);
		return;
	}
	if (conn->conn_buf) {
		free(conn->conn_buf);
	}
	if (conn->conn_fd >= 0) {
		cmyth_dbg(CMYTH_DBG_PROTO,
			  "%s: shutdown and close connection fd = %d\n",
			  __FUNCTION__, conn->conn_fd);
		shutdown(conn->conn_fd, SHUT_RDWR);
		closesocket(conn->conn_fd);
	}
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
 * cmyth_conn_create(void)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Allocate and initialize a cmyth_conn_t structure.  This should only
 * be called by cmyth_connect(), which establishes a connection.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_conn_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_conn_t
 */
static cmyth_conn_t
cmyth_conn_create(void)
{
	cmyth_conn_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_conn_destroy);

	ret->conn_fd = -1;
	ret->conn_buf = NULL;
	ret->conn_len = 0;
	ret->conn_buflen = 0;
	ret->conn_pos = 0;
	ret->conn_hang = 0;
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_connect(char *server, unsigned short port, unsigned buflen)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a connection to the port specified by 'port' on the
 * server named 'server'.  This creates a data structure called a
 * cmyth_conn_t which contains the file descriptor for a socket, a
 * buffer for reading from the socket, and information used to manage
 * offsets in that buffer.  The buffer length is specified in 'buflen'.
 *
 * The returned connection has a single reference.  The connection
 * will be shut down and closed when the last reference is released
 * using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_conn_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_conn_t
 */
static char my_hostname[128];
static volatile cmyth_socket_t my_fd;

static void
sighandler(int sig)
{
	/*
	 * XXX: This is not thread safe...
	 */
	closesocket(my_fd);
	my_fd = -1;
}

static cmyth_conn_t
cmyth_connect_addr(struct addrinfo* addr, unsigned buflen,
		    int tcp_rcvbuf)
{
	cmyth_conn_t ret = NULL;
	unsigned char *buf = NULL;
	cmyth_socket_t fd;
#ifndef _MSC_VER
	void (*old_sighandler)(int);
	int old_alarm;
#endif
	int temp;
	socklen_t size;
	char namebuf[NI_MAXHOST], portbuf[NI_MAXSERV];

	fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (fd < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cannot create socket (%d)\n",
			  __FUNCTION__, errno);
		return NULL;
	}

	/*
	 * Set a 4kb tcp receive buffer on all myth protocol sockets,
	 * otherwise we risk the connection hanging.  Oddly, setting this
	 * on the data sockets causes stuttering during playback.
	 */
	if (tcp_rcvbuf == 0)
		tcp_rcvbuf = 4096;

	temp = tcp_rcvbuf;
	size = sizeof(temp);
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, size);
	if(getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, &size)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: could not get rcvbuf from socket(%d)\n",
			  __FUNCTION__, errno);
		temp = tcp_rcvbuf;
	}
	tcp_rcvbuf = temp;

	if (getnameinfo(addr->ai_addr, addr->ai_addrlen, namebuf, sizeof(namebuf), portbuf, sizeof(portbuf), NI_NUMERICHOST)) {
		strcpy(namebuf, "[unknown]");
		strcpy(portbuf, "[unknown]");
	}

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting to %s:%s fd = %d\n",
			__FUNCTION__, namebuf, portbuf, fd);
#ifndef _MSC_VER
	old_sighandler = signal(SIGALRM, sighandler);
	old_alarm = alarm(5);
#endif
	my_fd = fd;
	if (connect(fd, addr->ai_addr, addr->ai_addrlen) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: connect failed on port %s to '%s' (%d)\n",
			  __FUNCTION__, portbuf, namebuf, errno);
		closesocket(fd);
#ifndef _MSC_VER
		signal(SIGALRM, old_sighandler);
		alarm(old_alarm);
#endif
		return NULL;
	}
	my_fd = -1;
#ifndef _MSC_VER
	signal(SIGALRM, old_sighandler);
	alarm(old_alarm);
#endif

	if ((my_hostname[0] == '\0') &&
	    (gethostname(my_hostname, sizeof(my_hostname)) < 0)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: gethostname failed (%d)\n",
			  __FUNCTION__, errno);
		goto shut;
	}

	/*
	 * Okay, we are connected. Now is a good time to allocate some
	 * resources.
	 */
	ret = cmyth_conn_create();
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_conn_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	buf = malloc(buflen * sizeof(unsigned char));
	if (!buf) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s:- malloc(%d) failed allocating buf\n",
			  __FUNCTION__, buflen * sizeof(unsigned char *));
		goto shut;
	}
	ret->conn_fd = fd;
	ret->conn_buflen = buflen;
	ret->conn_buf = buf;
	ret->conn_len = 0;
	ret->conn_pos = 0;
	ret->conn_version = 8;
	ret->conn_tcp_rcvbuf = tcp_rcvbuf;
	return ret;

    shut:
	if (ret) {
		ref_release(ret);
	}

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: error connecting to "
		  "%s, shutdown and close fd = %d\n",
		  __FUNCTION__, namebuf, fd);
	shutdown(fd, 2);
	closesocket(fd);
	return NULL;
}

static cmyth_conn_t
cmyth_connect(char *server, unsigned short port, unsigned buflen,
		    int tcp_rcvbuf)
{
	struct   addrinfo hints;
	struct   addrinfo *result, *addr;
	char     service[33];
	int      res;
	cmyth_conn_t conn = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	sprintf(service, "%d", port);

	res = getaddrinfo(server, service, &hints, &result);
	if(res) {
		switch(res) {
		case EAI_NONAME:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- The specified host is unknown\n",
					__FUNCTION__);
			break;

		case EAI_FAIL:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A non-recoverable failure in name resolution occurred\n",
					__FUNCTION__);
			break;

		case EAI_MEMORY:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A memory allocation failure occurred\n",
					__FUNCTION__);
			break;

		case EAI_AGAIN:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A temporary error occurred on an authoritative name server\n",
					__FUNCTION__);
			break;

		default:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- Unknown error %d\n",
					__FUNCTION__, res);
			break;
		}
		return NULL;
	}

	for (addr = result; addr; addr = addr->ai_next) {
		conn = cmyth_connect_addr(addr, buflen, tcp_rcvbuf);
		if (conn)
			break;
	}

	freeaddrinfo(result);
	return conn;
}

static cmyth_conn_t
cmyth_conn_connect(char *server, unsigned short port, unsigned buflen,
		   int tcp_rcvbuf, int event)
{
	cmyth_conn_t conn;
	char announcement[256];
	unsigned long tmp_ver;
	int attempt = 0;

    top:
	conn = cmyth_connect(server, port, buflen, tcp_rcvbuf);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, server, port, buflen);
		return NULL;
	}

	/*
	 * Find out what the Myth Protocol Version is for this connection.
	 * Loop around until we get agreement from the server.
	 */
	if (attempt == 0)
		tmp_ver = conn->conn_version;
	conn->conn_version = tmp_ver;

	/*
	 * Myth 0.23.1 (Myth 0.23 + fixes) introduced an out of sequence protocol version number (23056)
	 * due to the next protocol version number having already been bumped in trunk.
	 *
	 * http://www.mythtv.org/wiki/Myth_Protocol
	 */
	if (tmp_ver >= 62 && tmp_ver != 23056) { // Treat protocol version number 23056 the same as protocol 56
		myth_protomap_t *map = protomap;
		while (map->version != 0 && map->version != tmp_ver)
			map++;
		if (map->version == 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		sprintf(announcement, "MYTH_PROTO_VERSION %ld %s", conn->conn_version, map->token);
	} else {
		sprintf(announcement, "MYTH_PROTO_VERSION %ld", conn->conn_version);
	}
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	if (cmyth_rcv_version(conn, &tmp_ver) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_version() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_ERROR,
		  "%s: asked for version %ld, got version %ld\n",
		  __FUNCTION__, conn->conn_version, tmp_ver);
	if (conn->conn_version != tmp_ver) {
		if (attempt == 1) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		attempt = 1;
		ref_release(conn);
		goto top;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: agreed on Version %ld protocol\n",
		  __FUNCTION__, conn->conn_version);

	sprintf(announcement, "ANN Playback %s %d", my_hostname, event);
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	if (cmyth_rcv_okay(conn, "OK") < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}

	/*
	 * All of the downstream code in libcmyth assumes a monotonically increasing version number.
	 * This was not the case for Myth 0.23.1 (0.23 + fixes) where protocol version number 23056
	 * was used since 57 had already been used in trunk.
	 *
	 * Convert from protocol version number 23056 to version number 56 so subsequent code within
	 * libcmyth uses the same logic for the 23056 protocol as would be used for protocol version 56.
	 */
	if (conn->conn_version == 23056) {
		conn->conn_version = 56;
	}

	return conn;

    shut:
	ref_release(conn);
	return NULL;
}

/*
 * cmyth_conn_connect_ctrl(char *server, unsigned short port, unsigned buflen)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a connection for use as a control connection within the
 * MythTV protocol.  Return a pointer to the newly created connection.
 * The connection is returned held, and may be released using
 * ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
cmyth_conn_t
cmyth_conn_connect_ctrl(char *server, unsigned short port, unsigned buflen,
			int tcp_rcvbuf)
{
	cmyth_conn_t ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting control connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 0);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting control connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

cmyth_conn_t
cmyth_conn_connect_event(char *server, unsigned short port, unsigned buflen,
			 int tcp_rcvbuf)
{
	cmyth_conn_t ret;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting event channel connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 1);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting event channel connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_connect_file(char *server, unsigned short port, unsigned buflen
 *                         cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a file structure containing a data connection for use
 * transfering a file within the MythTV protocol.  Return a pointer to
 * the newly created file structure.  The connection in the file
 * structure is returned held as is the file structure itself.  The
 * connection will be released when the file structure is released.
 * The file structure can be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_file_t (this is a pointer type)
 *
 * Failure: NULL cmyth_file_t
 */
cmyth_file_t
cmyth_conn_connect_file(cmyth_proginfo_t prog,  cmyth_conn_t control,
			unsigned buflen, int tcp_rcvbuf)
{
	cmyth_conn_t conn = NULL;
	char *announcement = NULL;
	char *myth_host = NULL;
	char reply[16];
	int err = 0;
	int count = 0;
	int r;
	int ann_size = sizeof("ANN FileTransfer  0[]:[][]:[]");
	cmyth_file_t ret = NULL;

	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog is NULL\n", __FUNCTION__);
		goto shut;
	}
	if (!prog->proginfo_host) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog host is NULL\n",
			  __FUNCTION__);
		goto shut;
	}
	if (!prog->proginfo_pathname) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog has no pathname in it\n",
			  __FUNCTION__);
		goto shut;
	}
	ret = cmyth_file_create(control);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_file_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting data connection\n",
		  __FUNCTION__);
	if (control->conn_version >= 17) {
		myth_host = cmyth_conn_get_setting_unlocked(control, prog->proginfo_host,
		                                   "BackendServerIP");
		if (myth_host && (strcmp(myth_host, "-1") == 0)) {
			ref_release(myth_host);
			myth_host = NULL;
		}
	}
	if (!myth_host) {
		cmyth_dbg(CMYTH_DBG_PROTO,
		          "%s: BackendServerIP setting not found. Using proginfo_host: %s\n",
		          __FUNCTION__, prog->proginfo_host);
		myth_host = ref_alloc(strlen(prog->proginfo_host) + 1);
		strcpy(myth_host, prog->proginfo_host);
	}
	conn = cmyth_connect(myth_host, prog->proginfo_port,
			     buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting data connection, conn = %d\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__,
			  myth_host, prog->proginfo_port, buflen);
		goto shut;
	}
	/*
	 * Explicitly set the conn version to the control version as cmyth_connect() doesn't and some of
	 * the cmyth_rcv_* functions expect it to be the same as the protocol version used by mythbackend.
	 */
	conn->conn_version = control->conn_version;

	ann_size += strlen(prog->proginfo_pathname) + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	if (control->conn_version >= 44) {
		sprintf(announcement, "ANN FileTransfer %s 0[]:[]%s[]:[]", // write = false
			  my_hostname, prog->proginfo_pathname);
	}
	else {
		sprintf(announcement, "ANN FileTransfer %s[]:[]%s",
			  my_hostname, prog->proginfo_pathname);
	}

	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	ret->file_data = ref_hold(conn);
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto shut;
	}
	reply[sizeof(reply) - 1] = '\0';
	r = cmyth_rcv_string(conn, &err, reply, sizeof(reply) - 1, count); 
	if (err != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	if (strcmp(reply, "OK") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: reply ('%s') is not 'OK'\n",
			  __FUNCTION__, reply);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_long(conn, &err, &ret->file_id, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (id) cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_uint64(conn, &err, &ret->file_length, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (length) cmyth_rcv_uint64() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	if (count != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: %d leftover bytes\n",
			  __FUNCTION__, count);
	}
	free(announcement);
	ref_release(conn);
	ref_release(myth_host);
	return ret;

    shut:
	if (announcement) {
		free(announcement);
	}
	ref_release(ret);
	ref_release(conn);
	ref_release(myth_host);
	return NULL;
}

/*
 * cmyth_conn_connect_path(char* path, cmyth_conn_t control,
 *                         unsigned buflen, int tcp_rcvbuf)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a file structure containing a data connection for use
 * transfering a file within the MythTV protocol.  Return a pointer to
 * the newly created file structure.  The connection in the file
 * structure is returned held as is the file structure itself.  The
 * connection will be released when the file structure is released.
 * The file structure can be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_file_t (this is a pointer type)
 *
 * Failure: NULL cmyth_file_t
 */
cmyth_file_t
cmyth_conn_connect_path(char* path, cmyth_conn_t control,
			unsigned buflen, int tcp_rcvbuf)
{
	cmyth_conn_t conn = NULL;
	char *announcement = NULL;
	char reply[16];
	char host[256];
	int err = 0;
	int count = 0;
	int r, port;
	int ann_size = sizeof("ANN FileTransfer  0[]:[][]:[]");
	struct sockaddr_in addr;
        socklen_t addr_size = sizeof(addr);
	cmyth_file_t ret = NULL;

	if (getpeername(control->conn_fd, (struct sockaddr*)&addr, &addr_size)<0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: getpeername() failed\n",
			  __FUNCTION__);
		goto shut;
	}

	inet_ntop(addr.sin_family, &addr.sin_addr, host, sizeof(host));
	port = ntohs(addr.sin_port);

	ret = cmyth_file_create(control);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_file_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting data connection\n",
		  __FUNCTION__);
	conn = cmyth_connect(host, port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting data connection, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, host, port, buflen);
		goto shut;
	}
	/*
	 * Explicitly set the conn version to the control version as cmyth_connect() doesn't and some of
	 * the cmyth_rcv_* functions expect it to be the same as the protocol version used by mythbackend.
	 */
	conn->conn_version = control->conn_version;

	ann_size += strlen(path) + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	if (control->conn_version >= 44) {
		sprintf(announcement, "ANN FileTransfer %s 0[]:[]%s[]:[]",  // write = false
			  my_hostname, path);
	}
	else {
		sprintf(announcement, "ANN FileTransfer %s[]:[]%s",
			  my_hostname, path);
	}
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	ret->file_data = ref_hold(conn);
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto shut;
	}
	reply[sizeof(reply) - 1] = '\0';
	r = cmyth_rcv_string(conn, &err, reply, sizeof(reply) - 1, count); 
	if (err != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	if (strcmp(reply, "OK") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: reply ('%s') is not 'OK'\n",
			  __FUNCTION__, reply);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_long(conn, &err, &ret->file_id, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (id) cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_uint64(conn, &err, &ret->file_length, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (length) cmyth_rcv_uint64() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	free(announcement);
	ref_release(conn);
	return ret;

    shut:
	if (announcement) {
		free(announcement);
	}
	ref_release(ret);
	ref_release(conn);
	return NULL;
}

/*
 * cmyth_conn_connect_ring(char *server, unsigned short port, unsigned buflen
 *                         cmyth_recorder_t rec)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a new ring buffer connection for use transferring live-tv
 * using the MythTV protocol.  Return a pointer to the newly created
 * ring buffer connection.  The ring buffer connection is returned
 * held, and may be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
int
cmyth_conn_connect_ring(cmyth_recorder_t rec, unsigned buflen, int tcp_rcvbuf)
{
	cmyth_conn_t conn;
	char *announcement;
	int ann_size = sizeof("ANN RingBuffer  ");
	char *server;
	unsigned short port;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	server = rec->rec_server;
	port = rec->rec_port;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting ringbuffer\n",
		  __FUNCTION__);
	conn = cmyth_connect(server, port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: connecting ringbuffer, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, server, port, buflen);
		return -1;
	}

	ann_size += CMYTH_LONG_LEN + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	sprintf(announcement,
		"ANN RingBuffer %s %d", my_hostname, rec->rec_id);
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		free(announcement);
		goto shut;
	}
	free(announcement);
	if (cmyth_rcv_okay(conn, "OK") < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}

        rec->rec_ring->conn_data = conn;
	return 0;

    shut:
	ref_release(conn);
	return -1;
}

int
cmyth_conn_connect_recorder(cmyth_recorder_t rec, unsigned buflen,
			    int tcp_rcvbuf)
{
	cmyth_conn_t conn;
	char *server;
	unsigned short port;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	server = rec->rec_server;
	port = rec->rec_port;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting recorder control\n",
		  __FUNCTION__);
	conn = cmyth_conn_connect_ctrl(server, port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting recorder control, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, server, port, buflen);
		return -1;
	}

	if (rec->rec_conn)
		ref_release(rec->rec_conn);
	rec->rec_conn = conn;

	return 0;
}

/*
 * cmyth_conn_check_block(cmyth_conn_t conn, unsigned long size)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Check whether a block has finished transfering from a backend
 * server. This non-blocking check looks for a response from the
 * server indicating that a block has been entirely sent to on a data
 * socket.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
int
cmyth_conn_check_block(cmyth_conn_t conn, unsigned long size)
{
	fd_set check;
	struct timeval timeout;
	int length;
	int err = 0;
	unsigned long sent;

	if (!conn) {
		return -EINVAL;
	}
	timeout.tv_sec = timeout.tv_usec = 0;
	FD_ZERO(&check);
	FD_SET(conn->conn_fd, &check);
	if (select((int)conn->conn_fd + 1, &check, NULL, NULL, &timeout) < 0) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: select failed (%d)\n",
			  __FUNCTION__, errno);
		return -(errno);
	}
	if (FD_ISSET(conn->conn_fd, &check)) {
		/*
		 * We have a bite, reel it in.
		 */
		length = cmyth_rcv_length(conn);
		if (length < 0) {
			return length;
		}
		cmyth_rcv_ulong(conn, &err, &sent, length);
		if (err) {
			return -err;
		}
		if (sent == size) {
			/*
			 * This block has been sent, return TRUE.
			 */
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: block finished (%d bytes)\n",
				  __FUNCTION__, sent);
			return 1;
		} else {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: block finished short (%d bytes)\n",
				  __FUNCTION__, sent);
			return -ECANCELED;
		}
	}
	return 0;
}

/*
 * cmyth_conn_get_recorder_from_num(cmyth_conn_t control,
 *                                  cmyth_recorder_num_t num,
 *                                  cmyth_recorder_t rec)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain a recorder from a connection by its recorder number.  The
 * recorder structure created by this describes how to set up a data
 * connection and play media streamed from a particular back-end recorder.
 *
 * This fills out the recorder structure specified by 'rec'.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
cmyth_recorder_t
cmyth_conn_get_recorder_from_num(cmyth_conn_t conn, int id)
{
	int err, count;
	int r;
	long port;
	char msg[256];
	char reply[256];
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	snprintf(msg, sizeof(msg), "GET_RECORDER_FROM_NUM[]:[]%d", id);

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}

	if ((r=cmyth_rcv_string(conn, &err,
				reply, sizeof(reply)-1, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;

	if ((r=cmyth_rcv_long(conn, &err, &port, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}

	if (port == -1)
		goto fail;

	rec->rec_id = id;
	rec->rec_server = ref_strdup(reply);
	rec->rec_port = port;

	if (cmyth_conn_connect_recorder(rec, conn->conn_buflen,
					conn->conn_tcp_rcvbuf) < 0)
		goto fail;

	pthread_mutex_unlock(&mutex);

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&mutex);

	return NULL;
}

/*
 * cmyth_conn_get_free_recorder(cmyth_conn_t control, cmyth_recorder_t rec)
 *                             
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the next available free recorder the connection specified by
 * 'control'.  This fills out the recorder structure specified by 'rec'.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
cmyth_recorder_t
cmyth_conn_get_free_recorder(cmyth_conn_t conn)
{
	int err, count;
	int r;
	long port, id;
	char msg[256];
	char reply[256];
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	snprintf(msg, sizeof(msg), "GET_FREE_RECORDER");

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}
	if ((r=cmyth_rcv_long(conn, &err, &id, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;
	if ((r=cmyth_rcv_string(conn, &err,
				reply, sizeof(reply)-1, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;
	if ((r=cmyth_rcv_long(conn, &err, &port, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}

	if (port == -1)
		goto fail;

	rec->rec_id = id;
	rec->rec_server = ref_strdup(reply);
	rec->rec_port = port;

	if (cmyth_conn_connect_recorder(rec, conn->conn_buflen,
					conn->conn_tcp_rcvbuf) < 0)
		goto fail;

	pthread_mutex_unlock(&mutex);

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&mutex);

	return NULL;
}

int
cmyth_conn_get_freespace(cmyth_conn_t control,
			 long long *total, long long *used)
{
	int err, count, ret = 0;
	int r;
	char msg[256];
	char reply[256];
	int64_t lreply;

	if (control == NULL)
		return -EINVAL;

	if ((total == NULL) || (used == NULL))
		return -EINVAL;

	pthread_mutex_lock(&mutex);

	if (control->conn_version >= 32)
		{ snprintf(msg, sizeof(msg), "QUERY_FREE_SPACE_SUMMARY"); }
	else if (control->conn_version >= 17)	
		{ snprintf(msg, sizeof(msg), "QUERY_FREE_SPACE"); }
	else
		{ snprintf(msg, sizeof(msg), "QUERY_FREESPACE"); }

	if ((err = cmyth_send_message(control, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	if ((count=cmyth_rcv_length(control)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto out;
	}
	
	if (control->conn_version >= 17) {
		if ((r=cmyth_rcv_int64(control, &err, &lreply, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, err);
			ret = err;
			goto out;
		}
		*total = lreply;
		if ((r=cmyth_rcv_int64(control, &err, &lreply, count - r)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, err);
			ret = err;
			goto out;
		}
		*used = lreply;
	}
	else
		{
			if ((r=cmyth_rcv_string(control, &err, reply,
						sizeof(reply)-1, count)) < 0) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_rcv_string() failed (%d)\n",
					  __FUNCTION__, err);
				ret = err;
				goto out;
			}
			*total = atoi(reply);
			if ((r=cmyth_rcv_string(control, &err, reply,
						sizeof(reply)-1,
						count-r)) < 0) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_rcv_string() failed (%d)\n",
					  __FUNCTION__, err);
				ret = err;
				goto out;
			}
			*used = atoi(reply);

			*used *= 1024;
			*total *= 1024;
		}

    out:
	pthread_mutex_unlock(&mutex);

	return ret;
}

int
cmyth_conn_hung(cmyth_conn_t control)
{
	if (control == NULL)
		return -EINVAL;

	return control->conn_hang;
}

int
cmyth_conn_get_protocol_version(cmyth_conn_t conn)
{
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			__FUNCTION__);
		return -1;
	}

	return conn->conn_version;
}


int
cmyth_conn_get_free_recorder_count(cmyth_conn_t conn)
{
	char msg[256];
	int count, err;
	long c, r;
	int ret;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&mutex);

	snprintf(msg, sizeof(msg), "GET_FREE_RECORDER_COUNT");
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto err;
	}

	if ((count=cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto err;
	}
	if ((r=cmyth_rcv_long(conn, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto err;
	}

	ret = c;

    err:
	pthread_mutex_unlock(&mutex);

	return ret;
}

static char *
cmyth_conn_get_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting)
{
	char msg[256];
	int count, err;
	char* result = NULL;

	if(conn->conn_version < 17) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: protocol version doesn't support QUERY_SETTING\n",
			  __FUNCTION__);
		return NULL;
	}

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	snprintf(msg, sizeof(msg), "QUERY_SETTING %s %s", hostname, setting);
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	if ((count=cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto err;
	}

	result = ref_alloc(count+1);
	count -= cmyth_rcv_string(conn, &err,
				    result, count, count);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	while(count > 0 && !err) {
		char buffer[100];
		count -= cmyth_rcv_string(conn, &err, buffer, sizeof(buffer)-1, count);
		buffer[sizeof(buffer)-1] = 0;
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: odd left over data %s\n", __FUNCTION__, buffer);
	}

	return result;
err:
	if(result)
		ref_release(result);

	return NULL;
}

char *
cmyth_conn_get_setting(cmyth_conn_t conn, const char* hostname, const char* setting)
{
	char* result = NULL;

	pthread_mutex_lock(&mutex);
	result = cmyth_conn_get_setting_unlocked(conn, hostname, setting);
	pthread_mutex_unlock(&mutex);

	return result;
}
