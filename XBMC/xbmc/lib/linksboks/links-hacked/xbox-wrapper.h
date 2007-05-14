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

#ifdef __XBOX__

#ifndef XBOX_WRAPPER_H
#define XBOX_WRAPPER_H

#define inline			__inline

#define strncasecmp		_strnicmp
#define strcasecmp		_stricmp


/* Xbox mapping of UNIX syscalls and al */
#define open			xbox_open
#define close			xbox_close
#define	read			xbox_read
#define write			xbox_write
#define socket			xbox_socket
#define connect			xbox_connect
#define listen			xbox_listen
#define bind			xbox_bind
#define fcntl			xbox_fcntl
#define c_pipe			xbox_pipe
#define getsockname		xbox_getsockname
#define accept			xbox_accept
#define select			xbox_select
#define opendir			xbox_opendir
#define readdir			xbox_readdir
#define closedir		xbox_closedir
/*#undef FD_SET
#define FD_SET(fd, set) xbox_set_fd_to_fdset((fd), (set)) */
/*#undef FD_ISSET
#define FD_ISSET(fd, set) ((xbox_get_fd_type(xbox_get_fd_type == FD_TYPE_SOCKET)) ? __WSAFDIsSet((SOCKET)(xbox_get_socket(fd)), (fd_set FAR *)(set)) : -1) //dirty
*/

#define getenv(x)		NULL

#define SHS	128


/* Winsockx equivalent of error codes */
#define EALREADY		WSAEALREADY
#define ECONNRESET		WSAECONNRESET
#define EINPROGRESS		WSAEINPROGRESS

#define O_RDONLY       0x0000  /* open for reading only */
#define O_WRONLY       0x0001  /* open for writing only */
#define O_RDWR         0x0002  /* open for reading and writing */
#define O_APPEND       0x0008  /* writes done at eof */

#define O_CREAT        0x0100  /* create and open file */
#define O_TRUNC        0x0200  /* open and truncate */
#define O_EXCL         0x0400  /* open only if file doesn't already exist */

#define O_TEXT         0x4000  /* file mode is text (translated) */
#define O_BINARY       0x8000  /* file mode is binary (untranslated) */

#define O_NONBLOCK        04000
#define O_NDELAY        O_NONBLOCK

#define O_NOCTTY			0

#define F_GETFD         	1       /* get close_on_exec */
#define F_SETFD         	2       /* set/clear close_on_exec */
#define F_GETFL         	3       /* get file->f_flags */
#define F_SETFL         	4       /* set file->f_flags */

#define FD_CLOEXEC			1

#ifndef SO_ERROR
#define SO_ERROR	10001
#endif

/* from include/linux/limits.h */
#define PIPE_BUF        4096    /* # bytes in atomic write to a pipe */


#define FD_TYPE_NONE		0
#define FD_TYPE_FILE		1
#define FD_TYPE_SOCKET		2
#define FD_TYPE_DNSQUERY	3


/* Dummy termios struct */
struct termios
{
	void *dummy;
};

struct dirent
{
	char *d_name;
	HANDLE handle;
	WIN32_FIND_DATA finddata;
};

typedef struct _dir
{
	char name[MAX_PATH];
	int first;
	struct dirent ent;
} DIR;

#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)


int XBNet_Init( BYTE cfgFlags );
struct in_addr XBNet_getLocalAddress( );



int xbox_get_fd_type( int fd );
SOCKET xbox_get_socket( int fd );
XNDNS *xbox_get_dnsquery( int fd );
int xbox_registersocket( SOCKET socket );
int xbox_registerfile( int file );
int xbox_registerdnsquery( XNDNS *query );


int xbox_open(const char *pathname, int flags);
int xbox_creat(const char *pathname, int mode);
int xbox_close(int fd);
int xbox_connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
int xbox_read(int fd, void *buf, int count);
int xbox_write(int fd, const void *buf, int count);
int xbox_socket(int domain, int type, int protocol);
int xbox_listen(int s, int backlog);
int xbox_bind(int sockfd, struct sockaddr *my_addr, int addrlen);
int xbox_fcntl(int fd, int cmd, long arg);
int xbox_getsockname(int s, struct sockaddr *name, int *namelen);
int xbox_accept(int sock, struct sockaddr *adresse, int *longueur);
void xbox_set_fd_to_fdset(int fd, fd_set FAR *set);
int xbox_select(int n, struct fd_set *rd, struct fd_set *wr, struct fd_set *exc, struct timeval *tm);

int sleep( int nb_sec );

DIR *xbox_opendir(const char *name);
struct dirent *xbox_readdir(DIR *dir);
int xbox_closedir(DIR *d);


#endif

#endif