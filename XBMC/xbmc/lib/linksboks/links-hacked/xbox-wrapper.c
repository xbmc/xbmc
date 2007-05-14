/*
 * LinksBoks
 * Copyright (c) 2003-2005 ysbox
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

#include "links.h"

#ifdef __XBOX__

#define MAX_FDS			256
#define FIRST_FDNUM		5


typedef struct
{
	int file;
	SOCKET skt;
	XNDNS *dnsq;
	int type;
} xbox_fd_t;


#undef open
#undef close
#undef read
#undef write
#undef socket
#undef connect
#undef listen
#undef bind
#undef fcntl
#undef c_pipe
#undef getsockname
#undef accept
#undef select
//#undef FD_ISSET
//#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(fd), (fd_set FAR *)(set)) //dirty again
/*#undef FD_SET
#define FD_SET(fd, set) do { \
    u_int __i; \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == (fd)) { \
            break; \
        } \
    } \
    if (__i == ((fd_set FAR *)(set))->fd_count) { \
        if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE) { \
            ((fd_set FAR *)(set))->fd_array[__i] = (fd); \
            ((fd_set FAR *)(set))->fd_count++; \
        } \
    } \
} while(0) */

xbox_fd_t xbox_fd[MAX_FDS];


int sleep( int nb_sec )
{
	Sleep( (DWORD)nb_sec * 1000 );
	return 0;
}

int xbox_get_fd_type( int fd )
{
	return xbox_fd[fd].type;
}

SOCKET xbox_get_socket( int fd )
{
	return xbox_fd[fd].skt;
}

int xbox_get_file( int fd )
{
	return xbox_fd[fd].file;
}

XNDNS *xbox_get_dnsquery( int fd )
{
	return xbox_fd[fd].dnsq;
}

void xbox_unregisterfd( int fd )
{
	xbox_fd[fd].type = FD_TYPE_NONE;
	xbox_fd[fd].skt = 0;
	xbox_fd[fd].file = 0;
	xbox_fd[fd].dnsq = NULL;
}

int xbox_registersocket( SOCKET socket )
{
	int i;

	for( i = FIRST_FDNUM; i < MAX_FDS; i++ )
	{
		if( xbox_fd[i].type == FD_TYPE_NONE )
		{
			xbox_unregisterfd( i );
			xbox_fd[i].skt = socket;
			xbox_fd[i].type = FD_TYPE_SOCKET;
			return i;
		}
	}

	return -1;
}

int xbox_registerfile( int file )
{
	int i;

	for( i = FIRST_FDNUM; i < MAX_FDS; i++ )
	{
		if( xbox_fd[i].type == FD_TYPE_NONE )
		{
			xbox_unregisterfd( i );
			xbox_fd[i].file = file;
			xbox_fd[i].type = FD_TYPE_FILE;
			return i;
		}
	}

	return -1;
}

int xbox_registerdnsquery( XNDNS *query )
{
	int i;

	for( i = FIRST_FDNUM; i < MAX_FDS; i++ )
	{
		if( xbox_fd[i].type == FD_TYPE_NONE )
		{
			xbox_unregisterfd( i );
			xbox_fd[i].dnsq = query;
			xbox_fd[i].type = FD_TYPE_DNSQUERY;
			return i;
		}
	}

	return -1;
}

int xbox_open(const char *pathname, int flags)
{
	int fd = _open( pathname, flags, _S_IREAD | _S_IWRITE );
	if( fd == -1 )
		return -1;
	return( xbox_registerfile( fd ) );
}

int xbox_read(int s, void *ptr, int len)
{
	switch(xbox_get_fd_type(s))
	{
		case FD_TYPE_SOCKET:
			return recv(xbox_get_socket(s), ptr, len, 0);
		case FD_TYPE_FILE:
			return _read(xbox_get_file(s), ptr, len);
	}
	return -1;
}

int xbox_write(int s, void *ptr, int len)
{
	switch(xbox_get_fd_type(s))
	{
		case FD_TYPE_SOCKET:
            return send(xbox_get_socket(s), ptr, len, 0);
		case FD_TYPE_FILE:
			return _write(xbox_get_file(s), ptr, len);
	}
	return -1;
}

int xbox_close(int s)
{
	int result;

	switch(xbox_get_fd_type(s))
	{
		case FD_TYPE_SOCKET:
            result = closesocket(xbox_get_socket(s));
			break;
		case FD_TYPE_FILE:
			result = _close(xbox_get_file(s));
			break;
		case FD_TYPE_DNSQUERY:
			result = XNetDnsRelease(xbox_get_dnsquery(s));
			break;
	}

	xbox_unregisterfd( s );
	return result;
}

int xbox_socket(int af, int sock, int prot)
{
	SOCKET h = socket(af, sock, prot);
	if (h == INVALID_SOCKET) return -1;
	return xbox_registersocket( h );
}

int xbox_connect(int s, struct sockaddr *sa, int sal)
{
	return connect(xbox_get_socket(s), sa, sal);
}

int xbox_getpeername(int s, struct sockaddr *sa, int *sal)
{
	return getpeername(xbox_get_socket(s), sa, sal);
}

int xbox_getsockname(int s, struct sockaddr *sa, int *sal)
{
	return getsockname(xbox_get_socket(s), sa, sal);
}

int xbox_listen(int s, int c)
{
	return listen(xbox_get_socket(s), c);
}

int xbox_accept(int s, struct sockaddr *sa, int *sal)
{
	SOCKET a = accept(xbox_get_socket(s), sa, sal);
	if (a == INVALID_SOCKET) return -1;
	return xbox_registersocket( a );
}

int xbox_bind(int s, struct sockaddr *sa, int sal)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	if (!sin->sin_port) {
		while(1) {
			sin->sin_port = (rand() % 5000) + 16384;
			if (!xbox_bind(s, sa, sal)) return 0;
		}
	}
	if (bind(xbox_get_socket(s), sa, sal)) return -1;
	getsockname(xbox_get_socket(s), sa, &sal);
	return 0;
}

#define PIPE_RETRIES	10

int xbox_pipe(int *fd)
{
	int s1, s2, s3, l;
	struct sockaddr_in sa1, sa2;
	int retry_count = 0;
	again:
	if ((s1 = xbox_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		/*perror("socket1");*/
		goto fatal_retry;
	}
	if ((s2 = xbox_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		/*perror("socket2");*/
		xbox_close(s1);
		goto fatal_retry;
	}
	memset(&sa1, 0, sizeof(sa1));
	sa1.sin_family = AF_INET;
	sa1.sin_port = 0;
	sa1.sin_addr.s_addr = INADDR_ANY;
	if (xbox_bind(s1, (struct sockaddr *)&sa1, sizeof(sa1))) {
		/*perror("bind");*/
		clo:
		xbox_close(s1);
		xbox_close(s2);
		goto fatal_retry;
	}
	if (xbox_listen(s1, 1)) {
		/*perror("listen");*/
		goto clo;
	}
	sa1.sin_addr = XBNet_getLocalAddress();
	if (xbox_connect(s2, (struct sockaddr *)&sa1, sizeof(sa1))) {
		/*perror("connect");*/
		goto clo;
	}
	l = sizeof(sa2);
	if ((s3 = xbox_accept(s1, (struct sockaddr *)&sa2, &l)) < 0) {
		/*perror("accept");*/
		goto clo;
	}
	xbox_getsockname(s3, (struct sockaddr *)&sa1, &l);
	//if (sa1.sin_addr.s_addr != sa2.sin_addr.s_addr) {
	//	xbox_close(s3);
	//	goto clo;
	//}
	xbox_close(s1);
	fd[0] = s2;
	fd[1] = s3;
	return 0;

	fatal_retry:
	if (++retry_count > PIPE_RETRIES) return -1;
	sleep(1);
	goto again;
}


void xbox_set_fd_to_fdset(int fd, fd_set FAR *set)
{
  switch(xbox_get_fd_type(fd))
  {
    case FD_TYPE_SOCKET:
      FD_SET(xbox_get_socket(fd), set);
      break;
    default:
      break;
  }
}

inline void xbox_set_fdset(struct fd_set *in, struct fd_set *out, int fd, int *s, int type)
{
	XNDNS *dnsq;
	switch(xbox_get_fd_type(fd))
	{
		case FD_TYPE_SOCKET:
            if(FD_ISSET(xbox_get_socket(fd), in))
                FD_SET(fd, out);
			else
				FD_CLR(fd, out);
			break;
		case FD_TYPE_FILE:
			if(type == 3)
				break;
			FD_SET(fd, out);
			(*s)++;
			break;
		case FD_TYPE_DNSQUERY:
			if((dnsq = xbox_get_dnsquery(fd)) && (dnsq->iStatus == 0) && (type == 1))
			{
				FD_SET(fd, out);
				(*s)++;
			}
			else
				FD_CLR(fd, out);
			break;
	}
}

int xbox_select(int n, struct fd_set *rd, struct fd_set *wr, struct fd_set *exc, struct timeval *tm)
{
	int i, s;
	struct fd_set rr, rw, re;

	FD_ZERO(&rr);
	FD_ZERO(&rw);
	FD_ZERO(&re);

	/* Prepare fdsets for the "real" select() call. Only applicable to sockets */
	for (i = 0; i < n; i++)
	{
		rr.fd_array[i] = 0;
		rw.fd_array[i] = 0;
		re.fd_array[i] = 0;

		if( rd && rd->fd_array[i] != 0 && i < rd->fd_count && (xbox_get_fd_type(rd->fd_array[i]) == FD_TYPE_SOCKET) )
			FD_SET(xbox_get_socket(rd->fd_array[i]), &rr);
		if( wr && wr->fd_array[i] != 0 && i < wr->fd_count && (xbox_get_fd_type(wr->fd_array[i]) == FD_TYPE_SOCKET) )
			FD_SET(xbox_get_socket(wr->fd_array[i]), &rw);
		if( exc && exc->fd_array[i] != 0 && i < exc->fd_count && (xbox_get_fd_type(exc->fd_array[i]) == FD_TYPE_SOCKET) )
			FD_SET(xbox_get_socket(exc->fd_array[i]), &re);
	}

	if ((s = select(1, &rr, &rw, &re, tm)) < 0) {
		FD_ZERO(rd);
		return 0;
	}

	/* Ok, clear the provided fdsets */
/*	if(rd)	FD_ZERO(rd);
	if(wr)	FD_ZERO(wr);
	if(exc)	FD_ZERO(exc); */

	/* And fill'em! */
	if( rd )
		for (i = 0; i < n; i++)
			xbox_set_fdset(&rr, rd, i, &s, 1);
	if( wr )
		for (i = 0; i < n; i++)
			xbox_set_fdset(&rw, wr, i, &s, 2);
	if( exc )
		for (i = 0; i < n; i++)
			xbox_set_fdset(&re, exc, i, &s, 3);

	return s;
}

#ifndef SO_ERROR
#define SO_ERROR	10001
#endif

int xbox_getsockopt(int s, int level, int optname, void *optval, int *optlen)
{
	if (optname == SO_ERROR && *optlen >= sizeof(int)) {
		*(int *)optval = 0;
		*optlen = sizeof(int);
		return 0;
	}
	return -1;
}


int xbox_fcntl(int fd, int cmd, long arg)
{
	if( cmd == F_SETFL && arg == O_NONBLOCK )
		return set_nonblocking_fd( fd );

	return -1;
}

DIR *xbox_opendir(const char *name)
{
	DIR *d = (DIR *)malloc(sizeof(DIR));

	strncpy(d->name, name, MAX_PATH-3);


	if(strlen(name) > 0 && name[strlen(name)-1] == '\\')
		strcat(d->name, "*");
	else
		strcat(d->name, "\\*");

	d->ent.handle = FindFirstFile(d->name, &(d->ent.finddata));

	if(d->ent.handle == INVALID_HANDLE_VALUE)
	{
		closedir(d);
		return NULL;
	}
	else
	{
		d->first = 1;
		return d;
	}
}

int xbox_closedir(DIR *d)
{
	if(d->ent.handle)
		FindClose(d->ent.handle);

	free(d);

	return 0;
}

struct dirent *xbox_readdir(DIR *dir)
{
	if(!dir->first)
	{
		if(!FindNextFile(dir->ent.handle, &(dir->ent.finddata)))
			return NULL;
	}
	else
		dir->first = 0;
	
	dir->ent.d_name = dir->ent.finddata.cFileName;

	return &(dir->ent);
}

#endif