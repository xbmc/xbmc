/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * giounix.c: IO Channels using unix file descriptors
 * Copyright 1998 Owen Taylor
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

#define _POSIX_SOURCE		/* for SSIZE_MAX */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "glib.h"
#include "galias.h"

/*
 * Unix IO Channels
 */

typedef struct _GIOUnixChannel GIOUnixChannel;
typedef struct _GIOUnixWatch GIOUnixWatch;

struct _GIOUnixChannel
{
  GIOChannel channel;
  gint fd;
};

struct _GIOUnixWatch
{
  GSource       source;
  GPollFD       pollfd;
  GIOChannel   *channel;
  GIOCondition  condition;
};


static GIOStatus	g_io_unix_read		(GIOChannel   *channel,
						 gchar        *buf,
						 gsize         count,
						 gsize        *bytes_read,
						 GError      **err);
static GIOStatus	g_io_unix_write		(GIOChannel   *channel,
						 const gchar  *buf,
						 gsize         count,
						 gsize        *bytes_written,
						 GError      **err);
static GIOStatus	g_io_unix_seek		(GIOChannel   *channel,
						 gint64        offset,
						 GSeekType     type,
						 GError      **err);
static GIOStatus	g_io_unix_close		(GIOChannel   *channel,
						 GError      **err);
static void		g_io_unix_free		(GIOChannel   *channel);
static GSource*		g_io_unix_create_watch	(GIOChannel   *channel,
						 GIOCondition  condition);
static GIOStatus	g_io_unix_set_flags	(GIOChannel   *channel,
                       				 GIOFlags      flags,
						 GError      **err);
static GIOFlags 	g_io_unix_get_flags	(GIOChannel   *channel);

static gboolean g_io_unix_prepare  (GSource     *source,
				    gint        *timeout);
static gboolean g_io_unix_check    (GSource     *source);
static gboolean g_io_unix_dispatch (GSource     *source,
				    GSourceFunc  callback,
				    gpointer     user_data);
static void     g_io_unix_finalize (GSource     *source);

GSourceFuncs g_io_watch_funcs = {
  g_io_unix_prepare,
  g_io_unix_check,
  g_io_unix_dispatch,
  g_io_unix_finalize
};

static GIOFuncs unix_channel_funcs = {
  g_io_unix_read,
  g_io_unix_write,
  g_io_unix_seek,
  g_io_unix_close,
  g_io_unix_create_watch,
  g_io_unix_free,
  g_io_unix_set_flags,
  g_io_unix_get_flags,
};

static gboolean 
g_io_unix_prepare (GSource  *source,
		   gint     *timeout)
{
  GIOUnixWatch *watch = (GIOUnixWatch *)source;
  GIOCondition buffer_condition = g_io_channel_get_buffer_condition (watch->channel);

  *timeout = -1;

  /* Only return TRUE here if _all_ bits in watch->condition will be set
   */
  return ((watch->condition & buffer_condition) == watch->condition);
}

static gboolean 
g_io_unix_check (GSource  *source)
{
  GIOUnixWatch *watch = (GIOUnixWatch *)source;
  GIOCondition buffer_condition = g_io_channel_get_buffer_condition (watch->channel);
  GIOCondition poll_condition = watch->pollfd.revents;

  return ((poll_condition | buffer_condition) & watch->condition);
}

static gboolean
g_io_unix_dispatch (GSource     *source,
		    GSourceFunc  callback,
		    gpointer     user_data)

{
  GIOFunc func = (GIOFunc)callback;
  GIOUnixWatch *watch = (GIOUnixWatch *)source;
  GIOCondition buffer_condition = g_io_channel_get_buffer_condition (watch->channel);

  if (!func)
    {
      g_warning ("IO watch dispatched without callback\n"
		 "You must call g_source_connect().");
      return FALSE;
    }
  
  return (*func) (watch->channel,
		  (watch->pollfd.revents | buffer_condition) & watch->condition,
		  user_data);
}

static void 
g_io_unix_finalize (GSource *source)
{
  GIOUnixWatch *watch = (GIOUnixWatch *)source;

  g_io_channel_unref (watch->channel);
}

static GIOStatus
g_io_unix_read (GIOChannel *channel, 
		gchar      *buf, 
		gsize       count,
		gsize      *bytes_read,
		GError    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  gssize result;

  if (count > SSIZE_MAX) /* At least according to the Debian manpage for read */
    count = SSIZE_MAX;

 retry:
  result = read (unix_channel->fd, buf, count);

  if (result < 0)
    {
      int errsv = errno;
      *bytes_read = 0;

      switch (errsv)
        {
#ifdef EINTR
          case EINTR:
            goto retry;
#endif
#ifdef EAGAIN
          case EAGAIN:
            return G_IO_STATUS_AGAIN;
#endif
          default:
            g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                                 g_io_channel_error_from_errno (errsv),
                                 g_strerror (errsv));
            return G_IO_STATUS_ERROR;
        }
    }

  *bytes_read = result;

  return (result > 0) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF;
}

static GIOStatus
g_io_unix_write (GIOChannel  *channel, 
		 const gchar *buf, 
		 gsize       count,
		 gsize      *bytes_written,
		 GError    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  gssize result;

 retry:
  result = write (unix_channel->fd, buf, count);

  if (result < 0)
    {
      int errsv = errno;
      *bytes_written = 0;

      switch (errsv)
        {
#ifdef EINTR
          case EINTR:
            goto retry;
#endif
#ifdef EAGAIN
          case EAGAIN:
            return G_IO_STATUS_AGAIN;
#endif
          default:
            g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                                 g_io_channel_error_from_errno (errsv),
                                 g_strerror (errsv));
            return G_IO_STATUS_ERROR;
        }
    }

  *bytes_written = result;

  return G_IO_STATUS_NORMAL;
}

static GIOStatus
g_io_unix_seek (GIOChannel *channel,
		gint64      offset, 
		GSeekType   type,
                GError    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  int whence;
  off_t tmp_offset;
  off_t result;

  switch (type)
    {
    case G_SEEK_SET:
      whence = SEEK_SET;
      break;
    case G_SEEK_CUR:
      whence = SEEK_CUR;
      break;
    case G_SEEK_END:
      whence = SEEK_END;
      break;
    default:
      whence = -1; /* Shut the compiler up */
      g_assert_not_reached ();
    }

  tmp_offset = offset;
  if (tmp_offset != offset)
    {
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (EINVAL),
                           g_strerror (EINVAL));
      return G_IO_STATUS_ERROR;
    }
  
  result = lseek (unix_channel->fd, tmp_offset, whence);

  if (result < 0)
    {
      int errsv = errno;
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (errsv),
                           g_strerror (errsv));
      return G_IO_STATUS_ERROR;
    }

  return G_IO_STATUS_NORMAL;
}


static GIOStatus
g_io_unix_close (GIOChannel *channel,
		 GError    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;

  if (close (unix_channel->fd) < 0)
    {
      int errsv = errno;
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (errsv),
                           g_strerror (errsv));
      return G_IO_STATUS_ERROR;
    }

  return G_IO_STATUS_NORMAL;
}

static void 
g_io_unix_free (GIOChannel *channel)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;

  g_free (unix_channel);
}

static GSource *
g_io_unix_create_watch (GIOChannel   *channel,
			GIOCondition  condition)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  GSource *source;
  GIOUnixWatch *watch;


  source = g_source_new (&g_io_watch_funcs, sizeof (GIOUnixWatch));
  watch = (GIOUnixWatch *)source;
  
  watch->channel = channel;
  g_io_channel_ref (channel);
  
  watch->condition = condition;

  watch->pollfd.fd = unix_channel->fd;
  watch->pollfd.events = condition;

  g_source_add_poll (source, &watch->pollfd);

  return source;
}

static GIOStatus
g_io_unix_set_flags (GIOChannel *channel,
                     GIOFlags    flags,
                     GError    **err)
{
  glong fcntl_flags;
  GIOUnixChannel *unix_channel = (GIOUnixChannel *) channel;

  fcntl_flags = 0;

  if (flags & G_IO_FLAG_APPEND)
    fcntl_flags |= O_APPEND;
  if (flags & G_IO_FLAG_NONBLOCK)
#ifdef O_NONBLOCK
    fcntl_flags |= O_NONBLOCK;
#else
    fcntl_flags |= O_NDELAY;
#endif

  if (fcntl (unix_channel->fd, F_SETFL, fcntl_flags) == -1)
    {
      int errsv = errno;
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (errsv),
                           g_strerror (errsv));
      return G_IO_STATUS_ERROR;
    }

  return G_IO_STATUS_NORMAL;
}

static GIOFlags
g_io_unix_get_flags (GIOChannel *channel)
{
  GIOFlags flags = 0;
  glong fcntl_flags;
  GIOUnixChannel *unix_channel = (GIOUnixChannel *) channel;

  fcntl_flags = fcntl (unix_channel->fd, F_GETFL);

  if (fcntl_flags == -1)
    {
      int err = errno;
      g_warning (G_STRLOC "Error while getting flags for FD: %s (%d)\n",
		 g_strerror (err), err);
      return 0;
    }

  if (fcntl_flags & O_APPEND)
    flags |= G_IO_FLAG_APPEND;
#ifdef O_NONBLOCK
  if (fcntl_flags & O_NONBLOCK)
#else
  if (fcntl_flags & O_NDELAY)
#endif
    flags |= G_IO_FLAG_NONBLOCK;

  switch (fcntl_flags & (O_RDONLY | O_WRONLY | O_RDWR))
    {
      case O_RDONLY:
        channel->is_readable = TRUE;
        channel->is_writeable = FALSE;
        break;
      case O_WRONLY:
        channel->is_readable = FALSE;
        channel->is_writeable = TRUE;
        break;
      case O_RDWR:
        channel->is_readable = TRUE;
        channel->is_writeable = TRUE;
        break;
      default:
        g_assert_not_reached ();
    }

  return flags;
}

GIOChannel *
g_io_channel_new_file (const gchar *filename,
                       const gchar *mode,
                       GError     **error)
{
  int fid, flags;
  mode_t create_mode;
  GIOChannel *channel;
  enum { /* Cheesy hack */
    MODE_R = 1 << 0,
    MODE_W = 1 << 1,
    MODE_A = 1 << 2,
    MODE_PLUS = 1 << 3
  } mode_num;
  struct stat buffer;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (mode != NULL, NULL);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  switch (mode[0])
    {
      case 'r':
        mode_num = MODE_R;
        break;
      case 'w':
        mode_num = MODE_W;
        break;
      case 'a':
        mode_num = MODE_A;
        break;
      default:
        g_warning ("Invalid GIOFileMode %s.\n", mode);
        return NULL;
    }

  switch (mode[1])
    {
      case '\0':
        break;
      case '+':
        if (mode[2] == '\0')
          {
            mode_num |= MODE_PLUS;
            break;
          }
        /* Fall through */
      default:
        g_warning ("Invalid GIOFileMode %s.\n", mode);
        return NULL;
    }

  switch (mode_num)
    {
      case MODE_R:
        flags = O_RDONLY;
        break;
      case MODE_W:
        flags = O_WRONLY | O_TRUNC | O_CREAT;
        break;
      case MODE_A:
        flags = O_WRONLY | O_APPEND | O_CREAT;
        break;
      case MODE_R | MODE_PLUS:
        flags = O_RDWR;
        break;
      case MODE_W | MODE_PLUS:
        flags = O_RDWR | O_TRUNC | O_CREAT;
        break;
      case MODE_A | MODE_PLUS:
        flags = O_RDWR | O_APPEND | O_CREAT;
        break;
      default:
        g_assert_not_reached ();
        flags = 0;
    }

  create_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  fid = open (filename, flags, create_mode);
  if (fid == -1)
    {
      int err = errno;
      g_set_error_literal (error, G_FILE_ERROR,
                           g_file_error_from_errno (err),
                           g_strerror (err));
      return (GIOChannel *)NULL;
    }

  if (fstat (fid, &buffer) == -1) /* In case someone opens a FIFO */
    {
      int err = errno;
      close (fid);
      g_set_error_literal (error, G_FILE_ERROR,
                           g_file_error_from_errno (err),
                           g_strerror (err));
      return (GIOChannel *)NULL;
    }

  channel = (GIOChannel *) g_new (GIOUnixChannel, 1);

  channel->is_seekable = S_ISREG (buffer.st_mode) || S_ISCHR (buffer.st_mode)
                         || S_ISBLK (buffer.st_mode);

  switch (mode_num)
    {
      case MODE_R:
        channel->is_readable = TRUE;
        channel->is_writeable = FALSE;
        break;
      case MODE_W:
      case MODE_A:
        channel->is_readable = FALSE;
        channel->is_writeable = TRUE;
        break;
      case MODE_R | MODE_PLUS:
      case MODE_W | MODE_PLUS:
      case MODE_A | MODE_PLUS:
        channel->is_readable = TRUE;
        channel->is_writeable = TRUE;
        break;
      default:
        g_assert_not_reached ();
    }

  g_io_channel_init (channel);
  channel->close_on_unref = TRUE; /* must be after g_io_channel_init () */
  channel->funcs = &unix_channel_funcs;

  ((GIOUnixChannel *) channel)->fd = fid;
  return channel;
}

GIOChannel *
g_io_channel_unix_new (gint fd)
{
  struct stat buffer;
  GIOUnixChannel *unix_channel = g_new (GIOUnixChannel, 1);
  GIOChannel *channel = (GIOChannel *)unix_channel;

  g_io_channel_init (channel);
  channel->funcs = &unix_channel_funcs;

  unix_channel->fd = fd;

  /* I'm not sure if fstat on a non-file (e.g., socket) works
   * it should be safe to say if it fails, the fd isn't seekable.
   */
  /* Newer UNIX versions support S_ISSOCK(), fstat() will probably
   * succeed in most cases.
   */
  if (fstat (unix_channel->fd, &buffer) == 0)
    channel->is_seekable = S_ISREG (buffer.st_mode) || S_ISCHR (buffer.st_mode)
                           || S_ISBLK (buffer.st_mode);
  else /* Assume not seekable */
    channel->is_seekable = FALSE;

  g_io_unix_get_flags (channel); /* Sets is_readable, is_writeable */

  return channel;
}

gint
g_io_channel_unix_get_fd (GIOChannel *channel)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  return unix_channel->fd;
}

#define __G_IO_UNIX_C__
#include "galiasdef.c"
