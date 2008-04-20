/* -*- c-file-style: "linux" -*-
 *
 * Try creating a Unix-domain socket, opening it, and reading from it.
 * The POSIX name for these is AF_LOCAL/PF_LOCAL.
 *
 * This is used by the Samba autoconf scripts to detect systems which
 * don't have Unix-domain sockets, such as (probably) VMS, or systems
 * on which they are broken under some conditions, such as RedHat 7.0
 * (unpatched).  We can't build WinBind there at the moment.
 *
 * Coding standard says to always use exit() for this, not return, so
 * we do.
 *
 * Martin Pool <mbp@samba.org>, June 2000. */

/* TODO: Look for AF_LOCAL (most standard), AF_UNIX, and AF_FILE. */

#include <stdio.h>

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

#ifdef HAVE_SYS_UN_H
#  include <sys/un.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if HAVE_ERRNO_DECL
# include <errno.h>
#else
extern int errno;
#endif

static int bind_socket(char const *filename)
{
	int sock_fd;
	struct sockaddr_un name;
	size_t size;
	
	/* Create the socket. */
	if ((sock_fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
		perror ("socket(PF_LOCAL, SOCK_STREAM)");
		exit(1);
	}
     
	/* Bind a name to the socket. */
	name.sun_family = AF_LOCAL;
	strncpy(name.sun_path, filename, sizeof (name.sun_path));
     
       /* The size of the address is
          the offset of the start of the filename,
          plus its length,
          plus one for the terminating null byte.
          Alternatively you can just do:
          size = SUN_LEN (&name);
      */
	size = SUN_LEN(&name);
	/* XXX: This probably won't work on unfriendly libcs */
     
	if (bind(sock_fd, (struct sockaddr *) &name, size) < 0) {
		perror ("bind");
		exit(1);
	}

	return sock_fd;
}


int main(void)
{
	int sock_fd;
	int kid;
	char const *filename = "conftest.unixsock.sock";

	/* abolish hanging */
	alarm(15);		/* secs */

	if ((sock_fd = bind_socket(filename)) < 0)
		exit(1);

	/* the socket will be deleted when autoconf cleans up these
           files. */

	exit(0);
}
