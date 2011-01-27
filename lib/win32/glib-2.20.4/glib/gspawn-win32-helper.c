/* gspawn-win32-helper.c - Helper program for process launching on Win32.
 *
 *  Copyright 2000 Red Hat, Inc.
 *  Copyright 2000 Tor Lillqvist
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

#include "config.h"

#include <fcntl.h>

#undef G_LOG_DOMAIN
#include "glib.h"
#define GSPAWN_HELPER
#include "gspawn-win32.c"	/* For shared definitions */


static void
write_err_and_exit (gint    fd,
		    gintptr msg)
{
  gintptr en = errno;
  
  write (fd, &msg, sizeof(gintptr));
  write (fd, &en, sizeof(gintptr));
  
  _exit (1);
}

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

/* We build gspawn-win32-helper.exe as a Windows GUI application
 * to avoid any temporarily flashing console windows in case
 * the gspawn function is invoked by a GUI program. Thus, no main()
 * but a WinMain(). We do, however, still use argc and argv tucked
 * away in the global __argc and __argv by the C runtime startup code.
 */

/* Info peeked from mingw runtime's source code. __wgetmainargs() is a
 * function to get the program's argv in wide char format.
 */

typedef struct {
  int newmode;
} _startupinfo;

extern void __wgetmainargs(int *argc,
			   wchar_t ***wargv,
			   wchar_t ***wenviron,
			   int expand_wildcards,
			   _startupinfo *startupinfo);

/* Copy of protect_argv that handles wchar_t strings */

static gint
protect_wargv (wchar_t  **wargv,
	       wchar_t ***new_wargv)
{
  gint i;
  gint argc = 0;
  
  while (wargv[argc])
    ++argc;
  *new_wargv = g_new (wchar_t *, argc+1);

  /* Quote each argv element if necessary, so that it will get
   * reconstructed correctly in the C runtime startup code.  Note that
   * the unquoting algorithm in the C runtime is really weird, and
   * rather different than what Unix shells do. See stdargv.c in the C
   * runtime sources (in the Platform SDK, in src/crt).
   *
   * Note that an new_wargv[0] constructed by this function should
   * *not* be passed as the filename argument to a _wspawn* or _wexec*
   * family function. That argument should be the real file name
   * without any quoting.
   */
  for (i = 0; i < argc; i++)
    {
      wchar_t *p = wargv[i];
      wchar_t *q;
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
	      wchar_t *pp = p;
	      while (*pp && *pp == '\\')
		pp++;
	      if (*pp == '"')
		len++;
	    }
	  len++;
	  p++;
	}

      q = (*new_wargv)[i] = g_new (wchar_t, len + need_dblquotes*2 + 1);
      p = wargv[i];

      if (need_dblquotes)
	*q++ = '"';

      while (*p)
	{
	  if (*p == '"')
	    *q++ = '\\';
	  else if (*p == '\\')
	    {
	      wchar_t *pp = p;
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
    }
  (*new_wargv)[argc] = NULL;

  return argc;
}

#ifndef HELPER_CONSOLE
int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	 struct HINSTANCE__ *hPrevInstance,
	 char               *lpszCmdLine,
	 int                 nCmdShow)
#else
int
main (int ignored_argc, char **ignored_argv)
#endif
{
  int child_err_report_fd = -1;
  int helper_sync_fd = -1;
  int i;
  int fd;
  int mode;
  gintptr handle;
  int saved_errno;
  gintptr no_error = CHILD_NO_ERROR;
  gint argv_zero_offset = ARG_PROGRAM;
  wchar_t **new_wargv;
  int argc;
  wchar_t **wargv, **wenvp;
  _startupinfo si = { 0 };
  char c;

  g_assert (__argc >= ARG_COUNT);

  /* Fetch the wide-char argument vector */
  __wgetmainargs (&argc, &wargv, &wenvp, 0, &si);

  /* We still have the system codepage args in __argv. We can look
   * at the first args in which gspawn-win32.c passes us flags and
   * fd numbers in __argv, as we know those are just ASCII anyway.
   */
  g_assert (argc == __argc);

  /* argv[ARG_CHILD_ERR_REPORT] is the file descriptor number onto
   * which write error messages.
   */
  child_err_report_fd = atoi (__argv[ARG_CHILD_ERR_REPORT]);

  /* Hack to implement G_SPAWN_FILE_AND_ARGV_ZERO. If
   * argv[ARG_CHILD_ERR_REPORT] is suffixed with a '#' it means we get
   * the program to run and its argv[0] separately.
   */
  if (__argv[ARG_CHILD_ERR_REPORT][strlen (__argv[ARG_CHILD_ERR_REPORT]) - 1] == '#')
    argv_zero_offset++;

  /* argv[ARG_HELPER_SYNC] is the file descriptor number we read a
   * byte that tells us it is OK to exit. We have to wait until the
   * parent allows us to exit, so that the parent has had time to
   * duplicate the process handle we sent it. Duplicating a handle
   * from another process works only if that other process exists.
   */
  helper_sync_fd = atoi (__argv[ARG_HELPER_SYNC]);

  /* argv[ARG_STDIN..ARG_STDERR] are the file descriptor numbers that
   * should be dup2'd to 0, 1 and 2. '-' if the corresponding fd
   * should be left alone, and 'z' if it should be connected to the
   * bit bucket NUL:.
   */
  if (__argv[ARG_STDIN][0] == '-')
    ; /* Nothing */
  else if (__argv[ARG_STDIN][0] == 'z')
    {
      fd = open ("NUL:", O_RDONLY);
      if (fd != 0)
	{
	  dup2 (fd, 0);
	  close (fd);
	}
    }
  else
    {
      fd = atoi (__argv[ARG_STDIN]);
      if (fd != 0)
	{
	  dup2 (fd, 0);
	  close (fd);
	}
    }

  if (__argv[ARG_STDOUT][0] == '-')
    ; /* Nothing */
  else if (__argv[ARG_STDOUT][0] == 'z')
    {
      fd = open ("NUL:", O_WRONLY);
      if (fd != 1)
	{
	  dup2 (fd, 1);
	  close (fd);
	}
    }
  else
    {
      fd = atoi (__argv[ARG_STDOUT]);
      if (fd != 1)
	{
	  dup2 (fd, 1);
	  close (fd);
	}
    }

  if (__argv[ARG_STDERR][0] == '-')
    ; /* Nothing */
  else if (__argv[ARG_STDERR][0] == 'z')
    {
      fd = open ("NUL:", O_WRONLY);
      if (fd != 2)
	{
	  dup2 (fd, 2);
	  close (fd);
	}
    }
  else
    {
      fd = atoi (__argv[ARG_STDERR]);
      if (fd != 2)
	{
	  dup2 (fd, 2);
	  close (fd);
	}
    }

  /* __argv[ARG_WORKING_DIRECTORY] is the directory in which to run the
   * process.  If "-", don't change directory.
   */
  if (__argv[ARG_WORKING_DIRECTORY][0] == '-' &&
      __argv[ARG_WORKING_DIRECTORY][1] == 0)
    ; /* Nothing */
  else if (_wchdir (wargv[ARG_WORKING_DIRECTORY]) < 0)
    write_err_and_exit (child_err_report_fd, CHILD_CHDIR_FAILED);

  /* __argv[ARG_CLOSE_DESCRIPTORS] is "y" if file descriptors from 3
   *  upwards should be closed
   */
  if (__argv[ARG_CLOSE_DESCRIPTORS][0] == 'y')
    for (i = 3; i < 1000; i++)	/* FIXME real limit? */
      if (i != child_err_report_fd && i != helper_sync_fd)
	close (i);

  /* We don't want our child to inherit the error report and
   * helper sync fds.
   */
  child_err_report_fd = dup_noninherited (child_err_report_fd, _O_WRONLY);
  helper_sync_fd = dup_noninherited (helper_sync_fd, _O_RDONLY);

  /* __argv[ARG_WAIT] is "w" to wait for the program to exit */
  if (__argv[ARG_WAIT][0] == 'w')
    mode = P_WAIT;
  else
    mode = P_NOWAIT;

  /* __argv[ARG_USE_PATH] is "y" to use PATH, otherwise not */

  /* __argv[ARG_PROGRAM] is executable file to run,
   * __argv[argv_zero_offset]... is its argv. argv_zero_offset equals
   * ARG_PROGRAM unless G_SPAWN_FILE_AND_ARGV_ZERO was used, in which
   * case we have a separate executable name and argv[0].
   */

  /* For the program name passed to spawnv(), don't use the quoted
   * version.
   */
  protect_wargv (wargv + argv_zero_offset, &new_wargv);

  if (__argv[ARG_USE_PATH][0] == 'y')
    handle = _wspawnvp (mode, wargv[ARG_PROGRAM], (const wchar_t **) new_wargv);
  else
    handle = _wspawnv (mode, wargv[ARG_PROGRAM], (const wchar_t **) new_wargv);

  saved_errno = errno;

  if (handle == -1 && saved_errno != 0)
    write_err_and_exit (child_err_report_fd, CHILD_SPAWN_FAILED);

  write (child_err_report_fd, &no_error, sizeof (no_error));
  write (child_err_report_fd, &handle, sizeof (handle));

  read (helper_sync_fd, &c, 1);

  return 0;
}
