/* close replacement.
   Copyright (C) 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <unistd.h>

#if GNULIB_SYS_SOCKET
# define WIN32_LEAN_AND_MEAN
# include <sys/socket.h>
#endif

#if HAVE__GL_CLOSE_FD_MAYBE_SOCKET

/* Get set_winsock_errno, FD_TO_SOCKET etc. */
#include "w32sock.h"

static int
_gl_close_fd_maybe_socket (int fd)
{
  SOCKET sock = FD_TO_SOCKET (fd);
  WSANETWORKEVENTS ev;

  ev.lNetworkEvents = 0xDEADBEEF;
  WSAEnumNetworkEvents (sock, NULL, &ev);
  if (ev.lNetworkEvents != 0xDEADBEEF)
    {
      /* FIXME: other applications, like squid, use an undocumented
	 _free_osfhnd free function.  But this is not enough: The 'osfile'
	 flags for fd also needs to be cleared, but it is hard to access it.
	 Instead, here we just close twice the file descriptor.  */
      if (closesocket (sock))
	{
	  set_winsock_errno ();
	  return -1;
	}
      else
	{
	  /* This call frees the file descriptor and does a
	     CloseHandle ((HANDLE) _get_osfhandle (fd)), which fails.  */
	  _close (fd);
	  return 0;
	}
    }
  else
    return _close (fd);
}
#endif

/* Override close() to call into other gnulib modules.  */

int
rpl_close (int fd)
#undef close
{
#if HAVE__GL_CLOSE_FD_MAYBE_SOCKET
  int retval = _gl_close_fd_maybe_socket (fd);
#else
  int retval = close (fd);
#endif

#ifdef FCHDIR_REPLACEMENT
  if (retval >= 0)
    _gl_unregister_fd (fd);
#endif

  return retval;
}
