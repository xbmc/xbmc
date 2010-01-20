/* gpoll.h - poll(2) support
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if !defined (__GLIB_H_INSIDE__) && !defined (__G_MAIN_H__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_POLL_H__
#define __G_POLL_H__

#include <glib/gtypes.h>

G_BEGIN_DECLS

/* Any definitions using GPollFD or GPollFunc are primarily
 * for Unix and not guaranteed to be the compatible on all
 * operating systems on which GLib runs. Right now, the
 * GLib does use these functions on Win32 as well, but interprets
 * them in a fairly different way than on Unix. If you use
 * these definitions, you are should be prepared to recode
 * for different operating systems.
 *
 * Note that on systems with a working poll(2), that function is used
 * in place of g_poll(). Thus g_poll() must have the same signature as
 * poll(), meaning GPollFD must have the same layout as struct pollfd.
 *
 *
 * On Win32, the fd in a GPollFD should be Win32 HANDLE (*not* a file
 * descriptor as provided by the C runtime) that can be used by
 * MsgWaitForMultipleObjects. This does *not* include file handles
 * from CreateFile, SOCKETs, nor pipe handles. (But you can use
 * WSAEventSelect to signal events when a SOCKET is readable).
 *
 * On Win32, fd can also be the special value G_WIN32_MSG_HANDLE to
 * indicate polling for messages.
 *
 * But note that G_WIN32_MSG_HANDLE GPollFDs should not be used by GDK
 * (GTK) programs, as GDK itself wants to read messages and convert them
 * to GDK events.
 *
 * So, unless you really know what you are doing, it's best not to try
 * to use the main loop polling stuff for your own needs on
 * Windows.
 */
typedef struct _GPollFD GPollFD;
typedef gint	(*GPollFunc)	(GPollFD *ufds,
				 guint	  nfsd,
				 gint     timeout_);

struct _GPollFD
{
#if defined (G_OS_WIN32) && GLIB_SIZEOF_VOID_P == 8
  gint64	fd;
#else
  gint		fd;
#endif
  gushort 	events;
  gushort 	revents;
};

#ifdef G_OS_WIN32
#if GLIB_SIZEOF_VOID_P == 8
#define G_POLLFD_FORMAT "%#I64x"
#else
#define G_POLLFD_FORMAT "%#x"
#endif
#else
#define G_POLLFD_FORMAT "%d"
#endif

gint g_poll (GPollFD *fds,
	     guint    nfds,
	     gint     timeout);

G_END_DECLS

#endif /* __G_POLL_H__ */
