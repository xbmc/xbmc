/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gpoll.c: poll(2) abstraction
 * Copyright 1998 Owen Taylor
 * Copyright 2008 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"

/* Uncomment the next line (and the corresponding line in gmain.c) to
 * enable debugging printouts if the environment variable
 * G_MAIN_POLL_DEBUG is set to some value.
 */
/* #define G_MAIN_POLL_DEBUG */

#ifdef _WIN32
/* Always enable debugging printout on Windows, as it is more often
 * needed there...
 */
#define G_MAIN_POLL_DEBUG
#endif

#include "glib.h"
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef GLIB_HAVE_SYS_POLL_H
#  include <sys/poll.h>
#  undef events	 /* AIX 4.1.5 & 4.3.2 define this for SVR3,4 compatibility */
#  undef revents /* AIX 4.1.5 & 4.3.2 define this for SVR3,4 compatibility */

/* The poll() emulation on OS/X doesn't handle fds=NULL, nfds=0,
 * so we prefer our own poll emulation.
 */
#if defined(_POLL_EMUL_H_) || defined(BROKEN_POLL)
#undef HAVE_POLL
#endif

#endif /* GLIB_HAVE_SYS_POLL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif /* G_OS_WIN32 */

#include "galias.h"

#ifdef G_MAIN_POLL_DEBUG
extern gboolean _g_main_poll_debug;
#endif

#ifdef HAVE_POLL
/* SunOS has poll, but doesn't provide a prototype. */
#  if defined (sun) && !defined (__SVR4)
extern gint poll (struct pollfd *fds, guint nfsd, gint timeout);
#  endif  /* !sun */

/**
 * g_poll:
 * @fds: file descriptors to poll
 * @nfds: the number of file descriptors in @fds
 * @timeout: amount of time to wait, in milliseconds, or -1 to wait forever
 *
 * Polls @fds, as with the poll() system call, but portably. (On
 * systems that don't have poll(), it is emulated using select().)
 * This is used internally by #GMainContext, but it can be called
 * directly if you need to block until a file descriptor is ready, but
 * don't want to run the full main loop.
 *
 * Each element of @fds is a #GPollFD describing a single file
 * descriptor to poll. The %fd field indicates the file descriptor,
 * and the %events field indicates the events to poll for. On return,
 * the %revents fields will be filled with the events that actually
 * occurred.
 *
 * On POSIX systems, the file descriptors in @fds can be any sort of
 * file descriptor, but the situation is much more complicated on
 * Windows. If you need to use g_poll() in code that has to run on
 * Windows, the easiest solution is to construct all of your
 * #GPollFD<!-- -->s with g_io_channel_win32_make_pollfd().
 *
 * Return value: the number of entries in @fds whose %revents fields
 * were filled in, or 0 if the operation timed out, or -1 on error or
 * if the call was interrupted.
 *
 * Since: 2.20
 **/
gint
g_poll (GPollFD *fds,
	guint    nfds,
	gint     timeout)
{
  return poll ((struct pollfd *)fds, nfds, timeout);
}

#else	/* !HAVE_POLL */

#ifdef G_OS_WIN32

static int
poll_rest (gboolean  poll_msgs,
	   HANDLE   *handles,
	   gint      nhandles,
	   GPollFD  *fds,
	   guint     nfds,
	   gint      timeout)
{
  DWORD ready;
  GPollFD *f;
  int recursed_result;

  if (poll_msgs)
    {
      /* Wait for either messages or handles
       * -> Use MsgWaitForMultipleObjectsEx
       */
      if (_g_main_poll_debug)
	g_print ("  MsgWaitForMultipleObjectsEx(%d, %d)\n", nhandles, timeout);

      ready = MsgWaitForMultipleObjectsEx (nhandles, handles, timeout,
					   QS_ALLINPUT, MWMO_ALERTABLE);

      if (ready == WAIT_FAILED)
	{
	  gchar *emsg = g_win32_error_message (GetLastError ());
	  g_warning ("MsgWaitForMultipleObjectsEx failed: %s", emsg);
	  g_free (emsg);
	}
    }
  else if (nhandles == 0)
    {
      /* No handles to wait for, just the timeout */
      if (timeout == INFINITE)
	ready = WAIT_FAILED;
      else
	{
	  SleepEx (timeout, TRUE);
	  ready = WAIT_TIMEOUT;
	}
    }
  else
    {
      /* Wait for just handles
       * -> Use WaitForMultipleObjectsEx
       */
      if (_g_main_poll_debug)
	g_print ("  WaitForMultipleObjectsEx(%d, %d)\n", nhandles, timeout);

      ready = WaitForMultipleObjectsEx (nhandles, handles, FALSE, timeout, TRUE);
      if (ready == WAIT_FAILED)
	{
	  gchar *emsg = g_win32_error_message (GetLastError ());
	  g_warning ("WaitForMultipleObjectsEx failed: %s", emsg);
	  g_free (emsg);
	}
    }

  if (_g_main_poll_debug)
    g_print ("  wait returns %ld%s\n",
	     ready,
	     (ready == WAIT_FAILED ? " (WAIT_FAILED)" :
	      (ready == WAIT_TIMEOUT ? " (WAIT_TIMEOUT)" :
	       (poll_msgs && ready == WAIT_OBJECT_0 + nhandles ? " (msg)" : ""))));

  if (ready == WAIT_FAILED)
    return -1;
  else if (ready == WAIT_TIMEOUT ||
	   ready == WAIT_IO_COMPLETION)
    return 0;
  else if (poll_msgs && ready == WAIT_OBJECT_0 + nhandles)
    {
      for (f = fds; f < &fds[nfds]; ++f)
	if (f->fd == G_WIN32_MSG_HANDLE && f->events & G_IO_IN)
	  f->revents |= G_IO_IN;

      /* If we have a timeout, or no handles to poll, be satisfied
       * with just noticing we have messages waiting.
       */
      if (timeout != 0 || nhandles == 0)
	return 1;

      /* If no timeout and handles to poll, recurse to poll them,
       * too.
       */
      recursed_result = poll_rest (FALSE, handles, nhandles, fds, nfds, 0);
      return (recursed_result == -1) ? -1 : 1 + recursed_result;
    }
  else if (ready >= WAIT_OBJECT_0 && ready < WAIT_OBJECT_0 + nhandles)
    {
      for (f = fds; f < &fds[nfds]; ++f)
	{
	  if ((HANDLE) f->fd == handles[ready - WAIT_OBJECT_0])
	    {
	      f->revents = f->events;
	      if (_g_main_poll_debug)
		g_print ("  got event %p\n", (HANDLE) f->fd);
	    }
	}

      /* If no timeout and polling several handles, recurse to poll
       * the rest of them.
       */
      if (timeout == 0 && nhandles > 1)
	{
	  /* Remove the handle that fired */
	  int i;
	  if (ready < nhandles - 1)
	    for (i = ready - WAIT_OBJECT_0 + 1; i < nhandles; i++)
	      handles[i-1] = handles[i];
	  nhandles--;
	  recursed_result = poll_rest (FALSE, handles, nhandles, fds, nfds, 0);
	  return (recursed_result == -1) ? -1 : 1 + recursed_result;
	}
      return 1;
    }

  return 0;
}

gint
g_poll (GPollFD *fds,
	guint    nfds,
	gint     timeout)
{
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  gboolean poll_msgs = FALSE;
  GPollFD *f;
  gint nhandles = 0;
  int retval;

  if (_g_main_poll_debug)
    g_print ("g_poll: waiting for");

  for (f = fds; f < &fds[nfds]; ++f)
    if (f->fd == G_WIN32_MSG_HANDLE && (f->events & G_IO_IN))
      {
	if (_g_main_poll_debug && !poll_msgs)
	  g_print (" MSG");
	poll_msgs = TRUE;
      }
    else if (f->fd > 0)
      {
	/* Don't add the same handle several times into the array, as
	 * docs say that is not allowed, even if it actually does seem
	 * to work.
	 */
	gint i;

	for (i = 0; i < nhandles; i++)
	  if (handles[i] == (HANDLE) f->fd)
	    break;

	if (i == nhandles)
	  {
	    if (nhandles == MAXIMUM_WAIT_OBJECTS)
	      {
		g_warning ("Too many handles to wait for!\n");
		break;
	      }
	    else
	      {
		if (_g_main_poll_debug)
		  g_print (" %p", (HANDLE) f->fd);
		handles[nhandles++] = (HANDLE) f->fd;
	      }
	  }
      }

  if (_g_main_poll_debug)
    g_print ("\n");

  for (f = fds; f < &fds[nfds]; ++f)
    f->revents = 0;

  if (timeout == -1)
    timeout = INFINITE;

  /* Polling for several things? */
  if (nhandles > 1 || (nhandles > 0 && poll_msgs))
    {
      /* First check if one or several of them are immediately
       * available
       */
      retval = poll_rest (poll_msgs, handles, nhandles, fds, nfds, 0);

      /* If not, and we have a significant timeout, poll again with
       * timeout then. Note that this will return indication for only
       * one event, or only for messages. We ignore timeouts less than
       * ten milliseconds as they are mostly pointless on Windows, the
       * MsgWaitForMultipleObjectsEx() call will timeout right away
       * anyway.
       */
      if (retval == 0 && (timeout == INFINITE || timeout >= 10))
	retval = poll_rest (poll_msgs, handles, nhandles, fds, nfds, timeout);
    }
  else
    {
      /* Just polling for one thing, so no need to check first if
       * available immediately
       */
      retval = poll_rest (poll_msgs, handles, nhandles, fds, nfds, timeout);
    }

  if (retval == -1)
    for (f = fds; f < &fds[nfds]; ++f)
      f->revents = 0;

  return retval;
}

#else  /* !G_OS_WIN32 */

/* The following implementation of poll() comes from the GNU C Library.
 * Copyright (C) 1994, 1996, 1997 Free Software Foundation, Inc.
 */

#include <string.h> /* for bzero on BSD systems */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#ifdef G_OS_BEOS
#undef NO_FD_SET
#endif /* G_OS_BEOS */

#ifndef NO_FD_SET
#  define SELECT_MASK fd_set
#else /* !NO_FD_SET */
#  ifndef _AIX
typedef long fd_mask;
#  endif /* _AIX */
#  ifdef _IBMR2
#    define SELECT_MASK void
#  else /* !_IBMR2 */
#    define SELECT_MASK int
#  endif /* !_IBMR2 */
#endif /* !NO_FD_SET */

gint
g_poll (GPollFD *fds,
	guint    nfds,
	gint     timeout)
{
  struct timeval tv;
  SELECT_MASK rset, wset, xset;
  GPollFD *f;
  int ready;
  int maxfd = 0;

  FD_ZERO (&rset);
  FD_ZERO (&wset);
  FD_ZERO (&xset);

  for (f = fds; f < &fds[nfds]; ++f)
    if (f->fd >= 0)
      {
	if (f->events & G_IO_IN)
	  FD_SET (f->fd, &rset);
	if (f->events & G_IO_OUT)
	  FD_SET (f->fd, &wset);
	if (f->events & G_IO_PRI)
	  FD_SET (f->fd, &xset);
	if (f->fd > maxfd && (f->events & (G_IO_IN|G_IO_OUT|G_IO_PRI)))
	  maxfd = f->fd;
      }

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;

  ready = select (maxfd + 1, &rset, &wset, &xset,
		  timeout == -1 ? NULL : &tv);
  if (ready > 0)
    for (f = fds; f < &fds[nfds]; ++f)
      {
	f->revents = 0;
	if (f->fd >= 0)
	  {
	    if (FD_ISSET (f->fd, &rset))
	      f->revents |= G_IO_IN;
	    if (FD_ISSET (f->fd, &wset))
	      f->revents |= G_IO_OUT;
	    if (FD_ISSET (f->fd, &xset))
	      f->revents |= G_IO_PRI;
	  }
      }

  return ready;
}

#endif /* !G_OS_WIN32 */

#endif	/* !HAVE_POLL */

#define __G_POLL_C__
#include "galiasdef.c"
