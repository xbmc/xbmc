/* gspawn-win32.c - Process launching on Win32
 *
 *  Copyright 2000 Red Hat, Inc.
 *  Copyright 2003 Tor Lillqvist
 *
 * GLib is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not, write
 * to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Implementation details on Win32.
 *
 * - There is no way to set the no-inherit flag for
 *   a "file descriptor" in the MS C runtime. The flag is there,
 *   and the dospawn() function uses it, but unfortunately
 *   this flag can only be set when opening the file.
 * - As there is no fork(), we cannot reliably change directory
 *   before starting the child process. (There might be several threads
 *   running, and the current directory is common for all threads.)
 *
 * Thus, we must in many cases use a helper program to handle closing
 * of (inherited) file descriptors and changing of directory. The
 * helper process is also needed if the standard input, standard
 * output, or standard error of the process to be run are supposed to
 * be redirected somewhere.
 *
 * The structure of the source code in this file is a mess, I know.
 */

/* Define this to get some logging all the time */
/* #define G_SPAWN_WIN32_DEBUG */

#include "config.h"

#include "glib.h"
#include "gprintfint.h"
#include "glibintl.h"
#include "galias.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <direct.h>
#include <wchar.h>

#ifdef G_SPAWN_WIN32_DEBUG
  static int debug = 1;
  #define SETUP_DEBUG() /* empty */
#else
  static int debug = -1;
  #define SETUP_DEBUG()					\
    G_STMT_START					\
      {							\
	if (debug == -1)				\
	  {						\
	    if (getenv ("G_SPAWN_WIN32_DEBUG") != NULL)	\
	      debug = 1;				\
	    else					\
	      debug = 0;				\
	  }						\
      }							\
    G_STMT_END
#endif

enum
{
  CHILD_NO_ERROR,
  CHILD_CHDIR_FAILED,
  CHILD_SPAWN_FAILED,
};

enum {
  ARG_CHILD_ERR_REPORT = 1,
  ARG_HELPER_SYNC,
  ARG_STDIN,
  ARG_STDOUT,
  ARG_STDERR,
  ARG_WORKING_DIRECTORY,
  ARG_CLOSE_DESCRIPTORS,
  ARG_USE_PATH,
  ARG_WAIT,
  ARG_PROGRAM,
  ARG_COUNT = ARG_PROGRAM
};

static int
dup_noninherited (int fd,
		  int mode)
{
  HANDLE filehandle;

  DuplicateHandle (GetCurrentProcess (), (LPHANDLE) _get_osfhandle (fd),
		   GetCurrentProcess (), &filehandle,
		   0, FALSE, DUPLICATE_SAME_ACCESS);
  close (fd);
  return _open_osfhandle ((gintptr) filehandle, mode | _O_NOINHERIT);
}

#ifndef GSPAWN_HELPER

#ifdef _WIN64
#define HELPER_PROCESS "gspawn-win64-helper"
#else
#define HELPER_PROCESS "gspawn-win32-helper"
#endif

static gchar *
protect_argv_string (const gchar *string)
{
  const gchar *p = string;
  gchar *retval, *q;
  gint len = 0;
  gboolean need_dblquotes = FALSE;
  while (*p)
    {
      if (*p == ' ' || *p == '\t')
	need_dblquotes = TRUE;
      else if (*p == '"')
	len++;
      else if (*p == '\\')
	{
	  const gchar *pp = p;
	  while (*pp && *pp == '\\')
	    pp++;
	  if (*pp == '"')
	    len++;
	}
      len++;
      p++;
    }
  
  q = retval = g_malloc (len + need_dblquotes*2 + 1);
  p = string;

  if (need_dblquotes)
    *q++ = '"';
  
  while (*p)
    {
      if (*p == '"')
	*q++ = '\\';
      else if (*p == '\\')
	{
	  const gchar *pp = p;
	  while (*pp && *pp == '\\')
	    pp++;
	  if (*pp == '"')
	    *q++ = '\\';
	}
      *q++ = *p;
      p++;
    }
  
  if (need_dblquotes)
    *q++ = '"';
  *q++ = '\0';

  return retval;
}

static gint
protect_argv (gchar  **argv,
	      gchar ***new_argv)
{
  gint i;
  gint argc = 0;
  
  while (argv[argc])
    ++argc;
  *new_argv = g_new (gchar *, argc+1);

  /* Quote each argv element if necessary, so that it will get
   * reconstructed correctly in the C runtime startup code.  Note that
   * the unquoting algorithm in the C runtime is really weird, and
   * rather different than what Unix shells do. See stdargv.c in the C
   * runtime sources (in the Platform SDK, in src/crt).
   *
   * Note that an new_argv[0] constructed by this function should
   * *not* be passed as the filename argument to a spawn* or exec*
   * family function. That argument should be the real file name
   * without any quoting.
   */
  for (i = 0; i < argc; i++)
    (*new_argv)[i] = protect_argv_string (argv[i]);

  (*new_argv)[argc] = NULL;

  return argc;
}

GQuark
g_spawn_error_quark (void)
{
  return g_quark_from_static_string ("g-exec-error-quark");
}

gboolean
g_spawn_async_utf8 (const gchar          *working_directory,
		    gchar               **argv,
		    gchar               **envp,
		    GSpawnFlags           flags,
		    GSpawnChildSetupFunc  child_setup,
		    gpointer              user_data,
		    GPid                 *child_handle,
		    GError              **error)
{
  g_return_val_if_fail (argv != NULL, FALSE);
  
  return g_spawn_async_with_pipes_utf8 (working_directory,
					argv, envp,
					flags,
					child_setup,
					user_data,
					child_handle,
					NULL, NULL, NULL,
					error);
}

/* Avoids a danger in threaded situations (calling close()
 * on a file descriptor twice, and another thread has
 * re-opened it since the first close)
 */
static void
close_and_invalidate (gint *fd)
{
  if (*fd < 0)
    return;

  close (*fd);
  *fd = -1;
}

typedef enum
{
  READ_FAILED = 0, /* FALSE */
  READ_OK,
  READ_EOF
} ReadResult;

static ReadResult
read_data (GString     *str,
           GIOChannel  *iochannel,
           GError     **error)
{
  GIOStatus giostatus;
  gsize bytes;
  gchar buf[4096];

 again:
  
  giostatus = g_io_channel_read_chars (iochannel, buf, sizeof (buf), &bytes, NULL);

  if (bytes == 0)
    return READ_EOF;
  else if (bytes > 0)
    {
      g_string_append_len (str, buf, bytes);
      return READ_OK;
    }
  else if (giostatus == G_IO_STATUS_AGAIN)
    goto again;
  else if (giostatus == G_IO_STATUS_ERROR)
    {
      g_set_error_literal (error, G_SPAWN_ERROR, G_SPAWN_ERROR_READ,
                           _("Failed to read data from child process"));
      
      return READ_FAILED;
    }
  else
    return READ_OK;
}

static gboolean
make_pipe (gint     p[2],
           GError **error)
{
  if (_pipe (p, 4096, _O_BINARY) < 0)
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
                   _("Failed to create pipe for communicating with child process (%s)"),
                   g_strerror (errno));
      return FALSE;
    }
  else
    return TRUE;
}

/* The helper process writes a status report back to us, through a
 * pipe, consisting of two ints.
 */
static gboolean
read_helper_report (int      fd,
		    gintptr  report[2],
		    GError **error)
{
  gint bytes = 0;
  
  while (bytes < sizeof(gintptr)*2)
    {
      gint chunk;

      if (debug)
	g_print ("%s:read_helper_report: read %" G_GSIZE_FORMAT "...\n",
		 __FILE__,
		 sizeof(gintptr)*2 - bytes);

      chunk = read (fd, ((gchar*)report) + bytes,
		    sizeof(gintptr)*2 - bytes);

      if (debug)
	g_print ("...got %d bytes\n", chunk);
          
      if (chunk < 0)
        {
          /* Some weird shit happened, bail out */
              
          g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
                       _("Failed to read from child pipe (%s)"),
                       g_strerror (errno));

          return FALSE;
        }
      else if (chunk == 0)
	{
	  g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		       _("Failed to read from child pipe (%s)"),
		       "EOF");
	  break; /* EOF */
	}
      else
	bytes += chunk;
    }

  if (bytes < sizeof(gintptr)*2)
    return FALSE;

  return TRUE;
}

static void
set_child_error (gintptr      report[2],
		 const gchar *working_directory,
		 GError     **error)
{
  switch (report[0])
    {
    case CHILD_CHDIR_FAILED:
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_CHDIR,
		   _("Failed to change to directory '%s' (%s)"),
		   working_directory,
		   g_strerror (report[1]));
      break;
    case CHILD_SPAWN_FAILED:
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Failed to execute child process (%s)"),
		   g_strerror (report[1]));
      break;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
utf8_charv_to_wcharv (char     **utf8_charv,
		      wchar_t ***wcharv,
		      int       *error_index,
		      GError   **error)
{
  wchar_t **retval = NULL;

  *wcharv = NULL;
  if (utf8_charv != NULL)
    {
      int n = 0, i;

      while (utf8_charv[n])
	n++;
      retval = g_new (wchar_t *, n + 1);

      for (i = 0; i < n; i++)
	{
	  retval[i] = g_utf8_to_utf16 (utf8_charv[i], -1, NULL, NULL, error);
	  if (retval[i] == NULL)
	    {
	      if (error_index)
		*error_index = i;
	      while (i)
		g_free (retval[--i]);
	      g_free (retval);
	      return FALSE;
	    }
	}
	    
      retval[n] = NULL;
    }
  *wcharv = retval;
  return TRUE;
}

static gboolean
do_spawn_directly (gint                 *exit_status,
		   gboolean		 do_return_handle,
		   GSpawnFlags           flags,
		   gchar               **argv,
		   char                **envp,
		   char                **protected_argv,
		   GPid                 *child_handle,
		   GError              **error)     
{
  const int mode = (exit_status == NULL) ? P_NOWAIT : P_WAIT;
  char **new_argv;
  gintptr rc = -1;
  int saved_errno;
  GError *conv_error = NULL;
  gint conv_error_index;
  wchar_t *wargv0, **wargv, **wenvp;

  new_argv = (flags & G_SPAWN_FILE_AND_ARGV_ZERO) ? protected_argv + 1 : protected_argv;
      
  wargv0 = g_utf8_to_utf16 (argv[0], -1, NULL, NULL, &conv_error);
  if (wargv0 == NULL)
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Invalid program name: %s"),
		   conv_error->message);
      g_error_free (conv_error);
      
      return FALSE;
    }
  
  if (!utf8_charv_to_wcharv (new_argv, &wargv, &conv_error_index, &conv_error))
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Invalid string in argument vector at %d: %s"),
		   conv_error_index, conv_error->message);
      g_error_free (conv_error);
      g_free (wargv0);

      return FALSE;
    }

  if (!utf8_charv_to_wcharv (envp, &wenvp, NULL, &conv_error))
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Invalid string in environment: %s"),
		   conv_error->message);
      g_error_free (conv_error);
      g_free (wargv0);
      g_strfreev ((gchar **) wargv);

      return FALSE;
    }

  if (flags & G_SPAWN_SEARCH_PATH)
    if (wenvp != NULL)
      rc = _wspawnvpe (mode, wargv0, (const wchar_t **) wargv, (const wchar_t **) wenvp);
    else
      rc = _wspawnvp (mode, wargv0, (const wchar_t **) wargv);
  else
    if (wenvp != NULL)
      rc = _wspawnve (mode, wargv0, (const wchar_t **) wargv, (const wchar_t **) wenvp);
    else
      rc = _wspawnv (mode, wargv0, (const wchar_t **) wargv);

  g_free (wargv0);
  g_strfreev ((gchar **) wargv);
  g_strfreev ((gchar **) wenvp);

  saved_errno = errno;

  if (rc == -1 && saved_errno != 0)
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Failed to execute child process (%s)"),
		   g_strerror (saved_errno));
      return FALSE;
    }

  if (exit_status == NULL)
    {
      if (child_handle && do_return_handle)
	*child_handle = (GPid) rc;
      else
	{
	  CloseHandle ((HANDLE) rc);
	  if (child_handle)
	    *child_handle = 0;
	}
    }
  else
    *exit_status = rc;

  return TRUE;
}

static gboolean
do_spawn_with_pipes (gint                 *exit_status,
		     gboolean		   do_return_handle,
		     const gchar          *working_directory,
		     gchar               **argv,
		     char                **envp,
		     GSpawnFlags           flags,
		     GSpawnChildSetupFunc  child_setup,
		     GPid                 *child_handle,
		     gint                 *standard_input,
		     gint                 *standard_output,
		     gint                 *standard_error,
		     gint		  *err_report,
		     GError              **error)     
{
  char **protected_argv;
  char args[ARG_COUNT][10];
  char **new_argv;
  int i;
  gintptr rc = -1;
  int saved_errno;
  int argc;
  int stdin_pipe[2] = { -1, -1 };
  int stdout_pipe[2] = { -1, -1 };
  int stderr_pipe[2] = { -1, -1 };
  int child_err_report_pipe[2] = { -1, -1 };
  int helper_sync_pipe[2] = { -1, -1 };
  gintptr helper_report[2];
  static gboolean warned_about_child_setup = FALSE;
  GError *conv_error = NULL;
  gint conv_error_index;
  gchar *helper_process;
  CONSOLE_CURSOR_INFO cursor_info;
  wchar_t *whelper, **wargv, **wenvp;
  extern gchar *_glib_get_dll_directory (void);
  gchar *glib_dll_directory;

  if (child_setup && !warned_about_child_setup)
    {
      warned_about_child_setup = TRUE;
      g_warning ("passing a child setup function to the g_spawn functions is pointless on Windows and it is ignored");
    }

  argc = protect_argv (argv, &protected_argv);

  if (!standard_input && !standard_output && !standard_error &&
      (flags & G_SPAWN_CHILD_INHERITS_STDIN) &&
      !(flags & G_SPAWN_STDOUT_TO_DEV_NULL) &&
      !(flags & G_SPAWN_STDERR_TO_DEV_NULL) &&
      (working_directory == NULL || !*working_directory) &&
      (flags & G_SPAWN_LEAVE_DESCRIPTORS_OPEN))
    {
      /* We can do without the helper process */
      gboolean retval =
	do_spawn_directly (exit_status, do_return_handle, flags,
			   argv, envp, protected_argv,
			   child_handle, error);
      g_strfreev (protected_argv);
      return retval;
    }

  if (standard_input && !make_pipe (stdin_pipe, error))
    goto cleanup_and_fail;
  
  if (standard_output && !make_pipe (stdout_pipe, error))
    goto cleanup_and_fail;
  
  if (standard_error && !make_pipe (stderr_pipe, error))
    goto cleanup_and_fail;
  
  if (!make_pipe (child_err_report_pipe, error))
    goto cleanup_and_fail;
  
  if (!make_pipe (helper_sync_pipe, error))
    goto cleanup_and_fail;
  
  new_argv = g_new (char *, argc + 1 + ARG_COUNT);
  if (GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE), &cursor_info))
    helper_process = HELPER_PROCESS "-console.exe";
  else
    helper_process = HELPER_PROCESS ".exe";
  
  glib_dll_directory = _glib_get_dll_directory ();
  if (glib_dll_directory != NULL)
    {
      helper_process = g_build_filename (glib_dll_directory, helper_process, NULL);
      g_free (glib_dll_directory);
    }
  else
    helper_process = g_strdup (helper_process);

  new_argv[0] = protect_argv_string (helper_process);

  _g_sprintf (args[ARG_CHILD_ERR_REPORT], "%d", child_err_report_pipe[1]);
  new_argv[ARG_CHILD_ERR_REPORT] = args[ARG_CHILD_ERR_REPORT];
  
  /* Make the read end of the child error report pipe
   * noninherited. Otherwise it will needlessly be inherited by the
   * helper process, and the started actual user process. As such that
   * shouldn't harm, but it is unnecessary.
   */
  child_err_report_pipe[0] = dup_noninherited (child_err_report_pipe[0], _O_RDONLY);

  if (flags & G_SPAWN_FILE_AND_ARGV_ZERO)
    {
      /* Overload ARG_CHILD_ERR_REPORT to also encode the
       * G_SPAWN_FILE_AND_ARGV_ZERO functionality.
       */
      strcat (args[ARG_CHILD_ERR_REPORT], "#");
    }
  
  _g_sprintf (args[ARG_HELPER_SYNC], "%d", helper_sync_pipe[0]);
  new_argv[ARG_HELPER_SYNC] = args[ARG_HELPER_SYNC];
  
  /* Make the write end of the sync pipe noninherited. Otherwise the
   * helper process will inherit it, and thus if this process happens
   * to crash before writing the sync byte to the pipe, the helper
   * process won't read but won't get any EOF either, as it has the
   * write end open itself.
   */
  helper_sync_pipe[1] = dup_noninherited (helper_sync_pipe[1], _O_WRONLY);

  if (standard_input)
    {
      _g_sprintf (args[ARG_STDIN], "%d", stdin_pipe[0]);
      new_argv[ARG_STDIN] = args[ARG_STDIN];
    }
  else if (flags & G_SPAWN_CHILD_INHERITS_STDIN)
    {
      /* Let stdin be alone */
      new_argv[ARG_STDIN] = "-";
    }
  else
    {
      /* Keep process from blocking on a read of stdin */
      new_argv[ARG_STDIN] = "z";
    }
  
  if (standard_output)
    {
      _g_sprintf (args[ARG_STDOUT], "%d", stdout_pipe[1]);
      new_argv[ARG_STDOUT] = args[ARG_STDOUT];
    }
  else if (flags & G_SPAWN_STDOUT_TO_DEV_NULL)
    {
      new_argv[ARG_STDOUT] = "z";
    }
  else
    {
      new_argv[ARG_STDOUT] = "-";
    }
  
  if (standard_error)
    {
      _g_sprintf (args[ARG_STDERR], "%d", stderr_pipe[1]);
      new_argv[ARG_STDERR] = args[ARG_STDERR];
    }
  else if (flags & G_SPAWN_STDERR_TO_DEV_NULL)
    {
      new_argv[ARG_STDERR] = "z";
    }
  else
    {
      new_argv[ARG_STDERR] = "-";
    }
  
  if (working_directory && *working_directory)
    new_argv[ARG_WORKING_DIRECTORY] = protect_argv_string (working_directory);
  else
    new_argv[ARG_WORKING_DIRECTORY] = g_strdup ("-");
  
  if (!(flags & G_SPAWN_LEAVE_DESCRIPTORS_OPEN))
    new_argv[ARG_CLOSE_DESCRIPTORS] = "y";
  else
    new_argv[ARG_CLOSE_DESCRIPTORS] = "-";

  if (flags & G_SPAWN_SEARCH_PATH)
    new_argv[ARG_USE_PATH] = "y";
  else
    new_argv[ARG_USE_PATH] = "-";

  if (exit_status == NULL)
    new_argv[ARG_WAIT] = "-";
  else
    new_argv[ARG_WAIT] = "w";

  for (i = 0; i <= argc; i++)
    new_argv[ARG_PROGRAM + i] = protected_argv[i];

  SETUP_DEBUG();

  if (debug)
    {
      g_print ("calling %s with argv:\n", helper_process);
      for (i = 0; i < argc + 1 + ARG_COUNT; i++)
	g_print ("argv[%d]: %s\n", i, (new_argv[i] ? new_argv[i] : "NULL"));
    }

  if (!utf8_charv_to_wcharv (new_argv, &wargv, &conv_error_index, &conv_error))
    {
      if (conv_error_index == ARG_WORKING_DIRECTORY)
	g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_CHDIR,
		     _("Invalid working directory: %s"),
		     conv_error->message);
      else
	g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		     _("Invalid string in argument vector at %d: %s"),
		     conv_error_index - ARG_PROGRAM, conv_error->message);
      g_error_free (conv_error);
      g_strfreev (protected_argv);
      g_free (new_argv[0]);
      g_free (new_argv[ARG_WORKING_DIRECTORY]);
      g_free (new_argv);
      g_free (helper_process);

      goto cleanup_and_fail;
    }

  if (!utf8_charv_to_wcharv (envp, &wenvp, NULL, &conv_error))
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Invalid string in environment: %s"),
		   conv_error->message);
      g_error_free (conv_error);
      g_strfreev (protected_argv);
      g_free (new_argv[0]);
      g_free (new_argv[ARG_WORKING_DIRECTORY]);
      g_free (new_argv);
      g_free (helper_process);
      g_strfreev ((gchar **) wargv);
 
      goto cleanup_and_fail;
    }

  whelper = g_utf8_to_utf16 (helper_process, -1, NULL, NULL, NULL);
  g_free (helper_process);

  if (wenvp != NULL)
    rc = _wspawnvpe (P_NOWAIT, whelper, (const wchar_t **) wargv, (const wchar_t **) wenvp);
  else
    rc = _wspawnvp (P_NOWAIT, whelper, (const wchar_t **) wargv);

  saved_errno = errno;

  g_free (whelper);
  g_strfreev ((gchar **) wargv);
  g_strfreev ((gchar **) wenvp);

  /* Close the other process's ends of the pipes in this process,
   * otherwise the reader will never get EOF.
   */
  close_and_invalidate (&child_err_report_pipe[1]);
  close_and_invalidate (&helper_sync_pipe[0]);
  close_and_invalidate (&stdin_pipe[0]);
  close_and_invalidate (&stdout_pipe[1]);
  close_and_invalidate (&stderr_pipe[1]);

  g_strfreev (protected_argv);

  g_free (new_argv[0]);
  g_free (new_argv[ARG_WORKING_DIRECTORY]);
  g_free (new_argv);

  /* Check if gspawn-win32-helper couldn't be run */
  if (rc == -1 && saved_errno != 0)
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		   _("Failed to execute helper program (%s)"),
		   g_strerror (saved_errno));
      goto cleanup_and_fail;
    }

  if (exit_status != NULL)
    {
      /* Synchronous case. Pass helper's report pipe back to caller,
       * which takes care of reading it after the grandchild has
       * finished.
       */
      g_assert (err_report != NULL);
      *err_report = child_err_report_pipe[0];
      write (helper_sync_pipe[1], " ", 1);
      close_and_invalidate (&helper_sync_pipe[1]);
    }
  else
    {
      /* Asynchronous case. We read the helper's report right away. */
      if (!read_helper_report (child_err_report_pipe[0], helper_report, error))
	goto cleanup_and_fail;
        
      close_and_invalidate (&child_err_report_pipe[0]);

      switch (helper_report[0])
	{
	case CHILD_NO_ERROR:
	  if (child_handle && do_return_handle)
	    {
	      /* rc is our HANDLE for gspawn-win32-helper. It has
	       * told us the HANDLE of its child. Duplicate that into
	       * a HANDLE valid in this process.
	       */
	      if (!DuplicateHandle ((HANDLE) rc, (HANDLE) helper_report[1],
				    GetCurrentProcess (), (LPHANDLE) child_handle,
				    0, TRUE, DUPLICATE_SAME_ACCESS))
		{
		  char *emsg = g_win32_error_message (GetLastError ());
		  g_print("%s\n", emsg);
		  *child_handle = 0;
		}
	    }
	  else if (child_handle)
	    *child_handle = 0;
	  write (helper_sync_pipe[1], " ", 1);
	  close_and_invalidate (&helper_sync_pipe[1]);
	  break;
	  
	default:
	  write (helper_sync_pipe[1], " ", 1);
	  close_and_invalidate (&helper_sync_pipe[1]);
	  set_child_error (helper_report, working_directory, error);
	  goto cleanup_and_fail;
	}
    }

  /* Success against all odds! return the information */
      
  if (standard_input)
    *standard_input = stdin_pipe[1];
  if (standard_output)
    *standard_output = stdout_pipe[0];
  if (standard_error)
    *standard_error = stderr_pipe[0];
  if (rc != -1)
    CloseHandle ((HANDLE) rc);
  
  return TRUE;

 cleanup_and_fail:

  if (rc != -1)
    CloseHandle ((HANDLE) rc);
  if (child_err_report_pipe[0] != -1)
    close (child_err_report_pipe[0]);
  if (child_err_report_pipe[1] != -1)
    close (child_err_report_pipe[1]);
  if (helper_sync_pipe[0] != -1)
    close (helper_sync_pipe[0]);
  if (helper_sync_pipe[1] != -1)
    close (helper_sync_pipe[1]);
  if (stdin_pipe[0] != -1)
    close (stdin_pipe[0]);
  if (stdin_pipe[1] != -1)
    close (stdin_pipe[1]);
  if (stdout_pipe[0] != -1)
    close (stdout_pipe[0]);
  if (stdout_pipe[1] != -1)
    close (stdout_pipe[1]);
  if (stderr_pipe[0] != -1)
    close (stderr_pipe[0]);
  if (stderr_pipe[1] != -1)
    close (stderr_pipe[1]);

  return FALSE;
}

gboolean
g_spawn_sync_utf8 (const gchar          *working_directory,
		   gchar               **argv,
		   gchar               **envp,
		   GSpawnFlags           flags,
		   GSpawnChildSetupFunc  child_setup,
		   gpointer              user_data,
		   gchar               **standard_output,
		   gchar               **standard_error,
		   gint                 *exit_status,
		   GError              **error)     
{
  gint outpipe = -1;
  gint errpipe = -1;
  gint reportpipe = -1;
  GIOChannel *outchannel = NULL;
  GIOChannel *errchannel = NULL;
  GPollFD outfd, errfd;
  GPollFD fds[2];
  gint nfds;
  gint outindex = -1;
  gint errindex = -1;
  gint ret;
  GString *outstr = NULL;
  GString *errstr = NULL;
  gboolean failed;
  gint status;
  
  g_return_val_if_fail (argv != NULL, FALSE);
  g_return_val_if_fail (!(flags & G_SPAWN_DO_NOT_REAP_CHILD), FALSE);
  g_return_val_if_fail (standard_output == NULL ||
                        !(flags & G_SPAWN_STDOUT_TO_DEV_NULL), FALSE);
  g_return_val_if_fail (standard_error == NULL ||
                        !(flags & G_SPAWN_STDERR_TO_DEV_NULL), FALSE);
  
  /* Just to ensure segfaults if callers try to use
   * these when an error is reported.
   */
  if (standard_output)
    *standard_output = NULL;

  if (standard_error)
    *standard_error = NULL;
  
  if (!do_spawn_with_pipes (&status,
			    FALSE,
			    working_directory,
			    argv,
			    envp,
			    flags,
			    child_setup,
			    NULL,
			    NULL,
			    standard_output ? &outpipe : NULL,
			    standard_error ? &errpipe : NULL,
			    &reportpipe,
			    error))
    return FALSE;

  /* Read data from child. */
  
  failed = FALSE;

  if (outpipe >= 0)
    {
      outstr = g_string_new (NULL);
      outchannel = g_io_channel_win32_new_fd (outpipe);
      g_io_channel_set_encoding (outchannel, NULL, NULL);
      g_io_channel_set_buffered (outchannel, FALSE);
      g_io_channel_win32_make_pollfd (outchannel,
				      G_IO_IN | G_IO_ERR | G_IO_HUP,
				      &outfd);
      if (debug)
	g_print ("outfd=%p\n", (HANDLE) outfd.fd);
    }
      
  if (errpipe >= 0)
    {
      errstr = g_string_new (NULL);
      errchannel = g_io_channel_win32_new_fd (errpipe);
      g_io_channel_set_encoding (errchannel, NULL, NULL);
      g_io_channel_set_buffered (errchannel, FALSE);
      g_io_channel_win32_make_pollfd (errchannel,
				      G_IO_IN | G_IO_ERR | G_IO_HUP,
				      &errfd);
      if (debug)
	g_print ("errfd=%p\n", (HANDLE) errfd.fd);
    }

  /* Read data until we get EOF on all pipes. */
  while (!failed && (outpipe >= 0 || errpipe >= 0))
    {
      nfds = 0;
      if (outpipe >= 0)
	{
	  fds[nfds] = outfd;
	  outindex = nfds;
	  nfds++;
	}
      if (errpipe >= 0)
	{
	  fds[nfds] = errfd;
	  errindex = nfds;
	  nfds++;
	}

      if (debug)
	g_print ("g_spawn_sync: calling g_io_channel_win32_poll, nfds=%d\n",
		 nfds);

      ret = g_io_channel_win32_poll (fds, nfds, -1);

      if (ret < 0)
        {
          failed = TRUE;

          g_set_error_literal (error, G_SPAWN_ERROR, G_SPAWN_ERROR_READ,
                               _("Unexpected error in g_io_channel_win32_poll() reading data from a child process"));
	  
          break;
        }

      if (outpipe >= 0 && (fds[outindex].revents & G_IO_IN))
        {
          switch (read_data (outstr, outchannel, error))
            {
            case READ_FAILED:
	      if (debug)
		g_print ("g_spawn_sync: outchannel: READ_FAILED\n");
              failed = TRUE;
              break;
            case READ_EOF:
	      if (debug)
		g_print ("g_spawn_sync: outchannel: READ_EOF\n");
              g_io_channel_unref (outchannel);
	      outchannel = NULL;
              close_and_invalidate (&outpipe);
              break;
            default:
	      if (debug)
		g_print ("g_spawn_sync: outchannel: OK\n");
              break;
            }

          if (failed)
            break;
        }

      if (errpipe >= 0 && (fds[errindex].revents & G_IO_IN))
        {
          switch (read_data (errstr, errchannel, error))
            {
            case READ_FAILED:
	      if (debug)
		g_print ("g_spawn_sync: errchannel: READ_FAILED\n");
              failed = TRUE;
              break;
            case READ_EOF:
	      if (debug)
		g_print ("g_spawn_sync: errchannel: READ_EOF\n");
	      g_io_channel_unref (errchannel);
	      errchannel = NULL;
              close_and_invalidate (&errpipe);
              break;
            default:
	      if (debug)
		g_print ("g_spawn_sync: errchannel: OK\n");
              break;
            }

          if (failed)
            break;
        }
    }

  if (reportpipe == -1)
    {
      /* No helper process, exit status of actual spawned process
       * already available.
       */
      if (exit_status)
        *exit_status = status;
    }
  else
    {
      /* Helper process was involved. Read its report now after the
       * grandchild has finished.
       */
      gintptr helper_report[2];

      if (!read_helper_report (reportpipe, helper_report, error))
	failed = TRUE;
      else
	{
	  switch (helper_report[0])
	    {
	    case CHILD_NO_ERROR:
	      if (exit_status)
		*exit_status = helper_report[1];
	      break;
	    default:
	      set_child_error (helper_report, working_directory, error);
	      failed = TRUE;
	      break;
	    }
	}
      close_and_invalidate (&reportpipe);
    }


  /* These should only be open still if we had an error.  */
  
  if (outchannel != NULL)
    g_io_channel_unref (outchannel);
  if (errchannel != NULL)
    g_io_channel_unref (errchannel);
  if (outpipe >= 0)
    close_and_invalidate (&outpipe);
  if (errpipe >= 0)
    close_and_invalidate (&errpipe);
  
  if (failed)
    {
      if (outstr)
        g_string_free (outstr, TRUE);
      if (errstr)
        g_string_free (errstr, TRUE);

      return FALSE;
    }
  else
    {
      if (standard_output)        
        *standard_output = g_string_free (outstr, FALSE);

      if (standard_error)
        *standard_error = g_string_free (errstr, FALSE);

      return TRUE;
    }
}

gboolean
g_spawn_async_with_pipes_utf8 (const gchar          *working_directory,
			       gchar               **argv,
			       gchar               **envp,
			       GSpawnFlags           flags,
			       GSpawnChildSetupFunc  child_setup,
			       gpointer              user_data,
			       GPid                 *child_handle,
			       gint                 *standard_input,
			       gint                 *standard_output,
			       gint                 *standard_error,
			       GError              **error)
{
  g_return_val_if_fail (argv != NULL, FALSE);
  g_return_val_if_fail (standard_output == NULL ||
                        !(flags & G_SPAWN_STDOUT_TO_DEV_NULL), FALSE);
  g_return_val_if_fail (standard_error == NULL ||
                        !(flags & G_SPAWN_STDERR_TO_DEV_NULL), FALSE);
  /* can't inherit stdin if we have an input pipe. */
  g_return_val_if_fail (standard_input == NULL ||
                        !(flags & G_SPAWN_CHILD_INHERITS_STDIN), FALSE);
  
  return do_spawn_with_pipes (NULL,
			      (flags & G_SPAWN_DO_NOT_REAP_CHILD),
			      working_directory,
			      argv,
			      envp,
			      flags,
			      child_setup,
			      child_handle,
			      standard_input,
			      standard_output,
			      standard_error,
			      NULL,
			      error);
}

gboolean
g_spawn_command_line_sync_utf8 (const gchar  *command_line,
				gchar       **standard_output,
				gchar       **standard_error,
				gint         *exit_status,
				GError      **error)
{
  gboolean retval;
  gchar **argv = 0;

  g_return_val_if_fail (command_line != NULL, FALSE);
  
  if (!g_shell_parse_argv (command_line,
                           NULL, &argv,
                           error))
    return FALSE;
  
  retval = g_spawn_sync_utf8 (NULL,
			      argv,
			      NULL,
			      G_SPAWN_SEARCH_PATH,
			      NULL,
			      NULL,
			      standard_output,
			      standard_error,
			      exit_status,
			      error);
  g_strfreev (argv);

  return retval;
}

gboolean
g_spawn_command_line_async_utf8 (const gchar *command_line,
				 GError     **error)
{
  gboolean retval;
  gchar **argv = 0;

  g_return_val_if_fail (command_line != NULL, FALSE);

  if (!g_shell_parse_argv (command_line,
                           NULL, &argv,
                           error))
    return FALSE;
  
  retval = g_spawn_async_utf8 (NULL,
			       argv,
			       NULL,
			       G_SPAWN_SEARCH_PATH,
			       NULL,
			       NULL,
			       NULL,
			       error);
  g_strfreev (argv);

  return retval;
}

void
g_spawn_close_pid (GPid pid)
{
    CloseHandle (pid);
}

#if !defined (_WIN64)

/* Binary compatibility versions that take system codepage pathnames,
 * argument vectors and environments. These get used only by code
 * built against 2.8.1 or earlier. Code built against 2.8.2 or later
 * will use the _utf8 versions above (see the #defines in gspawn.h).
 */

#undef g_spawn_async
#undef g_spawn_async_with_pipes
#undef g_spawn_sync
#undef g_spawn_command_line_sync
#undef g_spawn_command_line_async

static gboolean
setup_utf8_copies (const gchar *working_directory,
		   gchar      **utf8_working_directory,
		   gchar      **argv,
		   gchar     ***utf8_argv,
		   gchar      **envp,
		   gchar     ***utf8_envp,
		   GError     **error)
{
  gint i, argc, envc;

  if (working_directory == NULL)
    *utf8_working_directory = NULL;
  else
    {
      GError *conv_error = NULL;
      
      *utf8_working_directory = g_locale_to_utf8 (working_directory, -1, NULL, NULL, &conv_error);
      if (*utf8_working_directory == NULL)
	{
	  g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_CHDIR,
		       _("Invalid working directory: %s"),
		       conv_error->message);
	  g_error_free (conv_error);
	  return FALSE;
	}
    }

  argc = 0;
  while (argv[argc])
    ++argc;
  *utf8_argv = g_new (gchar *, argc + 1);
  for (i = 0; i < argc; i++)
    {
      GError *conv_error = NULL;

      (*utf8_argv)[i] = g_locale_to_utf8 (argv[i], -1, NULL, NULL, &conv_error);
      if ((*utf8_argv)[i] == NULL)
	{
	  g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
		       _("Invalid string in argument vector at %d: %s"),
		       i, conv_error->message);
	  g_error_free (conv_error);
	  
	  g_strfreev (*utf8_argv);
	  *utf8_argv = NULL;

	  g_free (*utf8_working_directory);
	  *utf8_working_directory = NULL;

	  return FALSE;
	}
    }
  (*utf8_argv)[argc] = NULL;

  if (envp == NULL)
    {
      *utf8_envp = NULL;
    }
  else
    {
      envc = 0;
      while (envp[envc])
	++envc;
      *utf8_envp = g_new (gchar *, envc + 1);
      for (i = 0; i < envc; i++)
	{
	  GError *conv_error = NULL;

	  (*utf8_envp)[i] = g_locale_to_utf8 (envp[i], -1, NULL, NULL, &conv_error);
	  if ((*utf8_envp)[i] == NULL)
	    {	
	      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
			   _("Invalid string in environment: %s"),
			   conv_error->message);
	      g_error_free (conv_error);

	      g_strfreev (*utf8_envp);
	      *utf8_envp = NULL;

	      g_strfreev (*utf8_argv);
	      *utf8_argv = NULL;

	      g_free (*utf8_working_directory);
	      *utf8_working_directory = NULL;

	      return FALSE;
	    }
	}
      (*utf8_envp)[envc] = NULL;
    }
  return TRUE;
}

static void
free_utf8_copies (gchar  *utf8_working_directory,
		  gchar **utf8_argv,
		  gchar **utf8_envp)
{
  g_free (utf8_working_directory);
  g_strfreev (utf8_argv);
  g_strfreev (utf8_envp);
}

gboolean
g_spawn_async_with_pipes (const gchar          *working_directory,
                          gchar               **argv,
                          gchar               **envp,
                          GSpawnFlags           flags,
                          GSpawnChildSetupFunc  child_setup,
                          gpointer              user_data,
                          GPid                 *child_handle,
                          gint                 *standard_input,
                          gint                 *standard_output,
                          gint                 *standard_error,
                          GError              **error)
{
  gchar *utf8_working_directory;
  gchar **utf8_argv;
  gchar **utf8_envp;
  gboolean retval;

  if (!setup_utf8_copies (working_directory, &utf8_working_directory,
			  argv, &utf8_argv,
			  envp, &utf8_envp,
			  error))
    return FALSE;

  retval = g_spawn_async_with_pipes_utf8 (utf8_working_directory,
					  utf8_argv, utf8_envp,
					  flags, child_setup, user_data,
					  child_handle,
					  standard_input, standard_output, standard_error,
					  error);

  free_utf8_copies (utf8_working_directory, utf8_argv, utf8_envp);

  return retval;
}

gboolean
g_spawn_async (const gchar          *working_directory,
	       gchar               **argv,
	       gchar               **envp,
	       GSpawnFlags           flags,
	       GSpawnChildSetupFunc  child_setup,
	       gpointer              user_data,
	       GPid                 *child_handle,
	       GError              **error)
{
  return g_spawn_async_with_pipes (working_directory,
				   argv, envp,
				   flags,
				   child_setup,
				   user_data,
				   child_handle,
				   NULL, NULL, NULL,
				   error);
}

gboolean
g_spawn_sync (const gchar          *working_directory,
	      gchar               **argv,
	      gchar               **envp,
	      GSpawnFlags           flags,
	      GSpawnChildSetupFunc  child_setup,
	      gpointer              user_data,
	      gchar               **standard_output,
	      gchar               **standard_error,
	      gint                 *exit_status,
	      GError              **error)     
{
  gchar *utf8_working_directory;
  gchar **utf8_argv;
  gchar **utf8_envp;
  gboolean retval;

  if (!setup_utf8_copies (working_directory, &utf8_working_directory,
			  argv, &utf8_argv,
			  envp, &utf8_envp,
			  error))
    return FALSE;

  retval = g_spawn_sync_utf8 (utf8_working_directory,
			      utf8_argv, utf8_envp,
			      flags, child_setup, user_data,
			      standard_output, standard_error, exit_status,
			      error);

  free_utf8_copies (utf8_working_directory, utf8_argv, utf8_envp);

  return retval;
}

gboolean
g_spawn_command_line_sync (const gchar  *command_line,
			   gchar       **standard_output,
			   gchar       **standard_error,
			   gint         *exit_status,
			   GError      **error)
{
  gboolean retval;
  gchar **argv = 0;

  g_return_val_if_fail (command_line != NULL, FALSE);
  
  if (!g_shell_parse_argv (command_line,
                           NULL, &argv,
                           error))
    return FALSE;
  
  retval = g_spawn_sync (NULL,
                         argv,
                         NULL,
                         G_SPAWN_SEARCH_PATH,
                         NULL,
                         NULL,
                         standard_output,
                         standard_error,
                         exit_status,
                         error);
  g_strfreev (argv);

  return retval;
}

gboolean
g_spawn_command_line_async (const gchar *command_line,
			    GError     **error)
{
  gboolean retval;
  gchar **argv = 0;

  g_return_val_if_fail (command_line != NULL, FALSE);

  if (!g_shell_parse_argv (command_line,
                           NULL, &argv,
                           error))
    return FALSE;
  
  retval = g_spawn_async (NULL,
                          argv,
                          NULL,
                          G_SPAWN_SEARCH_PATH,
                          NULL,
                          NULL,
                          NULL,
                          error);
  g_strfreev (argv);

  return retval;
}

#endif	/* !_WIN64 */

#endif /* !GSPAWN_HELPER */

#define __G_SPAWN_C__
#include "galiasdef.c"
