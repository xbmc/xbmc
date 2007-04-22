/* os_depx.h
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#ifdef HAVE_VALUES_H
#include <values.h>
#endif

#ifndef MAXINT
#ifdef INT_MAX
#define MAXINT INT_MAX
#else
#define MAXINT 0x7fffffff
#endif
#endif

#ifndef SA_RESTART
#define SA_RESTART	0
#endif

/*#ifdef sparc
#define htons(x) (x)
#endif*/

#ifndef HAVE_CFMAKERAW
void cfmakeraw(struct termios *t);
#endif

#ifdef __EMX__
#define _stricmp stricmp
#define read _read
#define write _write
#endif

#ifdef BEOS
#define socket be_socket
#define connect be_connect
#define getpeername be_getpeername
#define getsockname be_getsockname
#define listen be_listen
#define accept be_accept
#define bind be_bind
#define pipe be_pipe
#define read be_read
#define write be_write
#define close be_close
#define select be_select
#define getsockopt be_getsockopt
#ifndef PF_INET
#define PF_INET AF_INET
#endif
#ifndef SO_ERROR
#define SO_ERROR	10001
#endif
#ifdef errno
#undef errno
#endif
#define errno 1
#endif

#ifdef XBOX
#include "xbox-wrapper.h"
#endif

#ifdef GRDRV_SVGALIB
#define select vga_select
int vga_select(int  n,  fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		              struct timeval *timeout);
#endif /* #ifdef GRDRV_SVGALIB */

#ifdef GRDRV_ATHEOS
#define select ath_select
int ath_select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t);
#endif

