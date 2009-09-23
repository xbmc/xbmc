/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * giochannel.c: IO Channel abstraction
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

#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#undef G_DISABLE_DEPRECATED

#include "glib.h"

#include "giochannel.h"

#include "glibintl.h"

#include "galias.h"

#define G_IO_NICE_BUF_SIZE	1024

/* This needs to be as wide as the largest character in any possible encoding */
#define MAX_CHAR_SIZE		10

/* Some simplifying macros, which reduce the need to worry whether the
 * buffers have been allocated. These also make USE_BUF () an lvalue,
 * which is used in g_io_channel_read_to_end ().
 */
#define USE_BUF(channel)	((channel)->encoding ? (channel)->encoded_read_buf \
				 : (channel)->read_buf)
#define BUF_LEN(string)		((string) ? (string)->len : 0)

static GIOError		g_io_error_get_from_g_error	(GIOStatus    status,
							 GError      *err);
static void		g_io_channel_purge		(GIOChannel  *channel);
static GIOStatus	g_io_channel_fill_buffer	(GIOChannel  *channel,
							 GError     **err);
static GIOStatus	g_io_channel_read_line_backend	(GIOChannel  *channel,
							 gsize       *length,
							 gsize       *terminator_pos,
							 GError     **error);

/**
 * g_io_channel_init:
 * @channel: a #GIOChannel
 *
 * Initializes a #GIOChannel struct. 
 *
 * This is called by each of the above functions when creating a 
 * #GIOChannel, and so is not often needed by the application 
 * programmer (unless you are creating a new type of #GIOChannel).
 */
void
g_io_channel_init (GIOChannel *channel)
{
  channel->ref_count = 1;
  channel->encoding = g_strdup ("UTF-8");
  channel->line_term = NULL;
  channel->line_term_len = 0;
  channel->buf_size = G_IO_NICE_BUF_SIZE;
  channel->read_cd = (GIConv) -1;
  channel->write_cd = (GIConv) -1;
  channel->read_buf = NULL; /* Lazy allocate buffers */
  channel->encoded_read_buf = NULL;
  channel->write_buf = NULL;
  channel->partial_write_buf[0] = '\0';
  channel->use_buffer = TRUE;
  channel->do_encode = FALSE;
  channel->close_on_unref = FALSE;
}

/**
 * g_io_channel_ref:
 * @channel: a #GIOChannel
 *
 * Increments the reference count of a #GIOChannel.
 *
 * Returns: the @channel that was passed in (since 2.6)
 */
GIOChannel *
g_io_channel_ref (GIOChannel *channel)
{
  g_return_val_if_fail (channel != NULL, NULL);

  g_atomic_int_inc (&channel->ref_count);

  return channel;
}

/**
 * g_io_channel_unref:
 * @channel: a #GIOChannel
 *
 * Decrements the reference count of a #GIOChannel.
 */
void 
g_io_channel_unref (GIOChannel *channel)
{
  gboolean is_zero;

  g_return_if_fail (channel != NULL);

  is_zero = g_atomic_int_dec_and_test (&channel->ref_count);

  if (G_UNLIKELY (is_zero))
    {
      if (channel->close_on_unref)
        g_io_channel_shutdown (channel, TRUE, NULL);
      else
        g_io_channel_purge (channel);
      g_free (channel->encoding);
      if (channel->read_cd != (GIConv) -1)
        g_iconv_close (channel->read_cd);
      if (channel->write_cd != (GIConv) -1)
        g_iconv_close (channel->write_cd);
      g_free (channel->line_term);
      if (channel->read_buf)
        g_string_free (channel->read_buf, TRUE);
      if (channel->write_buf)
        g_string_free (channel->write_buf, TRUE);
      if (channel->encoded_read_buf)
        g_string_free (channel->encoded_read_buf, TRUE);
      channel->funcs->io_free (channel);
    }
}

static GIOError
g_io_error_get_from_g_error (GIOStatus  status,
			     GError    *err)
{
  switch (status)
    {
      case G_IO_STATUS_NORMAL:
      case G_IO_STATUS_EOF:
        return G_IO_ERROR_NONE;
      case G_IO_STATUS_AGAIN:
        return G_IO_ERROR_AGAIN;
      case G_IO_STATUS_ERROR:
	g_return_val_if_fail (err != NULL, G_IO_ERROR_UNKNOWN);
	
        if (err->domain != G_IO_CHANNEL_ERROR)
          return G_IO_ERROR_UNKNOWN;
        switch (err->code)
          {
            case G_IO_CHANNEL_ERROR_INVAL:
              return G_IO_ERROR_INVAL;
            default:
              return G_IO_ERROR_UNKNOWN;
          }
      default:
        g_assert_not_reached ();
    }
}

/**
 * g_io_channel_read:
 * @channel: a #GIOChannel
 * @buf: a buffer to read the data into (which should be at least 
 *       count bytes long)
 * @count: the number of bytes to read from the #GIOChannel
 * @bytes_read: returns the number of bytes actually read
 * 
 * Reads data from a #GIOChannel. 
 * 
 * Return value: %G_IO_ERROR_NONE if the operation was successful. 
 *
 * Deprecated:2.2: Use g_io_channel_read_chars() instead.
 **/
GIOError 
g_io_channel_read (GIOChannel *channel, 
		   gchar      *buf, 
		   gsize       count,
		   gsize      *bytes_read)
{
  GError *err = NULL;
  GIOError error;
  GIOStatus status;

  g_return_val_if_fail (channel != NULL, G_IO_ERROR_UNKNOWN);
  g_return_val_if_fail (bytes_read != NULL, G_IO_ERROR_UNKNOWN);

  if (count == 0)
    {
      if (bytes_read)
        *bytes_read = 0;
      return G_IO_ERROR_NONE;
    }

  g_return_val_if_fail (buf != NULL, G_IO_ERROR_UNKNOWN);

  status = channel->funcs->io_read (channel, buf, count, bytes_read, &err);

  error = g_io_error_get_from_g_error (status, err);

  if (err)
    g_error_free (err);

  return error;
}

/**
 * g_io_channel_write:
 * @channel:  a #GIOChannel
 * @buf: the buffer containing the data to write
 * @count: the number of bytes to write
 * @bytes_written: the number of bytes actually written
 * 
 * Writes data to a #GIOChannel. 
 * 
 * Return value:  %G_IO_ERROR_NONE if the operation was successful.
 *
 * Deprecated:2.2: Use g_io_channel_write_chars() instead.
 **/
GIOError 
g_io_channel_write (GIOChannel  *channel, 
		    const gchar *buf, 
		    gsize        count,
		    gsize       *bytes_written)
{
  GError *err = NULL;
  GIOError error;
  GIOStatus status;

  g_return_val_if_fail (channel != NULL, G_IO_ERROR_UNKNOWN);
  g_return_val_if_fail (bytes_written != NULL, G_IO_ERROR_UNKNOWN);

  status = channel->funcs->io_write (channel, buf, count, bytes_written, &err);

  error = g_io_error_get_from_g_error (status, err);

  if (err)
    g_error_free (err);

  return error;
}

/**
 * g_io_channel_seek:
 * @channel: a #GIOChannel
 * @offset: an offset, in bytes, which is added to the position specified 
 *          by @type
 * @type: the position in the file, which can be %G_SEEK_CUR (the current
 *        position), %G_SEEK_SET (the start of the file), or %G_SEEK_END 
 *        (the end of the file)
 * 
 * Sets the current position in the #GIOChannel, similar to the standard 
 * library function fseek(). 
 * 
 * Return value: %G_IO_ERROR_NONE if the operation was successful.
 *
 * Deprecated:2.2: Use g_io_channel_seek_position() instead.
 **/
GIOError 
g_io_channel_seek (GIOChannel *channel,
		   gint64      offset, 
		   GSeekType   type)
{
  GError *err = NULL;
  GIOError error;
  GIOStatus status;

  g_return_val_if_fail (channel != NULL, G_IO_ERROR_UNKNOWN);
  g_return_val_if_fail (channel->is_seekable, G_IO_ERROR_UNKNOWN);

  switch (type)
    {
      case G_SEEK_CUR:
      case G_SEEK_SET:
      case G_SEEK_END:
        break;
      default:
        g_warning ("g_io_channel_seek: unknown seek type");
        return G_IO_ERROR_UNKNOWN;
    }

  status = channel->funcs->io_seek (channel, offset, type, &err);

  error = g_io_error_get_from_g_error (status, err);

  if (err)
    g_error_free (err);

  return error;
}

/* The function g_io_channel_new_file() is prototyped in both
 * giounix.c and giowin32.c, so we stick its documentation here.
 */

/**
 * g_io_channel_new_file:
 * @filename: A string containing the name of a file
 * @mode: One of "r", "w", "a", "r+", "w+", "a+". These have
 *        the same meaning as in fopen()
 * @error: A location to return an error of type %G_FILE_ERROR
 *
 * Open a file @filename as a #GIOChannel using mode @mode. This
 * channel will be closed when the last reference to it is dropped,
 * so there is no need to call g_io_channel_close() (though doing
 * so will not cause problems, as long as no attempt is made to
 * access the channel after it is closed).
 *
 * Return value: A #GIOChannel on success, %NULL on failure.
 **/

/**
 * g_io_channel_close:
 * @channel: A #GIOChannel
 * 
 * Close an IO channel. Any pending data to be written will be
 * flushed, ignoring errors. The channel will not be freed until the
 * last reference is dropped using g_io_channel_unref(). 
 *
 * Deprecated:2.2: Use g_io_channel_shutdown() instead.
 **/
void
g_io_channel_close (GIOChannel *channel)
{
  GError *err = NULL;
  
  g_return_if_fail (channel != NULL);

  g_io_channel_purge (channel);

  channel->funcs->io_close (channel, &err);

  if (err)
    { /* No way to return the error */
      g_warning ("Error closing channel: %s", err->message);
      g_error_free (err);
    }
  
  channel->close_on_unref = FALSE; /* Because we already did */
  channel->is_readable = FALSE;
  channel->is_writeable = FALSE;
  channel->is_seekable = FALSE;
}

/**
 * g_io_channel_shutdown:
 * @channel: a #GIOChannel
 * @flush: if %TRUE, flush pending
 * @err: location to store a #GIOChannelError
 * 
 * Close an IO channel. Any pending data to be written will be
 * flushed if @flush is %TRUE. The channel will not be freed until the
 * last reference is dropped using g_io_channel_unref().
 *
 * Return value: the status of the operation.
 **/
GIOStatus
g_io_channel_shutdown (GIOChannel  *channel,
		       gboolean     flush,
		       GError     **err)
{
  GIOStatus status, result;
  GError *tmperr = NULL;
  
  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (err == NULL || *err == NULL, G_IO_STATUS_ERROR);

  if (channel->write_buf && channel->write_buf->len > 0)
    {
      if (flush)
        {
          GIOFlags flags;
      
          /* Set the channel to blocking, to avoid a busy loop
           */
          flags = g_io_channel_get_flags (channel);
          /* Ignore any errors here, they're irrelevant */
          g_io_channel_set_flags (channel, flags & ~G_IO_FLAG_NONBLOCK, NULL);

          result = g_io_channel_flush (channel, &tmperr);
        }
      else
        result = G_IO_STATUS_NORMAL;

      g_string_truncate(channel->write_buf, 0);
    }
  else
    result = G_IO_STATUS_NORMAL;

  if (channel->partial_write_buf[0] != '\0')
    {
      if (flush)
        g_warning ("Partial character at end of write buffer not flushed.\n");
      channel->partial_write_buf[0] = '\0';
    }

  status = channel->funcs->io_close (channel, err);

  channel->close_on_unref = FALSE; /* Because we already did */
  channel->is_readable = FALSE;
  channel->is_writeable = FALSE;
  channel->is_seekable = FALSE;

  if (status != G_IO_STATUS_NORMAL)
    {
      g_clear_error (&tmperr);
      return status;
    }
  else if (result != G_IO_STATUS_NORMAL)
    {
      g_propagate_error (err, tmperr);
      return result;
    }
  else
    return G_IO_STATUS_NORMAL;
}

/* This function is used for the final flush on close or unref */
static void
g_io_channel_purge (GIOChannel *channel)
{
  GError *err = NULL;
  GIOStatus status;

  g_return_if_fail (channel != NULL);

  if (channel->write_buf && channel->write_buf->len > 0)
    {
      GIOFlags flags;
      
      /* Set the channel to blocking, to avoid a busy loop
       */
      flags = g_io_channel_get_flags (channel);
      g_io_channel_set_flags (channel, flags & ~G_IO_FLAG_NONBLOCK, NULL);

      status = g_io_channel_flush (channel, &err);

      if (err)
	{ /* No way to return the error */
	  g_warning ("Error flushing string: %s", err->message);
	  g_error_free (err);
	}
    }

  /* Flush these in case anyone tries to close without unrefing */

  if (channel->read_buf)
    g_string_truncate (channel->read_buf, 0);
  if (channel->write_buf)
    g_string_truncate (channel->write_buf, 0);
  if (channel->encoding)
    {
      if (channel->encoded_read_buf)
        g_string_truncate (channel->encoded_read_buf, 0);

      if (channel->partial_write_buf[0] != '\0')
        {
          g_warning ("Partial character at end of write buffer not flushed.\n");
          channel->partial_write_buf[0] = '\0';
        }
    }
}

/**
 * g_io_create_watch:
 * @channel: a #GIOChannel to watch
 * @condition: conditions to watch for
 *
 * Creates a #GSource that's dispatched when @condition is met for the 
 * given @channel. For example, if condition is #G_IO_IN, the source will 
 * be dispatched when there's data available for reading.
 *
 * g_io_add_watch() is a simpler interface to this same functionality, for 
 * the case where you want to add the source to the default main loop context 
 * at the default priority.
 *
 * On Windows, polling a #GSource created to watch a channel for a socket
 * puts the socket in non-blocking mode. This is a side-effect of the
 * implementation and unavoidable.
 *
 * Returns: a new #GSource
 */
GSource *
g_io_create_watch (GIOChannel   *channel,
		   GIOCondition  condition)
{
  g_return_val_if_fail (channel != NULL, NULL);

  return channel->funcs->io_create_watch (channel, condition);
}

/**
 * g_io_add_watch_full:
 * @channel: a #GIOChannel
 * @priority: the priority of the #GIOChannel source
 * @condition: the condition to watch for
 * @func: the function to call when the condition is satisfied
 * @user_data: user data to pass to @func
 * @notify: the function to call when the source is removed
 *
 * Adds the #GIOChannel into the default main loop context
 * with the given priority.
 *
 * This internally creates a main loop source using g_io_create_watch()
 * and attaches it to the main loop context with g_source_attach().
 * You can do these steps manuallt if you need greater control.
 *
 * Returns: the event source id
 */
guint 
g_io_add_watch_full (GIOChannel    *channel,
		     gint           priority,
		     GIOCondition   condition,
		     GIOFunc        func,
		     gpointer       user_data,
		     GDestroyNotify notify)
{
  GSource *source;
  guint id;
  
  g_return_val_if_fail (channel != NULL, 0);

  source = g_io_create_watch (channel, condition);

  if (priority != G_PRIORITY_DEFAULT)
    g_source_set_priority (source, priority);
  g_source_set_callback (source, (GSourceFunc)func, user_data, notify);

  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

/**
 * g_io_add_watch:
 * @channel: a #GIOChannel
 * @condition: the condition to watch for
 * @func: the function to call when the condition is satisfied
 * @user_data: user data to pass to @func
 *
 * Adds the #GIOChannel into the default main loop context
 * with the default priority.
 *
 * Returns: the event source id
 */
guint 
g_io_add_watch (GIOChannel   *channel,
		GIOCondition  condition,
		GIOFunc       func,
		gpointer      user_data)
{
  return g_io_add_watch_full (channel, G_PRIORITY_DEFAULT, condition, func, user_data, NULL);
}

/**
 * g_io_channel_get_buffer_condition:
 * @channel: A #GIOChannel
 *
 * This function returns a #GIOCondition depending on whether there
 * is data to be read/space to write data in the internal buffers in 
 * the #GIOChannel. Only the flags %G_IO_IN and %G_IO_OUT may be set.
 *
 * Return value: A #GIOCondition
 **/
GIOCondition
g_io_channel_get_buffer_condition (GIOChannel *channel)
{
  GIOCondition condition = 0;

  if (channel->encoding)
    {
      if (channel->encoded_read_buf && (channel->encoded_read_buf->len > 0))
        condition |= G_IO_IN; /* Only return if we have full characters */
    }
  else
    {
      if (channel->read_buf && (channel->read_buf->len > 0))
        condition |= G_IO_IN;
    }

  if (channel->write_buf && (channel->write_buf->len < channel->buf_size))
    condition |= G_IO_OUT;

  return condition;
}

/**
 * g_io_channel_error_from_errno:
 * @en: an <literal>errno</literal> error number, e.g. %EINVAL
 *
 * Converts an <literal>errno</literal> error number to a #GIOChannelError.
 *
 * Return value: a #GIOChannelError error number, e.g. 
 *      %G_IO_CHANNEL_ERROR_INVAL.
 **/
GIOChannelError
g_io_channel_error_from_errno (gint en)
{
#ifdef EAGAIN
  g_return_val_if_fail (en != EAGAIN, G_IO_CHANNEL_ERROR_FAILED);
#endif

  switch (en)
    {
#ifdef EBADF
    case EBADF:
      g_warning("Invalid file descriptor.\n");
      return G_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EFAULT
    case EFAULT:
      g_warning("Buffer outside valid address space.\n");
      return G_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EFBIG
    case EFBIG:
      return G_IO_CHANNEL_ERROR_FBIG;
#endif

#ifdef EINTR
    /* In general, we should catch EINTR before we get here,
     * but close() is allowed to return EINTR by POSIX, so
     * we need to catch it here; EINTR from close() is
     * unrecoverable, because it's undefined whether
     * the fd was actually closed or not, so we just return
     * a generic error code.
     */
    case EINTR:
      return G_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EINVAL
    case EINVAL:
      return G_IO_CHANNEL_ERROR_INVAL;
#endif

#ifdef EIO
    case EIO:
      return G_IO_CHANNEL_ERROR_IO;
#endif

#ifdef EISDIR
    case EISDIR:
      return G_IO_CHANNEL_ERROR_ISDIR;
#endif

#ifdef ENOSPC
    case ENOSPC:
      return G_IO_CHANNEL_ERROR_NOSPC;
#endif

#ifdef ENXIO
    case ENXIO:
      return G_IO_CHANNEL_ERROR_NXIO;
#endif

#ifdef EOVERFLOW
    case EOVERFLOW:
      return G_IO_CHANNEL_ERROR_OVERFLOW;
#endif

#ifdef EPIPE
    case EPIPE:
      return G_IO_CHANNEL_ERROR_PIPE;
#endif

    default:
      return G_IO_CHANNEL_ERROR_FAILED;
    }
}

/**
 * g_io_channel_set_buffer_size:
 * @channel: a #GIOChannel
 * @size: the size of the buffer, or 0 to let GLib pick a good size
 *
 * Sets the buffer size.
 **/  
void
g_io_channel_set_buffer_size (GIOChannel *channel,
                              gsize       size)
{
  g_return_if_fail (channel != NULL);

  if (size == 0)
    size = G_IO_NICE_BUF_SIZE;

  if (size < MAX_CHAR_SIZE)
    size = MAX_CHAR_SIZE;

  channel->buf_size = size;
}

/**
 * g_io_channel_get_buffer_size:
 * @channel: a #GIOChannel
 *
 * Gets the buffer size.
 *
 * Return value: the size of the buffer.
 **/  
gsize
g_io_channel_get_buffer_size (GIOChannel *channel)
{
  g_return_val_if_fail (channel != NULL, 0);

  return channel->buf_size;
}

/**
 * g_io_channel_set_line_term:
 * @channel: a #GIOChannel
 * @line_term: The line termination string. Use %NULL for autodetect.
 *             Autodetection breaks on "\n", "\r\n", "\r", "\0", and
 *             the Unicode paragraph separator. Autodetection should
 *             not be used for anything other than file-based channels.
 * @length: The length of the termination string. If -1 is passed, the
 *          string is assumed to be nul-terminated. This option allows
 *          termination strings with embedded nuls.
 *
 * This sets the string that #GIOChannel uses to determine
 * where in the file a line break occurs.
 **/
void
g_io_channel_set_line_term (GIOChannel	*channel,
                            const gchar	*line_term,
			    gint         length)
{
  g_return_if_fail (channel != NULL);
  g_return_if_fail (line_term == NULL || length != 0); /* Disallow "" */

  if (line_term == NULL)
    length = 0;
  else if (length < 0)
    length = strlen (line_term);

  g_free (channel->line_term);
  channel->line_term = line_term ? g_memdup (line_term, length) : NULL;
  channel->line_term_len = length;
}

/**
 * g_io_channel_get_line_term:
 * @channel: a #GIOChannel
 * @length: a location to return the length of the line terminator
 *
 * This returns the string that #GIOChannel uses to determine
 * where in the file a line break occurs. A value of %NULL
 * indicates autodetection.
 *
 * Return value: The line termination string. This value
 *   is owned by GLib and must not be freed.
 **/
G_CONST_RETURN gchar*
g_io_channel_get_line_term (GIOChannel *channel,
			    gint       *length)
{
  g_return_val_if_fail (channel != NULL, NULL);

  if (length)
    *length = channel->line_term_len;

  return channel->line_term;
}

/**
 * g_io_channel_set_flags:
 * @channel: a #GIOChannel
 * @flags: the flags to set on the IO channel
 * @error: A location to return an error of type #GIOChannelError
 *
 * Sets the (writeable) flags in @channel to (@flags & %G_IO_CHANNEL_SET_MASK).
 *
 * Return value: the status of the operation. 
 **/
GIOStatus
g_io_channel_set_flags (GIOChannel  *channel,
                        GIOFlags     flags,
                        GError     **error)
{
  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);

  return (*channel->funcs->io_set_flags) (channel,
					  flags & G_IO_FLAG_SET_MASK,
					  error);
}

/**
 * g_io_channel_get_flags:
 * @channel: a #GIOChannel
 *
 * Gets the current flags for a #GIOChannel, including read-only
 * flags such as %G_IO_FLAG_IS_READABLE.
 *
 * The values of the flags %G_IO_FLAG_IS_READABLE and %G_IO_FLAG_IS_WRITEABLE
 * are cached for internal use by the channel when it is created.
 * If they should change at some later point (e.g. partial shutdown
 * of a socket with the UNIX shutdown() function), the user
 * should immediately call g_io_channel_get_flags() to update
 * the internal values of these flags.
 *
 * Return value: the flags which are set on the channel
 **/
GIOFlags
g_io_channel_get_flags (GIOChannel *channel)
{
  GIOFlags flags;

  g_return_val_if_fail (channel != NULL, 0);

  flags = (* channel->funcs->io_get_flags) (channel);

  /* Cross implementation code */

  if (channel->is_seekable)
    flags |= G_IO_FLAG_IS_SEEKABLE;
  if (channel->is_readable)
    flags |= G_IO_FLAG_IS_READABLE;
  if (channel->is_writeable)
    flags |= G_IO_FLAG_IS_WRITEABLE;

  return flags;
}

/**
 * g_io_channel_set_close_on_unref:
 * @channel: a #GIOChannel
 * @do_close: Whether to close the channel on the final unref of
 *            the GIOChannel data structure. The default value of
 *            this is %TRUE for channels created by g_io_channel_new_file (),
 *            and %FALSE for all other channels.
 *
 * Setting this flag to %TRUE for a channel you have already closed
 * can cause problems.
 **/
void
g_io_channel_set_close_on_unref	(GIOChannel *channel,
				 gboolean    do_close)
{
  g_return_if_fail (channel != NULL);

  channel->close_on_unref = do_close;
}

/**
 * g_io_channel_get_close_on_unref:
 * @channel: a #GIOChannel.
 *
 * Returns whether the file/socket/whatever associated with @channel
 * will be closed when @channel receives its final unref and is
 * destroyed. The default value of this is %TRUE for channels created
 * by g_io_channel_new_file (), and %FALSE for all other channels.
 *
 * Return value: Whether the channel will be closed on the final unref of
 *               the GIOChannel data structure.
 **/
gboolean
g_io_channel_get_close_on_unref	(GIOChannel *channel)
{
  g_return_val_if_fail (channel != NULL, FALSE);

  return channel->close_on_unref;
}

/**
 * g_io_channel_seek_position:
 * @channel: a #GIOChannel
 * @offset: The offset in bytes from the position specified by @type
 * @type: a #GSeekType. The type %G_SEEK_CUR is only allowed in those
 *                      cases where a call to g_io_channel_set_encoding ()
 *                      is allowed. See the documentation for
 *                      g_io_channel_set_encoding () for details.
 * @error: A location to return an error of type #GIOChannelError
 *
 * Replacement for g_io_channel_seek() with the new API.
 *
 * Return value: the status of the operation.
 **/
GIOStatus
g_io_channel_seek_position (GIOChannel  *channel,
                            gint64       offset,
                            GSeekType    type,
                            GError     **error)
{
  GIOStatus status;

  /* For files, only one of the read and write buffers can contain data.
   * For sockets, both can contain data.
   */

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_seekable, G_IO_STATUS_ERROR);

  switch (type)
    {
      case G_SEEK_CUR: /* The user is seeking relative to the head of the buffer */
        if (channel->use_buffer)
          {
            if (channel->do_encode && channel->encoded_read_buf
                && channel->encoded_read_buf->len > 0)
              {
                g_warning ("Seek type G_SEEK_CUR not allowed for this"
                  " channel's encoding.\n");
                return G_IO_STATUS_ERROR;
              }
          if (channel->read_buf)
            offset -= channel->read_buf->len;
          if (channel->encoded_read_buf)
            {
              g_assert (channel->encoded_read_buf->len == 0 || !channel->do_encode);

              /* If there's anything here, it's because the encoding is UTF-8,
               * so we can just subtract the buffer length, the same as for
               * the unencoded data.
               */

              offset -= channel->encoded_read_buf->len;
            }
          }
        break;
      case G_SEEK_SET:
      case G_SEEK_END:
        break;
      default:
        g_warning ("g_io_channel_seek_position: unknown seek type");
        return G_IO_STATUS_ERROR;
    }

  if (channel->use_buffer)
    {
      status = g_io_channel_flush (channel, error);
      if (status != G_IO_STATUS_NORMAL)
        return status;
    }

  status = channel->funcs->io_seek (channel, offset, type, error);

  if ((status == G_IO_STATUS_NORMAL) && (channel->use_buffer))
    {
      if (channel->read_buf)
        g_string_truncate (channel->read_buf, 0);

      /* Conversion state no longer matches position in file */
      if (channel->read_cd != (GIConv) -1)
        g_iconv (channel->read_cd, NULL, NULL, NULL, NULL);
      if (channel->write_cd != (GIConv) -1)
        g_iconv (channel->write_cd, NULL, NULL, NULL, NULL);

      if (channel->encoded_read_buf)
        {
          g_assert (channel->encoded_read_buf->len == 0 || !channel->do_encode);
          g_string_truncate (channel->encoded_read_buf, 0);
        }

      if (channel->partial_write_buf[0] != '\0')
        {
          g_warning ("Partial character at end of write buffer not flushed.\n");
          channel->partial_write_buf[0] = '\0';
        }
    }

  return status;
}

/**
 * g_io_channel_flush:
 * @channel: a #GIOChannel
 * @error: location to store an error of type #GIOChannelError
 *
 * Flushes the write buffer for the GIOChannel.
 *
 * Return value: the status of the operation: One of
 *   #G_IO_STATUS_NORMAL, #G_IO_STATUS_AGAIN, or
 *   #G_IO_STATUS_ERROR.
 **/
GIOStatus
g_io_channel_flush (GIOChannel	*channel,
		    GError     **error)
{
  GIOStatus status;
  gsize this_time = 1, bytes_written = 0;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), G_IO_STATUS_ERROR);

  if (channel->write_buf == NULL || channel->write_buf->len == 0)
    return G_IO_STATUS_NORMAL;

  do
    {
      g_assert (this_time > 0);

      status = channel->funcs->io_write (channel,
					 channel->write_buf->str + bytes_written,
					 channel->write_buf->len - bytes_written,
					 &this_time, error);
      bytes_written += this_time;
    }
  while ((bytes_written < channel->write_buf->len)
         && (status == G_IO_STATUS_NORMAL));

  g_string_erase (channel->write_buf, 0, bytes_written);

  return status;
}

/**
 * g_io_channel_set_buffered:
 * @channel: a #GIOChannel
 * @buffered: whether to set the channel buffered or unbuffered
 *
 * The buffering state can only be set if the channel's encoding
 * is %NULL. For any other encoding, the channel must be buffered.
 *
 * A buffered channel can only be set unbuffered if the channel's
 * internal buffers have been flushed. Newly created channels or
 * channels which have returned %G_IO_STATUS_EOF
 * not require such a flush. For write-only channels, a call to
 * g_io_channel_flush () is sufficient. For all other channels,
 * the buffers may be flushed by a call to g_io_channel_seek_position ().
 * This includes the possibility of seeking with seek type %G_SEEK_CUR
 * and an offset of zero. Note that this means that socket-based
 * channels cannot be set unbuffered once they have had data
 * read from them.
 *
 * On unbuffered channels, it is safe to mix read and write
 * calls from the new and old APIs, if this is necessary for
 * maintaining old code.
 *
 * The default state of the channel is buffered.
 **/
void
g_io_channel_set_buffered (GIOChannel *channel,
                           gboolean    buffered)
{
  g_return_if_fail (channel != NULL);

  if (channel->encoding != NULL)
    {
      g_warning ("Need to have NULL encoding to set the buffering state of the "
                 "channel.\n");
      return;
    }

  g_return_if_fail (!channel->read_buf || channel->read_buf->len == 0);
  g_return_if_fail (!channel->write_buf || channel->write_buf->len == 0);

  channel->use_buffer = buffered;
}

/**
 * g_io_channel_get_buffered:
 * @channel: a #GIOChannel
 *
 * Returns whether @channel is buffered.
 *
 * Return Value: %TRUE if the @channel is buffered. 
 **/
gboolean
g_io_channel_get_buffered (GIOChannel *channel)
{
  g_return_val_if_fail (channel != NULL, FALSE);

  return channel->use_buffer;
}

/**
 * g_io_channel_set_encoding:
 * @channel: a #GIOChannel
 * @encoding: the encoding type
 * @error: location to store an error of type #GConvertError
 *
 * Sets the encoding for the input/output of the channel. 
 * The internal encoding is always UTF-8. The default encoding 
 * for the external file is UTF-8.
 *
 * The encoding %NULL is safe to use with binary data.
 *
 * The encoding can only be set if one of the following conditions
 * is true:
 * <itemizedlist>
 * <listitem><para>
 *    The channel was just created, and has not been written to or read 
 *    from yet.
 * </para></listitem>
 * <listitem><para>
 *    The channel is write-only.
 * </para></listitem>
 * <listitem><para>
 *    The channel is a file, and the file pointer was just
 *    repositioned by a call to g_io_channel_seek_position().
 *    (This flushes all the internal buffers.)
 * </para></listitem>
 * <listitem><para>
 *    The current encoding is %NULL or UTF-8.
 * </para></listitem>
 * <listitem><para>
 *    One of the (new API) read functions has just returned %G_IO_STATUS_EOF
 *    (or, in the case of g_io_channel_read_to_end(), %G_IO_STATUS_NORMAL).
 * </para></listitem>
 * <listitem><para>
 *    One of the functions g_io_channel_read_chars() or 
 *    g_io_channel_read_unichar() has returned %G_IO_STATUS_AGAIN or 
 *    %G_IO_STATUS_ERROR. This may be useful in the case of 
 *    %G_CONVERT_ERROR_ILLEGAL_SEQUENCE.
 *    Returning one of these statuses from g_io_channel_read_line(),
 *    g_io_channel_read_line_string(), or g_io_channel_read_to_end()
 *    does <emphasis>not</emphasis> guarantee that the encoding can 
 *    be changed.
 * </para></listitem>
 * </itemizedlist>
 * Channels which do not meet one of the above conditions cannot call
 * g_io_channel_seek_position() with an offset of %G_SEEK_CUR, and, if 
 * they are "seekable", cannot call g_io_channel_write_chars() after 
 * calling one of the API "read" functions.
 *
 * Return Value: %G_IO_STATUS_NORMAL if the encoding was successfully set.
 **/
GIOStatus
g_io_channel_set_encoding (GIOChannel	*channel,
                           const gchar	*encoding,
			   GError      **error)
{
  GIConv read_cd, write_cd;
  gboolean did_encode;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL), G_IO_STATUS_ERROR);

  /* Make sure the encoded buffers are empty */

  g_return_val_if_fail (!channel->do_encode || !channel->encoded_read_buf ||
			channel->encoded_read_buf->len == 0, G_IO_STATUS_ERROR);

  if (!channel->use_buffer)
    {
      g_warning ("Need to set the channel buffered before setting the encoding.\n");
      g_warning ("Assuming this is what you meant and acting accordingly.\n");

      channel->use_buffer = TRUE;
    }

  if (channel->partial_write_buf[0] != '\0')
    {
      g_warning ("Partial character at end of write buffer not flushed.\n");
      channel->partial_write_buf[0] = '\0';
    }

  did_encode = channel->do_encode;

  if (!encoding || strcmp (encoding, "UTF8") == 0 || strcmp (encoding, "UTF-8") == 0)
    {
      channel->do_encode = FALSE;
      read_cd = write_cd = (GIConv) -1;
    }
  else
    {
      gint err = 0;
      const gchar *from_enc = NULL, *to_enc = NULL;

      if (channel->is_readable)
        {
          read_cd = g_iconv_open ("UTF-8", encoding);

          if (read_cd == (GIConv) -1)
            {
              err = errno;
              from_enc = encoding;
              to_enc = "UTF-8";
            }
        }
      else
        read_cd = (GIConv) -1;

      if (channel->is_writeable && err == 0)
        {
          write_cd = g_iconv_open (encoding, "UTF-8");

          if (write_cd == (GIConv) -1)
            {
              err = errno;
              from_enc = "UTF-8";
              to_enc = encoding;
            }
        }
      else
        write_cd = (GIConv) -1;

      if (err != 0)
        {
          g_assert (from_enc);
          g_assert (to_enc);

          if (err == EINVAL)
            g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_NO_CONVERSION,
                         _("Conversion from character set '%s' to '%s' is not supported"),
                         from_enc, to_enc);
          else
            g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                         _("Could not open converter from '%s' to '%s': %s"),
                         from_enc, to_enc, g_strerror (err));

          if (read_cd != (GIConv) -1)
            g_iconv_close (read_cd);
          if (write_cd != (GIConv) -1)
            g_iconv_close (write_cd);

          return G_IO_STATUS_ERROR;
        }

      channel->do_encode = TRUE;
    }

  /* The encoding is ok, so set the fields in channel */

  if (channel->read_cd != (GIConv) -1)
    g_iconv_close (channel->read_cd);
  if (channel->write_cd != (GIConv) -1)
    g_iconv_close (channel->write_cd);

  if (channel->encoded_read_buf && channel->encoded_read_buf->len > 0)
    {
      g_assert (!did_encode); /* Encoding UTF-8, NULL doesn't use encoded_read_buf */

      /* This is just validated UTF-8, so we can copy it back into read_buf
       * so it can be encoded in whatever the new encoding is.
       */

      g_string_prepend_len (channel->read_buf, channel->encoded_read_buf->str,
                            channel->encoded_read_buf->len);
      g_string_truncate (channel->encoded_read_buf, 0);
    }

  channel->read_cd = read_cd;
  channel->write_cd = write_cd;

  g_free (channel->encoding);
  channel->encoding = g_strdup (encoding);

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_get_encoding:
 * @channel: a #GIOChannel
 *
 * Gets the encoding for the input/output of the channel. 
 * The internal encoding is always UTF-8. The encoding %NULL 
 * makes the channel safe for binary data.
 *
 * Return value: A string containing the encoding, this string is
 *   owned by GLib and must not be freed.
 **/
G_CONST_RETURN gchar*
g_io_channel_get_encoding (GIOChannel *channel)
{
  g_return_val_if_fail (channel != NULL, NULL);

  return channel->encoding;
}

static GIOStatus
g_io_channel_fill_buffer (GIOChannel  *channel,
                          GError     **err)
{
  gsize read_size, cur_len, oldlen;
  GIOStatus status;

  if (channel->is_seekable && channel->write_buf && channel->write_buf->len > 0)
    {
      status = g_io_channel_flush (channel, err);
      if (status != G_IO_STATUS_NORMAL)
        return status;
    }
  if (channel->is_seekable && channel->partial_write_buf[0] != '\0')
    {
      g_warning ("Partial character at end of write buffer not flushed.\n");
      channel->partial_write_buf[0] = '\0';
    }

  if (!channel->read_buf)
    channel->read_buf = g_string_sized_new (channel->buf_size);

  cur_len = channel->read_buf->len;

  g_string_set_size (channel->read_buf, channel->read_buf->len + channel->buf_size);

  status = channel->funcs->io_read (channel, channel->read_buf->str + cur_len,
                                    channel->buf_size, &read_size, err);

  g_assert ((status == G_IO_STATUS_NORMAL) || (read_size == 0));

  g_string_truncate (channel->read_buf, read_size + cur_len);

  if ((status != G_IO_STATUS_NORMAL) &&
      ((status != G_IO_STATUS_EOF) || (channel->read_buf->len == 0)))
    return status;

  g_assert (channel->read_buf->len > 0);

  if (channel->encoded_read_buf)
    oldlen = channel->encoded_read_buf->len;
  else
    {
      oldlen = 0;
      if (channel->encoding)
        channel->encoded_read_buf = g_string_sized_new (channel->buf_size);
    }

  if (channel->do_encode)
    {
      gsize errnum, inbytes_left, outbytes_left;
      gchar *inbuf, *outbuf;
      int errval;

      g_assert (channel->encoded_read_buf);

reencode:

      inbytes_left = channel->read_buf->len;
      outbytes_left = MAX (channel->read_buf->len,
                           channel->encoded_read_buf->allocated_len
                           - channel->encoded_read_buf->len - 1); /* 1 for NULL */
      outbytes_left = MAX (outbytes_left, 6);

      inbuf = channel->read_buf->str;
      g_string_set_size (channel->encoded_read_buf,
                         channel->encoded_read_buf->len + outbytes_left);
      outbuf = channel->encoded_read_buf->str + channel->encoded_read_buf->len
               - outbytes_left;

      errnum = g_iconv (channel->read_cd, &inbuf, &inbytes_left,
			&outbuf, &outbytes_left);
      errval = errno;

      g_assert (inbuf + inbytes_left == channel->read_buf->str
                + channel->read_buf->len);
      g_assert (outbuf + outbytes_left == channel->encoded_read_buf->str
                + channel->encoded_read_buf->len);

      g_string_erase (channel->read_buf, 0,
		      channel->read_buf->len - inbytes_left);
      g_string_truncate (channel->encoded_read_buf,
			 channel->encoded_read_buf->len - outbytes_left);

      if (errnum == (gsize) -1)
        {
          switch (errval)
            {
              case EINVAL:
                if ((oldlen == channel->encoded_read_buf->len)
                  && (status == G_IO_STATUS_EOF))
                  status = G_IO_STATUS_EOF;
                else
                  status = G_IO_STATUS_NORMAL;
                break;
              case E2BIG:
                /* Buffer size at least 6, wrote at least on character */
                g_assert (inbuf != channel->read_buf->str);
                goto reencode;
              case EILSEQ:
                if (oldlen < channel->encoded_read_buf->len)
                  status = G_IO_STATUS_NORMAL;
                else
                  {
                    g_set_error_literal (err, G_CONVERT_ERROR,
                      G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                      _("Invalid byte sequence in conversion input"));
                    return G_IO_STATUS_ERROR;
                  }
                break;
              default:
                g_assert (errval != EBADF); /* The converter should be open */
                g_set_error (err, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                  _("Error during conversion: %s"), g_strerror (errval));
                return G_IO_STATUS_ERROR;
            }
        }
      g_assert ((status != G_IO_STATUS_NORMAL)
               || (channel->encoded_read_buf->len > 0));
    }
  else if (channel->encoding) /* UTF-8 */
    {
      gchar *nextchar, *lastchar;

      g_assert (channel->encoded_read_buf);

      nextchar = channel->read_buf->str;
      lastchar = channel->read_buf->str + channel->read_buf->len;

      while (nextchar < lastchar)
        {
          gunichar val_char;

          val_char = g_utf8_get_char_validated (nextchar, lastchar - nextchar);

          switch (val_char)
            {
              case -2:
                /* stop, leave partial character in buffer */
                lastchar = nextchar;
                break;
              case -1:
                if (oldlen < channel->encoded_read_buf->len)
                  status = G_IO_STATUS_NORMAL;
                else
                  {
                    g_set_error_literal (err, G_CONVERT_ERROR,
                      G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                      _("Invalid byte sequence in conversion input"));
                    status = G_IO_STATUS_ERROR;
                  }
                lastchar = nextchar;
                break;
              default:
                nextchar = g_utf8_next_char (nextchar);
                break;
            }
        }

      if (lastchar > channel->read_buf->str)
        {
          gint copy_len = lastchar - channel->read_buf->str;

          g_string_append_len (channel->encoded_read_buf, channel->read_buf->str,
                               copy_len);
          g_string_erase (channel->read_buf, 0, copy_len);
        }
    }

  return status;
}

/**
 * g_io_channel_read_line:
 * @channel: a #GIOChannel
 * @str_return: The line read from the #GIOChannel, including the
 *              line terminator. This data should be freed with g_free()
 *              when no longer needed. This is a nul-terminated string. 
 *              If a @length of zero is returned, this will be %NULL instead.
 * @length: location to store length of the read data, or %NULL
 * @terminator_pos: location to store position of line terminator, or %NULL
 * @error: A location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads a line, including the terminating character(s),
 * from a #GIOChannel into a newly-allocated string.
 * @str_return will contain allocated memory if the return
 * is %G_IO_STATUS_NORMAL.
 *
 * Return value: the status of the operation.
 **/
GIOStatus
g_io_channel_read_line (GIOChannel  *channel,
                        gchar      **str_return,
                        gsize       *length,
			gsize       *terminator_pos,
		        GError     **error)
{
  GIOStatus status;
  gsize got_length;
  
  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (str_return != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  status = g_io_channel_read_line_backend (channel, &got_length, terminator_pos, error);

  if (length)
    *length = got_length;

  if (status == G_IO_STATUS_NORMAL)
    {
      g_assert (USE_BUF (channel));
      *str_return = g_strndup (USE_BUF (channel)->str, got_length);
      g_string_erase (USE_BUF (channel), 0, got_length);
    }
  else
    *str_return = NULL;
  
  return status;
}

/**
 * g_io_channel_read_line_string:
 * @channel: a #GIOChannel
 * @buffer: a #GString into which the line will be written.
 *          If @buffer already contains data, the old data will
 *          be overwritten.
 * @terminator_pos: location to store position of line terminator, or %NULL
 * @error: a location to store an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads a line from a #GIOChannel, using a #GString as a buffer.
 *
 * Return value: the status of the operation.
 **/
GIOStatus
g_io_channel_read_line_string (GIOChannel  *channel,
                               GString	   *buffer,
			       gsize       *terminator_pos,
                               GError	  **error)
{
  gsize length;
  GIOStatus status;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (buffer != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  if (buffer->len > 0)
    g_string_truncate (buffer, 0); /* clear out the buffer */

  status = g_io_channel_read_line_backend (channel, &length, terminator_pos, error);

  if (status == G_IO_STATUS_NORMAL)
    {
      g_assert (USE_BUF (channel));
      g_string_append_len (buffer, USE_BUF (channel)->str, length);
      g_string_erase (USE_BUF (channel), 0, length);
    }

  return status;
}


static GIOStatus
g_io_channel_read_line_backend (GIOChannel  *channel,
                                gsize       *length,
                                gsize       *terminator_pos,
                                GError     **error)
{
  GIOStatus status;
  gsize checked_to, line_term_len, line_length, got_term_len;
  gboolean first_time = TRUE;

  if (!channel->use_buffer)
    {
      /* Can't do a raw read in read_line */
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                           _("Can't do a raw read in g_io_channel_read_line_string"));
      return G_IO_STATUS_ERROR;
    }

  status = G_IO_STATUS_NORMAL;

  if (channel->line_term)
    line_term_len = channel->line_term_len;
  else
    line_term_len = 3;
    /* This value used for setting checked_to, it's the longest of the four
     * we autodetect for.
     */

  checked_to = 0;

  while (TRUE)
    {
      gchar *nextchar, *lastchar;
      GString *use_buf;

      if (!first_time || (BUF_LEN (USE_BUF (channel)) == 0))
        {
read_again:
          status = g_io_channel_fill_buffer (channel, error);
          switch (status)
            {
              case G_IO_STATUS_NORMAL:
                if (BUF_LEN (USE_BUF (channel)) == 0)
                  /* Can happen when using conversion and only read
                   * part of a character
                   */
                  {
                    first_time = FALSE;
                    continue;
                  }
                break;
              case G_IO_STATUS_EOF:
                if (BUF_LEN (USE_BUF (channel)) == 0)
                  {
                    if (length)
                      *length = 0;

                    if (channel->encoding && channel->read_buf->len != 0)
                      {
                        g_set_error_literal (error, G_CONVERT_ERROR,
                                             G_CONVERT_ERROR_PARTIAL_INPUT,
                                             _("Leftover unconverted data in "
                                               "read buffer"));
                        return G_IO_STATUS_ERROR;
                      }
                    else
                      return G_IO_STATUS_EOF;
                  }
                break;
              default:
                if (length)
                  *length = 0;
                return status;
            }
        }

      g_assert (BUF_LEN (USE_BUF (channel)) != 0);

      use_buf = USE_BUF (channel); /* The buffer has been created by this point */

      first_time = FALSE;

      lastchar = use_buf->str + use_buf->len;

      for (nextchar = use_buf->str + checked_to; nextchar < lastchar;
           channel->encoding ? nextchar = g_utf8_next_char (nextchar) : nextchar++)
        {
          if (channel->line_term)
            {
              if (memcmp (channel->line_term, nextchar, line_term_len) == 0)
                {
                  line_length = nextchar - use_buf->str;
                  got_term_len = line_term_len;
                  goto done;
                }
            }
          else /* auto detect */
            {
              switch (*nextchar)
                {
                  case '\n': /* unix */
                    line_length = nextchar - use_buf->str;
                    got_term_len = 1;
                    goto done;
                  case '\r': /* Warning: do not use with sockets */
                    line_length = nextchar - use_buf->str;
                    if ((nextchar == lastchar - 1) && (status != G_IO_STATUS_EOF)
                       && (lastchar == use_buf->str + use_buf->len))
                      goto read_again; /* Try to read more data */
                    if ((nextchar < lastchar - 1) && (*(nextchar + 1) == '\n')) /* dos */
                      got_term_len = 2;
                    else /* mac */
                      got_term_len = 1;
                    goto done;
                  case '\xe2': /* Unicode paragraph separator */
                    if (strncmp ("\xe2\x80\xa9", nextchar, 3) == 0)
                      {
                        line_length = nextchar - use_buf->str;
                        got_term_len = 3;
                        goto done;
                      }
                    break;
                  case '\0': /* Embeded null in input */
                    line_length = nextchar - use_buf->str;
                    got_term_len = 1;
                    goto done;
                  default: /* no match */
                    break;
                }
            }
        }

      /* If encoding != NULL, valid UTF-8, didn't overshoot */
      g_assert (nextchar == lastchar);

      /* Check for EOF */

      if (status == G_IO_STATUS_EOF)
        {
          if (channel->encoding && channel->read_buf->len > 0)
            {
              g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                                   _("Channel terminates in a partial character"));
              return G_IO_STATUS_ERROR;
            }
          line_length = use_buf->len;
          got_term_len = 0;
          break;
        }

      if (use_buf->len > line_term_len - 1)
	checked_to = use_buf->len - (line_term_len - 1);
      else
	checked_to = 0;
    }

done:

  if (terminator_pos)
    *terminator_pos = line_length;

  if (length)
    *length = line_length + got_term_len;

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_read_to_end:
 * @channel: a #GIOChannel
 * @str_return: Location to store a pointer to a string holding
 *              the remaining data in the #GIOChannel. This data should
 *              be freed with g_free() when no longer needed. This
 *              data is terminated by an extra nul character, but there 
 *              may be other nuls in the intervening data.
 * @length: location to store length of the data
 * @error: location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads all the remaining data from the file.
 *
 * Return value: %G_IO_STATUS_NORMAL on success. 
 *     This function never returns %G_IO_STATUS_EOF.
 **/
GIOStatus
g_io_channel_read_to_end (GIOChannel  *channel,
                          gchar      **str_return,
                          gsize	      *length,
                          GError     **error)
{
  GIOStatus status;
    
  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
    G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  if (str_return)
    *str_return = NULL;
  if (length)
    *length = 0;

  if (!channel->use_buffer)
    {
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                           _("Can't do a raw read in g_io_channel_read_to_end"));
      return G_IO_STATUS_ERROR;
    }

  do
    status = g_io_channel_fill_buffer (channel, error);
  while (status == G_IO_STATUS_NORMAL);

  if (status != G_IO_STATUS_EOF)
    return status;

  if (channel->encoding && channel->read_buf->len > 0)
    {
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                           _("Channel terminates in a partial character"));
      return G_IO_STATUS_ERROR;
    }

  if (USE_BUF (channel) == NULL)
    {
      /* length is already set to zero */
      if (str_return)
        *str_return = g_strdup ("");
    }
  else
    {
      if (length)
        *length = USE_BUF (channel)->len;

      if (str_return)
        *str_return = g_string_free (USE_BUF (channel), FALSE);
      else
        g_string_free (USE_BUF (channel), TRUE);

      if (channel->encoding)
	channel->encoded_read_buf = NULL;
      else
	channel->read_buf = NULL;
    }

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_read_chars:
 * @channel: a #GIOChannel
 * @buf: a buffer to read data into
 * @count: the size of the buffer. Note that the buffer may
 *         not be complelely filled even if there is data
 *         in the buffer if the remaining data is not a
 *         complete character.
 * @bytes_read: The number of bytes read. This may be zero even on
 *              success if count < 6 and the channel's encoding is non-%NULL.
 *              This indicates that the next UTF-8 character is too wide for
 *              the buffer.
 * @error: a location to return an error of type #GConvertError
 *         or #GIOChannelError.
 *
 * Replacement for g_io_channel_read() with the new API.
 *
 * Return value: the status of the operation.
 **/
GIOStatus
g_io_channel_read_chars (GIOChannel  *channel,
                         gchar	     *buf,
                         gsize	      count,
			 gsize       *bytes_read,
                         GError     **error)
{
  GIOStatus status;
  gsize got_bytes;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  if (count == 0)
    {
      *bytes_read = 0;
      return G_IO_STATUS_NORMAL;
    }
  g_return_val_if_fail (buf != NULL, G_IO_STATUS_ERROR);

  if (!channel->use_buffer)
    {
      gsize tmp_bytes;
      
      g_assert (!channel->read_buf || channel->read_buf->len == 0);

      status = channel->funcs->io_read (channel, buf, count, &tmp_bytes, error);
      
      if (bytes_read)
	*bytes_read = tmp_bytes;

      return status;
    }

  status = G_IO_STATUS_NORMAL;

  while (BUF_LEN (USE_BUF (channel)) < count && status == G_IO_STATUS_NORMAL)
    status = g_io_channel_fill_buffer (channel, error);

  /* Only return an error if we have no data */

  if (BUF_LEN (USE_BUF (channel)) == 0)
    {
      g_assert (status != G_IO_STATUS_NORMAL);

      if (status == G_IO_STATUS_EOF && channel->encoding
          && BUF_LEN (channel->read_buf) > 0)
        {
          g_set_error_literal (error, G_CONVERT_ERROR,
                               G_CONVERT_ERROR_PARTIAL_INPUT,
                               _("Leftover unconverted data in read buffer"));
          status = G_IO_STATUS_ERROR;
        }

      if (bytes_read)
        *bytes_read = 0;

      return status;
    }

  if (status == G_IO_STATUS_ERROR)
    g_clear_error (error);

  got_bytes = MIN (count, BUF_LEN (USE_BUF (channel)));

  g_assert (got_bytes > 0);

  if (channel->encoding)
    /* Don't validate for NULL encoding, binary safe */
    {
      gchar *nextchar, *prevchar;

      g_assert (USE_BUF (channel) == channel->encoded_read_buf);

      nextchar = channel->encoded_read_buf->str;

      do
        {
          prevchar = nextchar;
          nextchar = g_utf8_next_char (nextchar);
          g_assert (nextchar != prevchar); /* Possible for *prevchar of -1 or -2 */
        }
      while (nextchar < channel->encoded_read_buf->str + got_bytes);

      if (nextchar > channel->encoded_read_buf->str + got_bytes)
        got_bytes = prevchar - channel->encoded_read_buf->str;

      g_assert (got_bytes > 0 || count < 6);
    }

  memcpy (buf, USE_BUF (channel)->str, got_bytes);
  g_string_erase (USE_BUF (channel), 0, got_bytes);

  if (bytes_read)
    *bytes_read = got_bytes;

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_read_unichar:
 * @channel: a #GIOChannel
 * @thechar: a location to return a character
 * @error: a location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads a Unicode character from @channel.
 * This function cannot be called on a channel with %NULL encoding.
 *
 * Return value: a #GIOStatus
 **/
GIOStatus
g_io_channel_read_unichar (GIOChannel  *channel,
			   gunichar    *thechar,
			   GError     **error)
{
  GIOStatus status = G_IO_STATUS_NORMAL;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->encoding != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  while (BUF_LEN (channel->encoded_read_buf) == 0 && status == G_IO_STATUS_NORMAL)
    status = g_io_channel_fill_buffer (channel, error);

  /* Only return an error if we have no data */

  if (BUF_LEN (USE_BUF (channel)) == 0)
    {
      g_assert (status != G_IO_STATUS_NORMAL);

      if (status == G_IO_STATUS_EOF && BUF_LEN (channel->read_buf) > 0)
        {
          g_set_error_literal (error, G_CONVERT_ERROR,
                               G_CONVERT_ERROR_PARTIAL_INPUT,
                               _("Leftover unconverted data in read buffer"));
          status = G_IO_STATUS_ERROR;
        }

      if (thechar)
        *thechar = (gunichar) -1;

      return status;
    }

  if (status == G_IO_STATUS_ERROR)
    g_clear_error (error);

  if (thechar)
    *thechar = g_utf8_get_char (channel->encoded_read_buf->str);

  g_string_erase (channel->encoded_read_buf, 0,
                  g_utf8_next_char (channel->encoded_read_buf->str)
                  - channel->encoded_read_buf->str);

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_write_chars:
 * @channel: a #GIOChannel
 * @buf: a buffer to write data from
 * @count: the size of the buffer. If -1, the buffer
 *         is taken to be a nul-terminated string.
 * @bytes_written: The number of bytes written. This can be nonzero
 *                 even if the return value is not %G_IO_STATUS_NORMAL.
 *                 If the return value is %G_IO_STATUS_NORMAL and the
 *                 channel is blocking, this will always be equal
 *                 to @count if @count >= 0.
 * @error: a location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Replacement for g_io_channel_write() with the new API.
 *
 * On seekable channels with encodings other than %NULL or UTF-8, generic
 * mixing of reading and writing is not allowed. A call to g_io_channel_write_chars ()
 * may only be made on a channel from which data has been read in the
 * cases described in the documentation for g_io_channel_set_encoding ().
 *
 * Return value: the status of the operation.
 **/
GIOStatus
g_io_channel_write_chars (GIOChannel   *channel,
                          const gchar  *buf,
                          gssize        count,
			  gsize        *bytes_written,
                          GError      **error)
{
  GIOStatus status;
  gssize wrote_bytes = 0;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_writeable, G_IO_STATUS_ERROR);

  if ((count < 0) && buf)
    count = strlen (buf);
  
  if (count == 0)
    {
      if (bytes_written)
        *bytes_written = 0;
      return G_IO_STATUS_NORMAL;
    }

  g_return_val_if_fail (buf != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (count > 0, G_IO_STATUS_ERROR);

  /* Raw write case */

  if (!channel->use_buffer)
    {
      gsize tmp_bytes;
      
      g_assert (!channel->write_buf || channel->write_buf->len == 0);
      g_assert (channel->partial_write_buf[0] == '\0');
      
      status = channel->funcs->io_write (channel, buf, count, &tmp_bytes, error);

      if (bytes_written)
	*bytes_written = tmp_bytes;

      return status;
    }

  /* General case */

  if (channel->is_seekable && (( BUF_LEN (channel->read_buf) > 0)
    || (BUF_LEN (channel->encoded_read_buf) > 0)))
    {
      if (channel->do_encode && BUF_LEN (channel->encoded_read_buf) > 0)
        {
          g_warning("Mixed reading and writing not allowed on encoded files");
          return G_IO_STATUS_ERROR;
        }
      status = g_io_channel_seek_position (channel, 0, G_SEEK_CUR, error);
      if (status != G_IO_STATUS_NORMAL)
        {
          if (bytes_written)
            *bytes_written = 0;
          return status;
        }
    }

  if (!channel->write_buf)
    channel->write_buf = g_string_sized_new (channel->buf_size);

  while (wrote_bytes < count)
    {
      gsize space_in_buf;

      /* If the buffer is full, try a write immediately. In
       * the nonblocking case, this prevents the user from
       * writing just a little bit to the buffer every time
       * and never receiving an EAGAIN.
       */

      if (channel->write_buf->len >= channel->buf_size - MAX_CHAR_SIZE)
        {
          gsize did_write = 0, this_time;

          do
            {
              status = channel->funcs->io_write (channel, channel->write_buf->str
                                                 + did_write, channel->write_buf->len
                                                 - did_write, &this_time, error);
              did_write += this_time;
            }
          while (status == G_IO_STATUS_NORMAL &&
                 did_write < MIN (channel->write_buf->len, MAX_CHAR_SIZE));

          g_string_erase (channel->write_buf, 0, did_write);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (status == G_IO_STATUS_AGAIN && wrote_bytes > 0)
                status = G_IO_STATUS_NORMAL;
              if (bytes_written)
                *bytes_written = wrote_bytes;
              return status;
            }
        }

      space_in_buf = MAX (channel->buf_size, channel->write_buf->allocated_len - 1)
                     - channel->write_buf->len; /* 1 for NULL */

      /* This is only true because g_io_channel_set_buffer_size ()
       * ensures that channel->buf_size >= MAX_CHAR_SIZE.
       */
      g_assert (space_in_buf >= MAX_CHAR_SIZE);

      if (!channel->encoding)
        {
          gssize write_this = MIN (space_in_buf, count - wrote_bytes);

          g_string_append_len (channel->write_buf, buf, write_this);
          buf += write_this;
          wrote_bytes += write_this;
        }
      else
        {
          const gchar *from_buf;
          gsize from_buf_len, from_buf_old_len, left_len;
          gsize err;
          gint errnum;

          if (channel->partial_write_buf[0] != '\0')
            {
              g_assert (wrote_bytes == 0);

              from_buf = channel->partial_write_buf;
              from_buf_old_len = strlen (channel->partial_write_buf);
              g_assert (from_buf_old_len > 0);
              from_buf_len = MIN (6, from_buf_old_len + count);

              memcpy (channel->partial_write_buf + from_buf_old_len, buf,
                      from_buf_len - from_buf_old_len);
            }
          else
            {
              from_buf = buf;
              from_buf_len = count - wrote_bytes;
              from_buf_old_len = 0;
            }

reconvert:

          if (!channel->do_encode) /* UTF-8 encoding */
            {
              const gchar *badchar;
              gsize try_len = MIN (from_buf_len, space_in_buf);

              /* UTF-8, just validate, emulate g_iconv */

              if (!g_utf8_validate (from_buf, try_len, &badchar))
                {
                  gunichar try_char;
                  gsize incomplete_len = from_buf + try_len - badchar;

                  left_len = from_buf + from_buf_len - badchar;

                  try_char = g_utf8_get_char_validated (badchar, incomplete_len);

                  switch (try_char)
                    {
                      case -2:
                        g_assert (incomplete_len < 6);
                        if (try_len == from_buf_len)
                          {
                            errnum = EINVAL;
                            err = (gsize) -1;
                          }
                        else
                          {
                            errnum = 0;
                            err = (gsize) 0;
                          }
                        break;
                      case -1:
                        g_warning ("Invalid UTF-8 passed to g_io_channel_write_chars().");
                        /* FIXME bail here? */
                        errnum = EILSEQ;
                        err = (gsize) -1;
                        break;
                      default:
                        g_assert_not_reached ();
                        err = (gsize) -1;
                        errnum = 0; /* Don't confunse the compiler */
                    }
                }
              else
                {
                  err = (gsize) 0;
                  errnum = 0;
                  left_len = from_buf_len - try_len;
                }

              g_string_append_len (channel->write_buf, from_buf,
                                   from_buf_len - left_len);
              from_buf += from_buf_len - left_len;
            }
          else
            {
               gchar *outbuf;

               left_len = from_buf_len;
               g_string_set_size (channel->write_buf, channel->write_buf->len
                                  + space_in_buf);
               outbuf = channel->write_buf->str + channel->write_buf->len
                        - space_in_buf;
               err = g_iconv (channel->write_cd, (gchar **) &from_buf, &left_len,
                              &outbuf, &space_in_buf);
               errnum = errno;
               g_string_truncate (channel->write_buf, channel->write_buf->len
                                  - space_in_buf);
            }

          if (err == (gsize) -1)
            {
              switch (errnum)
        	{
                  case EINVAL:
                    g_assert (left_len < 6);

                    if (from_buf_old_len == 0)
                      {
                        /* Not from partial_write_buf */

                        memcpy (channel->partial_write_buf, from_buf, left_len);
                        channel->partial_write_buf[left_len] = '\0';
                        if (bytes_written)
                          *bytes_written = count;
                        return G_IO_STATUS_NORMAL;
                      }

                    /* Working in partial_write_buf */

                    if (left_len == from_buf_len)
                      {
                        /* Didn't convert anything, must still have
                         * less than a full character
                         */

                        g_assert (count == from_buf_len - from_buf_old_len);

                        channel->partial_write_buf[from_buf_len] = '\0';

                        if (bytes_written)
                          *bytes_written = count;

                        return G_IO_STATUS_NORMAL;
                      }

                    g_assert (from_buf_len - left_len >= from_buf_old_len);

                    /* We converted all the old data. This is fine */

                    break;
                  case E2BIG:
                    if (from_buf_len == left_len)
                      {
                        /* Nothing was written, add enough space for
                         * at least one character.
                         */
                        space_in_buf += MAX_CHAR_SIZE;
                        goto reconvert;
                      }
                    break;
                  case EILSEQ:
                    g_set_error_literal (error, G_CONVERT_ERROR,
                      G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                      _("Invalid byte sequence in conversion input"));
                    if (from_buf_old_len > 0 && from_buf_len == left_len)
                      g_warning ("Illegal sequence due to partial character "
                                 "at the end of a previous write.\n");
                    else
                      wrote_bytes += from_buf_len - left_len - from_buf_old_len;
                    if (bytes_written)
                      *bytes_written = wrote_bytes;
                    channel->partial_write_buf[0] = '\0';
                    return G_IO_STATUS_ERROR;
                  default:
                    g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                      _("Error during conversion: %s"), g_strerror (errnum));
                    if (from_buf_len >= left_len + from_buf_old_len)
                      wrote_bytes += from_buf_len - left_len - from_buf_old_len;
                    if (bytes_written)
                      *bytes_written = wrote_bytes;
                    channel->partial_write_buf[0] = '\0';
                    return G_IO_STATUS_ERROR;
                }
            }

          g_assert (from_buf_len - left_len >= from_buf_old_len);

          wrote_bytes += from_buf_len - left_len - from_buf_old_len;

          if (from_buf_old_len > 0)
            {
              /* We were working in partial_write_buf */

              buf += from_buf_len - left_len - from_buf_old_len;
              channel->partial_write_buf[0] = '\0';
            }
          else
            buf = from_buf;
        }
    }

  if (bytes_written)
    *bytes_written = count;

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_write_unichar:
 * @channel: a #GIOChannel
 * @thechar: a character
 * @error: location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Writes a Unicode character to @channel.
 * This function cannot be called on a channel with %NULL encoding.
 *
 * Return value: a #GIOStatus
 **/
GIOStatus
g_io_channel_write_unichar (GIOChannel  *channel,
			    gunichar     thechar,
			    GError     **error)
{
  GIOStatus status;
  gchar static_buf[6];
  gsize char_len, wrote_len;

  g_return_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->encoding != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (channel->is_writeable, G_IO_STATUS_ERROR);

  char_len = g_unichar_to_utf8 (thechar, static_buf);

  if (channel->partial_write_buf[0] != '\0')
    {
      g_warning ("Partial charater written before writing unichar.\n");
      channel->partial_write_buf[0] = '\0';
    }

  status = g_io_channel_write_chars (channel, static_buf,
                                     char_len, &wrote_len, error);

  /* We validate UTF-8, so we can't get a partial write */

  g_assert (wrote_len == char_len || status != G_IO_STATUS_NORMAL);

  return status;
}

/**
 * g_io_channel_error_quark:
 *
 * Return value: the quark used as %G_IO_CHANNEL_ERROR
 **/
GQuark
g_io_channel_error_quark (void)
{
  return g_quark_from_static_string ("g-io-channel-error-quark");
}

#define __G_IOCHANNEL_C__
#include "galiasdef.c"
