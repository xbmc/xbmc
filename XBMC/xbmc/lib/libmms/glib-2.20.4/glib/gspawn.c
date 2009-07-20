/* gspawn.c - Process launching
 *
 *  Copyright 2000 Red Hat, Inc.
 *  g_execvpe implementation based on GNU libc execvp:
 *   Copyright 1991, 92, 95, 96, 97, 98, 99 Free Software Foundation, Inc.
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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>   /* for fdwalk */
#include <dirent.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#include "glib.h"
#include "glibintl.h"
#include "galias.h"

static gint g_execute (const gchar  *file,
                       gchar **argv,
                       gchar **envp,
                       gboolean search_path);

static gboolean make_pipe            (gint                  p[2],
                                      GError              **error);
static gboolean fork_exec_with_pipes (gboolean              intermediate_child,
                                      const gchar          *working_directory,
                                      gchar               **argv,
                                      gchar               **envp,
                                      gboolean              close_descriptors,
                                      gboolean              search_path,
                                      gboolean              stdout_to_null,
                                      gboolean              stderr_to_null,
                                      gboolean              child_inherits_stdin,
                                      gboolean              file_and_argv_zero,
                                      GSpawnChildSetupFunc  child_setup,
                                      gpointer              user_data,
                                      GPid                 *child_pid,
                                      gint                 *standard_input,
                                      gint                 *standard_output,
                                      gint                 *standard_error,
                                      GError              **error);

GQuark
g_spawn_error_quark (void)
{
  return g_quark_from_static_string ("g-exec-error-quark");
}

/**
 * g_spawn_async:
 * @working_directory: child's current working directory, or %NULL to inherit parent's
 * @argv: child's argument vector
 * @envp: child's environment, or %NULL to inherit parent's
 * @flags: flags from #GSpawnFlags
 * @child_setup: function to run in the child just before exec()
 * @user_data: user data for @child_setup
 * @child_pid: return location for child process reference, or %NULL
 * @error: return location for error
 * 
 * See g_spawn_async_with_pipes() for a full description; this function
 * simply calls the g_spawn_async_with_pipes() without any pipes.
 *
 * You should call g_spawn_close_pid() on the returned child process
 * reference when you don't need it any more.
 * 
 * <note><para>
 * If you are writing a GTK+ application, and the program you 
 * are spawning is a graphical application, too, then you may
 * want to use gdk_spawn_on_screen() instead to ensure that 
 * the spawned program opens its windows on the right screen.
 * </para></note>
 *
 * <note><para> Note that the returned @child_pid on Windows is a
 * handle to the child process and not its identifier. Process handles
 * and process identifiers are different concepts on Windows.
 * </para></note>
 *
 * Return value: %TRUE on success, %FALSE if error is set
 **/
gboolean
g_spawn_async (const gchar          *working_directory,
               gchar               **argv,
               gchar               **envp,
               GSpawnFlags           flags,
               GSpawnChildSetupFunc  child_setup,
               gpointer              user_data,
               GPid                 *child_pid,
               GError              **error)
{
  g_return_val_if_fail (argv != NULL, FALSE);
  
  return g_spawn_async_with_pipes (working_directory,
                                   argv, envp,
                                   flags,
                                   child_setup,
                                   user_data,
                                   child_pid,
                                   NULL, NULL, NULL,
                                   error);
}

/* Avoids a danger in threaded situations (calling close()
 * on a file descriptor twice, and another thread has
 * re-opened it since the first close)
 */
static gint
close_and_invalidate (gint *fd)
{
  gint ret;

  if (*fd < 0)
    return -1;
  else
    {
      ret = close (*fd);
      *fd = -1;
    }

  return ret;
}

/* Some versions of OS X define READ_OK in public headers */
#undef READ_OK

typedef enum
{
  READ_FAILED = 0, /* FALSE */
  READ_OK,
  READ_EOF
} ReadResult;

static ReadResult
read_data (GString *str,
           gint     fd,
           GError **error)
{
  gssize bytes;        
  gchar buf[4096];    

 again:
  
  bytes = read (fd, buf, 4096);

  if (bytes == 0)
    return READ_EOF;
  else if (bytes > 0)
    {
      g_string_append_len (str, buf, bytes);
      return READ_OK;
    }
  else if (bytes < 0 && errno == EINTR)
    goto again;
  else if (bytes < 0)
    {
      g_set_error (error,
                   G_SPAWN_ERROR,
                   G_SPAWN_ERROR_READ,
                   _("Failed to read data from child process (%s)"),
                   g_strerror (errno));
      
      return READ_FAILED;
    }
  else
    return READ_OK;
}

/**
 * g_spawn_sync:
 * @working_directory: child's current working directory, or %NULL to inherit parent's
 * @argv: child's argument vector
 * @envp: child's environment, or %NULL to inherit parent's
 * @flags: flags from #GSpawnFlags 
 * @child_setup: function to run in the child just before exec()
 * @user_data: user data for @child_setup
 * @standard_output: return location for child output, or %NULL
 * @standard_error: return location for child error messages, or %NULL
 * @exit_status: return location for child exit status, as returned by waitpid(), or %NULL
 * @error: return location for error, or %NULL
 *
 * Executes a child synchronously (waits for the child to exit before returning).
 * All output from the child is stored in @standard_output and @standard_error,
 * if those parameters are non-%NULL. Note that you must set the  
 * %G_SPAWN_STDOUT_TO_DEV_NULL and %G_SPAWN_STDERR_TO_DEV_NULL flags when
 * passing %NULL for @standard_output and @standard_error.
 * If @exit_status is non-%NULL, the exit status of the child is stored
 * there as it would be returned by waitpid(); standard UNIX macros such 
 * as WIFEXITED() and WEXITSTATUS() must be used to evaluate the exit status.
 * Note that this function call waitpid() even if @exit_status is %NULL, and
 * does not accept the %G_SPAWN_DO_NOT_REAP_CHILD flag.
 * If an error occurs, no data is returned in @standard_output, 
 * @standard_error, or @exit_status. 
 *
 * This function calls g_spawn_async_with_pipes() internally; see that
 * function for full details on the other parameters and details on
 * how these functions work on Windows.
 * 
 * Return value: %TRUE on success, %FALSE if an error was set.
 **/
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
  gint outpipe = -1;
  gint errpipe = -1;
  GPid pid;
  fd_set fds;
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
  
  if (!fork_exec_with_pipes (FALSE,
                             working_directory,
                             argv,
                             envp,
                             !(flags & G_SPAWN_LEAVE_DESCRIPTORS_OPEN),
                             (flags & G_SPAWN_SEARCH_PATH) != 0,
                             (flags & G_SPAWN_STDOUT_TO_DEV_NULL) != 0,
                             (flags & G_SPAWN_STDERR_TO_DEV_NULL) != 0,
                             (flags & G_SPAWN_CHILD_INHERITS_STDIN) != 0,
                             (flags & G_SPAWN_FILE_AND_ARGV_ZERO) != 0,
                             child_setup,
                             user_data,
                             &pid,
                             NULL,
                             standard_output ? &outpipe : NULL,
                             standard_error ? &errpipe : NULL,
                             error))
    return FALSE;

  /* Read data from child. */
  
  failed = FALSE;

  if (outpipe >= 0)
    {
      outstr = g_string_new (NULL);
    }
      
  if (errpipe >= 0)
    {
      errstr = g_string_new (NULL);
    }

  /* Read data until we get EOF on both pipes. */
  while (!failed &&
         (outpipe >= 0 ||
          errpipe >= 0))
    {
      ret = 0;
          
      FD_ZERO (&fds);
      if (outpipe >= 0)
        FD_SET (outpipe, &fds);
      if (errpipe >= 0)
        FD_SET (errpipe, &fds);
          
      ret = select (MAX (outpipe, errpipe) + 1,
                    &fds,
                    NULL, NULL,
                    NULL /* no timeout */);

      if (ret < 0 && errno != EINTR)
        {
          failed = TRUE;

          g_set_error (error,
                       G_SPAWN_ERROR,
                       G_SPAWN_ERROR_READ,
                       _("Unexpected error in select() reading data from a child process (%s)"),
                       g_strerror (errno));
              
          break;
        }

      if (outpipe >= 0 && FD_ISSET (outpipe, &fds))
        {
          switch (read_data (outstr, outpipe, error))
            {
            case READ_FAILED:
              failed = TRUE;
              break;
            case READ_EOF:
              close_and_invalidate (&outpipe);
              outpipe = -1;
              break;
            default:
              break;
            }

          if (failed)
            break;
        }

      if (errpipe >= 0 && FD_ISSET (errpipe, &fds))
        {
          switch (read_data (errstr, errpipe, error))
            {
            case READ_FAILED:
              failed = TRUE;
              break;
            case READ_EOF:
              close_and_invalidate (&errpipe);
              errpipe = -1;
              break;
            default:
              break;
            }

          if (failed)
            break;
        }
    }

  /* These should only be open still if we had an error.  */
  
  if (outpipe >= 0)
    close_and_invalidate (&outpipe);
  if (errpipe >= 0)
    close_and_invalidate (&errpipe);
  
  /* Wait for child to exit, even if we have
   * an error pending.
   */
 again:
      
  ret = waitpid (pid, &status, 0);

  if (ret < 0)
    {
      if (errno == EINTR)
        goto again;
      else if (errno == ECHILD)
        {
          if (exit_status)
            {
              g_warning ("In call to g_spawn_sync(), exit status of a child process was requested but SIGCHLD action was set to SIG_IGN and ECHILD was received by waitpid(), so exit status can't be returned. This is a bug in the program calling g_spawn_sync(); either don't request the exit status, or don't set the SIGCHLD action.");
            }
          else
            {
              /* We don't need the exit status. */
            }
        }
      else
        {
          if (!failed) /* avoid error pileups */
            {
              failed = TRUE;
                  
              g_set_error (error,
                           G_SPAWN_ERROR,
                           G_SPAWN_ERROR_READ,
                           _("Unexpected error in waitpid() (%s)"),
                           g_strerror (errno));
            }
        }
    }
  
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
      if (exit_status)
        *exit_status = status;
      
      if (standard_output)        
        *standard_output = g_string_free (outstr, FALSE);

      if (standard_error)
        *standard_error = g_string_free (errstr, FALSE);

      return TRUE;
    }
}

/**
 * g_spawn_async_with_pipes:
 * @working_directory: child's current working directory, or %NULL to inherit parent's, in the GLib file name encoding
 * @argv: child's argument vector, in the GLib file name encoding
 * @envp: child's environment, or %NULL to inherit parent's, in the GLib file name encoding
 * @flags: flags from #GSpawnFlags
 * @child_setup: function to run in the child just before exec()
 * @user_data: user data for @child_setup
 * @child_pid: return location for child process ID, or %NULL
 * @standard_input: return location for file descriptor to write to child's stdin, or %NULL
 * @standard_output: return location for file descriptor to read child's stdout, or %NULL
 * @standard_error: return location for file descriptor to read child's stderr, or %NULL
 * @error: return location for error
 *
 * Executes a child program asynchronously (your program will not
 * block waiting for the child to exit). The child program is
 * specified by the only argument that must be provided, @argv. @argv
 * should be a %NULL-terminated array of strings, to be passed as the
 * argument vector for the child. The first string in @argv is of
 * course the name of the program to execute. By default, the name of
 * the program must be a full path; the <envar>PATH</envar> shell variable 
 * will only be searched if you pass the %G_SPAWN_SEARCH_PATH flag.
 *
 * On Windows, note that all the string or string vector arguments to
 * this function and the other g_spawn*() functions are in UTF-8, the
 * GLib file name encoding. Unicode characters that are not part of
 * the system codepage passed in these arguments will be correctly
 * available in the spawned program only if it uses wide character API
 * to retrieve its command line. For C programs built with Microsoft's
 * tools it is enough to make the program have a wmain() instead of
 * main(). wmain() has a wide character argument vector as parameter.
 *
 * At least currently, mingw doesn't support wmain(), so if you use
 * mingw to develop the spawned program, it will have to call the
 * undocumented function __wgetmainargs() to get the wide character
 * argument vector and environment. See gspawn-win32-helper.c in the
 * GLib sources or init.c in the mingw runtime sources for a prototype
 * for that function. Alternatively, you can retrieve the Win32 system
 * level wide character command line passed to the spawned program
 * using the GetCommandLineW() function.
 *
 * On Windows the low-level child process creation API
 * <function>CreateProcess()</function> doesn't use argument vectors,
 * but a command line. The C runtime library's
 * <function>spawn*()</function> family of functions (which
 * g_spawn_async_with_pipes() eventually calls) paste the argument
 * vector elements together into a command line, and the C runtime startup code
 * does a corresponding reconstruction of an argument vector from the
 * command line, to be passed to main(). Complications arise when you have
 * argument vector elements that contain spaces of double quotes. The
 * <function>spawn*()</function> functions don't do any quoting or
 * escaping, but on the other hand the startup code does do unquoting
 * and unescaping in order to enable receiving arguments with embedded
 * spaces or double quotes. To work around this asymmetry,
 * g_spawn_async_with_pipes() will do quoting and escaping on argument
 * vector elements that need it before calling the C runtime
 * spawn() function.
 *
 * The returned @child_pid on Windows is a handle to the child
 * process, not its identifier. Process handles and process
 * identifiers are different concepts on Windows.
 *
 * @envp is a %NULL-terminated array of strings, where each string
 * has the form <literal>KEY=VALUE</literal>. This will become
 * the child's environment. If @envp is %NULL, the child inherits its
 * parent's environment.
 *
 * @flags should be the bitwise OR of any flags you want to affect the
 * function's behaviour. The %G_SPAWN_DO_NOT_REAP_CHILD means that 
 * the child will not automatically be reaped; you must use a
 * #GChildWatch source to be notified about the death of the child 
 * process. Eventually you must call g_spawn_close_pid() on the
 * @child_pid, in order to free resources which may be associated
 * with the child process. (On Unix, using a #GChildWatch source is
 * equivalent to calling waitpid() or handling the %SIGCHLD signal 
 * manually. On Windows, calling g_spawn_close_pid() is equivalent
 * to calling CloseHandle() on the process handle returned in 
 * @child_pid).
 *
 * %G_SPAWN_LEAVE_DESCRIPTORS_OPEN means that the parent's open file
 * descriptors will be inherited by the child; otherwise all
 * descriptors except stdin/stdout/stderr will be closed before
 * calling exec() in the child. %G_SPAWN_SEARCH_PATH 
 * means that <literal>argv[0]</literal> need not be an absolute path, it
 * will be looked for in the user's <envar>PATH</envar>. 
 * %G_SPAWN_STDOUT_TO_DEV_NULL means that the child's standard output will 
 * be discarded, instead of going to the same location as the parent's 
 * standard output. If you use this flag, @standard_output must be %NULL.
 * %G_SPAWN_STDERR_TO_DEV_NULL means that the child's standard error
 * will be discarded, instead of going to the same location as the parent's
 * standard error. If you use this flag, @standard_error must be %NULL.
 * %G_SPAWN_CHILD_INHERITS_STDIN means that the child will inherit the parent's
 * standard input (by default, the child's standard input is attached to
 * /dev/null). If you use this flag, @standard_input must be %NULL.
 * %G_SPAWN_FILE_AND_ARGV_ZERO means that the first element of @argv is
 * the file to execute, while the remaining elements are the
 * actual argument vector to pass to the file. Normally
 * g_spawn_async_with_pipes() uses @argv[0] as the file to execute, and
 * passes all of @argv to the child.
 *
 * @child_setup and @user_data are a function and user data. On POSIX
 * platforms, the function is called in the child after GLib has
 * performed all the setup it plans to perform (including creating
 * pipes, closing file descriptors, etc.) but before calling
 * exec(). That is, @child_setup is called just
 * before calling exec() in the child. Obviously
 * actions taken in this function will only affect the child, not the
 * parent.
 *
 * On Windows, there is no separate fork() and exec()
 * functionality. Child processes are created and run with a single
 * API call, CreateProcess(). There is no sensible thing @child_setup
 * could be used for on Windows so it is ignored and not called.
 *
 * If non-%NULL, @child_pid will on Unix be filled with the child's
 * process ID. You can use the process ID to send signals to the
 * child, or to use g_child_watch_add() (or waitpid()) if you specified the
 * %G_SPAWN_DO_NOT_REAP_CHILD flag. On Windows, @child_pid will be
 * filled with a handle to the child process only if you specified the
 * %G_SPAWN_DO_NOT_REAP_CHILD flag. You can then access the child
 * process using the Win32 API, for example wait for its termination
 * with the <function>WaitFor*()</function> functions, or examine its
 * exit code with GetExitCodeProcess(). You should close the handle 
 * with CloseHandle() or g_spawn_close_pid() when you no longer need it.
 *
 * If non-%NULL, the @standard_input, @standard_output, @standard_error
 * locations will be filled with file descriptors for writing to the child's
 * standard input or reading from its standard output or standard error.
 * The caller of g_spawn_async_with_pipes() must close these file descriptors
 * when they are no longer in use. If these parameters are %NULL, the corresponding
 * pipe won't be created.
 *
 * If @standard_input is NULL, the child's standard input is attached to 
 * /dev/null unless %G_SPAWN_CHILD_INHERITS_STDIN is set.
 *
 * If @standard_error is NULL, the child's standard error goes to the same 
 * location as the parent's standard error unless %G_SPAWN_STDERR_TO_DEV_NULL 
 * is set.
 *
 * If @standard_output is NULL, the child's standard output goes to the same 
 * location as the parent's standard output unless %G_SPAWN_STDOUT_TO_DEV_NULL 
 * is set.
 *
 * @error can be %NULL to ignore errors, or non-%NULL to report errors.
 * If an error is set, the function returns %FALSE. Errors
 * are reported even if they occur in the child (for example if the
 * executable in <literal>argv[0]</literal> is not found). Typically
 * the <literal>message</literal> field of returned errors should be displayed
 * to users. Possible errors are those from the #G_SPAWN_ERROR domain.
 *
 * If an error occurs, @child_pid, @standard_input, @standard_output,
 * and @standard_error will not be filled with valid values.
 *
 * If @child_pid is not %NULL and an error does not occur then the returned
 * process reference must be closed using g_spawn_close_pid().
 *
 * <note><para>
 * If you are writing a GTK+ application, and the program you 
 * are spawning is a graphical application, too, then you may
 * want to use gdk_spawn_on_screen_with_pipes() instead to ensure that 
 * the spawned program opens its windows on the right screen.
 * </para></note>
 * 
 * Return value: %TRUE on success, %FALSE if an error was set
 **/
gboolean
g_spawn_async_with_pipes (const gchar          *working_directory,
                          gchar               **argv,
                          gchar               **envp,
                          GSpawnFlags           flags,
                          GSpawnChildSetupFunc  child_setup,
                          gpointer              user_data,
                          GPid                 *child_pid,
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
  
  return fork_exec_with_pipes (!(flags & G_SPAWN_DO_NOT_REAP_CHILD),
                               working_directory,
                               argv,
                               envp,
                               !(flags & G_SPAWN_LEAVE_DESCRIPTORS_OPEN),
                               (flags & G_SPAWN_SEARCH_PATH) != 0,
                               (flags & G_SPAWN_STDOUT_TO_DEV_NULL) != 0,
                               (flags & G_SPAWN_STDERR_TO_DEV_NULL) != 0,
                               (flags & G_SPAWN_CHILD_INHERITS_STDIN) != 0,
                               (flags & G_SPAWN_FILE_AND_ARGV_ZERO) != 0,
                               child_setup,
                               user_data,
                               child_pid,
                               standard_input,
                               standard_output,
                               standard_error,
                               error);
}

/**
 * g_spawn_command_line_sync:
 * @command_line: a command line 
 * @standard_output: return location for child output
 * @standard_error: return location for child errors
 * @exit_status: return location for child exit status, as returned by waitpid()
 * @error: return location for errors
 *
 * A simple version of g_spawn_sync() with little-used parameters
 * removed, taking a command line instead of an argument vector.  See
 * g_spawn_sync() for full details. @command_line will be parsed by
 * g_shell_parse_argv(). Unlike g_spawn_sync(), the %G_SPAWN_SEARCH_PATH flag
 * is enabled. Note that %G_SPAWN_SEARCH_PATH can have security
 * implications, so consider using g_spawn_sync() directly if
 * appropriate. Possible errors are those from g_spawn_sync() and those
 * from g_shell_parse_argv().
 *
 * If @exit_status is non-%NULL, the exit status of the child is stored there as
 * it would be returned by waitpid(); standard UNIX macros such as WIFEXITED()
 * and WEXITSTATUS() must be used to evaluate the exit status.
 * 
 * On Windows, please note the implications of g_shell_parse_argv()
 * parsing @command_line. Parsing is done according to Unix shell rules, not 
 * Windows command interpreter rules.
 * Space is a separator, and backslashes are
 * special. Thus you cannot simply pass a @command_line containing
 * canonical Windows paths, like "c:\\program files\\app\\app.exe", as
 * the backslashes will be eaten, and the space will act as a
 * separator. You need to enclose such paths with single quotes, like
 * "'c:\\program files\\app\\app.exe' 'e:\\folder\\argument.txt'".
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 **/
gboolean
g_spawn_command_line_sync (const gchar  *command_line,
                           gchar       **standard_output,
                           gchar       **standard_error,
                           gint         *exit_status,
                           GError      **error)
{
  gboolean retval;
  gchar **argv = NULL;

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

/**
 * g_spawn_command_line_async:
 * @command_line: a command line
 * @error: return location for errors
 * 
 * A simple version of g_spawn_async() that parses a command line with
 * g_shell_parse_argv() and passes it to g_spawn_async(). Runs a
 * command line in the background. Unlike g_spawn_async(), the
 * %G_SPAWN_SEARCH_PATH flag is enabled, other flags are not. Note
 * that %G_SPAWN_SEARCH_PATH can have security implications, so
 * consider using g_spawn_async() directly if appropriate. Possible
 * errors are those from g_shell_parse_argv() and g_spawn_async().
 * 
 * The same concerns on Windows apply as for g_spawn_command_line_sync().
 *
 * Return value: %TRUE on success, %FALSE if error is set.
 **/
gboolean
g_spawn_command_line_async (const gchar *command_line,
                            GError     **error)
{
  gboolean retval;
  gchar **argv = NULL;

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

static gint
exec_err_to_g_error (gint en)
{
  switch (en)
    {
#ifdef EACCES
    case EACCES:
      return G_SPAWN_ERROR_ACCES;
      break;
#endif

#ifdef EPERM
    case EPERM:
      return G_SPAWN_ERROR_PERM;
      break;
#endif

#ifdef E2BIG
    case E2BIG:
      return G_SPAWN_ERROR_2BIG;
      break;
#endif

#ifdef ENOEXEC
    case ENOEXEC:
      return G_SPAWN_ERROR_NOEXEC;
      break;
#endif

#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
      return G_SPAWN_ERROR_NAMETOOLONG;
      break;
#endif

#ifdef ENOENT
    case ENOENT:
      return G_SPAWN_ERROR_NOENT;
      break;
#endif

#ifdef ENOMEM
    case ENOMEM:
      return G_SPAWN_ERROR_NOMEM;
      break;
#endif

#ifdef ENOTDIR
    case ENOTDIR:
      return G_SPAWN_ERROR_NOTDIR;
      break;
#endif

#ifdef ELOOP
    case ELOOP:
      return G_SPAWN_ERROR_LOOP;
      break;
#endif
      
#ifdef ETXTBUSY
    case ETXTBUSY:
      return G_SPAWN_ERROR_TXTBUSY;
      break;
#endif

#ifdef EIO
    case EIO:
      return G_SPAWN_ERROR_IO;
      break;
#endif

#ifdef ENFILE
    case ENFILE:
      return G_SPAWN_ERROR_NFILE;
      break;
#endif

#ifdef EMFILE
    case EMFILE:
      return G_SPAWN_ERROR_MFILE;
      break;
#endif

#ifdef EINVAL
    case EINVAL:
      return G_SPAWN_ERROR_INVAL;
      break;
#endif

#ifdef EISDIR
    case EISDIR:
      return G_SPAWN_ERROR_ISDIR;
      break;
#endif

#ifdef ELIBBAD
    case ELIBBAD:
      return G_SPAWN_ERROR_LIBBAD;
      break;
#endif
      
    default:
      return G_SPAWN_ERROR_FAILED;
      break;
    }
}

static gssize
write_all (gint fd, gconstpointer vbuf, gsize to_write)
{
  gchar *buf = (gchar *) vbuf;
  
  while (to_write > 0)
    {
      gssize count = write (fd, buf, to_write);
      if (count < 0)
        {
          if (errno != EINTR)
            return FALSE;
        }
      else
        {
          to_write -= count;
          buf += count;
        }
    }
  
  return TRUE;
}

G_GNUC_NORETURN
static void
write_err_and_exit (gint fd, gint msg)
{
  gint en = errno;
  
  write_all (fd, &msg, sizeof(msg));
  write_all (fd, &en, sizeof(en));
  
  _exit (1);
}

static int
set_cloexec (void *data, gint fd)
{
  if (fd >= GPOINTER_TO_INT (data))
    fcntl (fd, F_SETFD, FD_CLOEXEC);

  return 0;
}

#ifndef HAVE_FDWALK
static int
fdwalk (int (*cb)(void *data, int fd), void *data)
{
  gint open_max;
  gint fd;
  gint res = 0;
  
#ifdef HAVE_SYS_RESOURCE_H
  struct rlimit rl;
#endif

#ifdef __linux__  
  DIR *d;

  if ((d = opendir("/proc/self/fd"))) {
      struct dirent *de;

      while ((de = readdir(d))) {
          glong l;
          gchar *e = NULL;

          if (de->d_name[0] == '.')
              continue;
            
          errno = 0;
          l = strtol(de->d_name, &e, 10);
          if (errno != 0 || !e || *e)
              continue;

          fd = (gint) l;

          if ((glong) fd != l)
              continue;

          if (fd == dirfd(d))
              continue;

          if ((res = cb (data, fd)) != 0)
              break;
        }
      
      closedir(d);
      return res;
  }

  /* If /proc is not mounted or not accessible we fall back to the old
   * rlimit trick */

#endif
  
#ifdef HAVE_SYS_RESOURCE_H
      
  if (getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_max != RLIM_INFINITY)
      open_max = rl.rlim_max;
  else
#endif
      open_max = sysconf (_SC_OPEN_MAX);

  for (fd = 0; fd < open_max; fd++)
      if ((res = cb (data, fd)) != 0)
          break;

  return res;
}
#endif

static gint
sane_dup2 (gint fd1, gint fd2)
{
  gint ret;

 retry:
  ret = dup2 (fd1, fd2);
  if (ret < 0 && errno == EINTR)
    goto retry;

  return ret;
}

enum
{
  CHILD_CHDIR_FAILED,
  CHILD_EXEC_FAILED,
  CHILD_DUP2_FAILED,
  CHILD_FORK_FAILED
};

static void
do_exec (gint                  child_err_report_fd,
         gint                  stdin_fd,
         gint                  stdout_fd,
         gint                  stderr_fd,
         const gchar          *working_directory,
         gchar               **argv,
         gchar               **envp,
         gboolean              close_descriptors,
         gboolean              search_path,
         gboolean              stdout_to_null,
         gboolean              stderr_to_null,
         gboolean              child_inherits_stdin,
         gboolean              file_and_argv_zero,
         GSpawnChildSetupFunc  child_setup,
         gpointer              user_data)
{
  if (working_directory && chdir (working_directory) < 0)
    write_err_and_exit (child_err_report_fd,
                        CHILD_CHDIR_FAILED);

  /* Close all file descriptors but stdin stdout and stderr as
   * soon as we exec. Note that this includes
   * child_err_report_fd, which keeps the parent from blocking
   * forever on the other end of that pipe.
   */
  if (close_descriptors)
    {
      fdwalk (set_cloexec, GINT_TO_POINTER(3));
    }
  else
    {
      /* We need to do child_err_report_fd anyway */
      set_cloexec (GINT_TO_POINTER(0), child_err_report_fd);
    }
  
  /* Redirect pipes as required */
  
  if (stdin_fd >= 0)
    {
      /* dup2 can't actually fail here I don't think */
          
      if (sane_dup2 (stdin_fd, 0) < 0)
        write_err_and_exit (child_err_report_fd,
                            CHILD_DUP2_FAILED);

      /* ignore this if it doesn't work */
      close_and_invalidate (&stdin_fd);
    }
  else if (!child_inherits_stdin)
    {
      /* Keep process from blocking on a read of stdin */
      gint read_null = open ("/dev/null", O_RDONLY);
      sane_dup2 (read_null, 0);
      close_and_invalidate (&read_null);
    }

  if (stdout_fd >= 0)
    {
      /* dup2 can't actually fail here I don't think */
          
      if (sane_dup2 (stdout_fd, 1) < 0)
        write_err_and_exit (child_err_report_fd,
                            CHILD_DUP2_FAILED);

      /* ignore this if it doesn't work */
      close_and_invalidate (&stdout_fd);
    }
  else if (stdout_to_null)
    {
      gint write_null = open ("/dev/null", O_WRONLY);
      sane_dup2 (write_null, 1);
      close_and_invalidate (&write_null);
    }

  if (stderr_fd >= 0)
    {
      /* dup2 can't actually fail here I don't think */
          
      if (sane_dup2 (stderr_fd, 2) < 0)
        write_err_and_exit (child_err_report_fd,
                            CHILD_DUP2_FAILED);

      /* ignore this if it doesn't work */
      close_and_invalidate (&stderr_fd);
    }
  else if (stderr_to_null)
    {
      gint write_null = open ("/dev/null", O_WRONLY);
      sane_dup2 (write_null, 2);
      close_and_invalidate (&write_null);
    }
  
  /* Call user function just before we exec */
  if (child_setup)
    {
      (* child_setup) (user_data);
    }

  g_execute (argv[0],
             file_and_argv_zero ? argv + 1 : argv,
             envp, search_path);

  /* Exec failed */
  write_err_and_exit (child_err_report_fd,
                      CHILD_EXEC_FAILED);
}

static gboolean
read_ints (int      fd,
           gint*    buf,
           gint     n_ints_in_buf,    
           gint    *n_ints_read,      
           GError **error)
{
  gsize bytes = 0;    
  
  while (TRUE)
    {
      gssize chunk;    

      if (bytes >= sizeof(gint)*2)
        break; /* give up, who knows what happened, should not be
                * possible.
                */
          
    again:
      chunk = read (fd,
                    ((gchar*)buf) + bytes,
                    sizeof(gint) * n_ints_in_buf - bytes);
      if (chunk < 0 && errno == EINTR)
        goto again;
          
      if (chunk < 0)
        {
          /* Some weird shit happened, bail out */
              
          g_set_error (error,
                       G_SPAWN_ERROR,
                       G_SPAWN_ERROR_FAILED,
                       _("Failed to read from child pipe (%s)"),
                       g_strerror (errno));

          return FALSE;
        }
      else if (chunk == 0)
        break; /* EOF */
      else /* chunk > 0 */
	bytes += chunk;
    }

  *n_ints_read = (gint)(bytes / sizeof(gint));

  return TRUE;
}

static gboolean
fork_exec_with_pipes (gboolean              intermediate_child,
                      const gchar          *working_directory,
                      gchar               **argv,
                      gchar               **envp,
                      gboolean              close_descriptors,
                      gboolean              search_path,
                      gboolean              stdout_to_null,
                      gboolean              stderr_to_null,
                      gboolean              child_inherits_stdin,
                      gboolean              file_and_argv_zero,
                      GSpawnChildSetupFunc  child_setup,
                      gpointer              user_data,
                      GPid                 *child_pid,
                      gint                 *standard_input,
                      gint                 *standard_output,
                      gint                 *standard_error,
                      GError              **error)     
{
  GPid pid = -1;
  gint stdin_pipe[2] = { -1, -1 };
  gint stdout_pipe[2] = { -1, -1 };
  gint stderr_pipe[2] = { -1, -1 };
  gint child_err_report_pipe[2] = { -1, -1 };
  gint child_pid_report_pipe[2] = { -1, -1 };
  gint status;
  
  if (!make_pipe (child_err_report_pipe, error))
    return FALSE;

  if (intermediate_child && !make_pipe (child_pid_report_pipe, error))
    goto cleanup_and_fail;
  
  if (standard_input && !make_pipe (stdin_pipe, error))
    goto cleanup_and_fail;
  
  if (standard_output && !make_pipe (stdout_pipe, error))
    goto cleanup_and_fail;

  if (standard_error && !make_pipe (stderr_pipe, error))
    goto cleanup_and_fail;

  pid = fork ();

  if (pid < 0)
    {      
      g_set_error (error,
                   G_SPAWN_ERROR,
                   G_SPAWN_ERROR_FORK,
                   _("Failed to fork (%s)"),
                   g_strerror (errno));

      goto cleanup_and_fail;
    }
  else if (pid == 0)
    {
      /* Immediate child. This may or may not be the child that
       * actually execs the new process.
       */
      
      /* Be sure we crash if the parent exits
       * and we write to the err_report_pipe
       */
      signal (SIGPIPE, SIG_DFL);

      /* Close the parent's end of the pipes;
       * not needed in the close_descriptors case,
       * though
       */
      close_and_invalidate (&child_err_report_pipe[0]);
      close_and_invalidate (&child_pid_report_pipe[0]);
      close_and_invalidate (&stdin_pipe[1]);
      close_and_invalidate (&stdout_pipe[0]);
      close_and_invalidate (&stderr_pipe[0]);
      
      if (intermediate_child)
        {
          /* We need to fork an intermediate child that launches the
           * final child. The purpose of the intermediate child
           * is to exit, so we can waitpid() it immediately.
           * Then the grandchild will not become a zombie.
           */
          GPid grandchild_pid;

          grandchild_pid = fork ();

          if (grandchild_pid < 0)
            {
              /* report -1 as child PID */
              write_all (child_pid_report_pipe[1], &grandchild_pid,
                         sizeof(grandchild_pid));
              
              write_err_and_exit (child_err_report_pipe[1],
                                  CHILD_FORK_FAILED);              
            }
          else if (grandchild_pid == 0)
            {
              do_exec (child_err_report_pipe[1],
                       stdin_pipe[0],
                       stdout_pipe[1],
                       stderr_pipe[1],
                       working_directory,
                       argv,
                       envp,
                       close_descriptors,
                       search_path,
                       stdout_to_null,
                       stderr_to_null,
                       child_inherits_stdin,
                       file_and_argv_zero,
                       child_setup,
                       user_data);
            }
          else
            {
              write_all (child_pid_report_pipe[1], &grandchild_pid, sizeof(grandchild_pid));
              close_and_invalidate (&child_pid_report_pipe[1]);
              
              _exit (0);
            }
        }
      else
        {
          /* Just run the child.
           */

          do_exec (child_err_report_pipe[1],
                   stdin_pipe[0],
                   stdout_pipe[1],
                   stderr_pipe[1],
                   working_directory,
                   argv,
                   envp,
                   close_descriptors,
                   search_path,
                   stdout_to_null,
                   stderr_to_null,
                   child_inherits_stdin,
                   file_and_argv_zero,
                   child_setup,
                   user_data);
        }
    }
  else
    {
      /* Parent */
      
      gint buf[2];
      gint n_ints = 0;    

      /* Close the uncared-about ends of the pipes */
      close_and_invalidate (&child_err_report_pipe[1]);
      close_and_invalidate (&child_pid_report_pipe[1]);
      close_and_invalidate (&stdin_pipe[0]);
      close_and_invalidate (&stdout_pipe[1]);
      close_and_invalidate (&stderr_pipe[1]);

      /* If we had an intermediate child, reap it */
      if (intermediate_child)
        {
        wait_again:
          if (waitpid (pid, &status, 0) < 0)
            {
              if (errno == EINTR)
                goto wait_again;
              else if (errno == ECHILD)
                ; /* do nothing, child already reaped */
              else
                g_warning ("waitpid() should not fail in "
			   "'fork_exec_with_pipes'");
            }
        }
      

      if (!read_ints (child_err_report_pipe[0],
                      buf, 2, &n_ints,
                      error))
        goto cleanup_and_fail;
        
      if (n_ints >= 2)
        {
          /* Error from the child. */

          switch (buf[0])
            {
            case CHILD_CHDIR_FAILED:
              g_set_error (error,
                           G_SPAWN_ERROR,
                           G_SPAWN_ERROR_CHDIR,
                           _("Failed to change to directory '%s' (%s)"),
                           working_directory,
                           g_strerror (buf[1]));

              break;
              
            case CHILD_EXEC_FAILED:
              g_set_error (error,
                           G_SPAWN_ERROR,
                           exec_err_to_g_error (buf[1]),
                           _("Failed to execute child process \"%s\" (%s)"),
                           argv[0],
                           g_strerror (buf[1]));

              break;
              
            case CHILD_DUP2_FAILED:
              g_set_error (error,
                           G_SPAWN_ERROR,
                           G_SPAWN_ERROR_FAILED,
                           _("Failed to redirect output or input of child process (%s)"),
                           g_strerror (buf[1]));

              break;

            case CHILD_FORK_FAILED:
              g_set_error (error,
                           G_SPAWN_ERROR,
                           G_SPAWN_ERROR_FORK,
                           _("Failed to fork child process (%s)"),
                           g_strerror (buf[1]));
              break;
              
            default:
              g_set_error (error,
                           G_SPAWN_ERROR,
                           G_SPAWN_ERROR_FAILED,
                           _("Unknown error executing child process \"%s\""),
                           argv[0]);
              break;
            }

          goto cleanup_and_fail;
        }

      /* Get child pid from intermediate child pipe. */
      if (intermediate_child)
        {
          n_ints = 0;
          
          if (!read_ints (child_pid_report_pipe[0],
                          buf, 1, &n_ints, error))
            goto cleanup_and_fail;

          if (n_ints < 1)
            {
              g_set_error (error,
                           G_SPAWN_ERROR,
                           G_SPAWN_ERROR_FAILED,
                           _("Failed to read enough data from child pid pipe (%s)"),
                           g_strerror (errno));
              goto cleanup_and_fail;
            }
          else
            {
              /* we have the child pid */
              pid = buf[0];
            }
        }
      
      /* Success against all odds! return the information */
      close_and_invalidate (&child_err_report_pipe[0]);
      close_and_invalidate (&child_pid_report_pipe[0]);
 
      if (child_pid)
        *child_pid = pid;

      if (standard_input)
        *standard_input = stdin_pipe[1];
      if (standard_output)
        *standard_output = stdout_pipe[0];
      if (standard_error)
        *standard_error = stderr_pipe[0];
      
      return TRUE;
    }

 cleanup_and_fail:

  /* There was an error from the Child, reap the child to avoid it being
     a zombie.
   */

  if (pid > 0)
  {
    wait_failed:
     if (waitpid (pid, NULL, 0) < 0)
       {
          if (errno == EINTR)
            goto wait_failed;
          else if (errno == ECHILD)
            ; /* do nothing, child already reaped */
          else
            g_warning ("waitpid() should not fail in "
                       "'fork_exec_with_pipes'");
       }
   }

  close_and_invalidate (&child_err_report_pipe[0]);
  close_and_invalidate (&child_err_report_pipe[1]);
  close_and_invalidate (&child_pid_report_pipe[0]);
  close_and_invalidate (&child_pid_report_pipe[1]);
  close_and_invalidate (&stdin_pipe[0]);
  close_and_invalidate (&stdin_pipe[1]);
  close_and_invalidate (&stdout_pipe[0]);
  close_and_invalidate (&stdout_pipe[1]);
  close_and_invalidate (&stderr_pipe[0]);
  close_and_invalidate (&stderr_pipe[1]);

  return FALSE;
}

static gboolean
make_pipe (gint     p[2],
           GError **error)
{
  if (pipe (p) < 0)
    {
      gint errsv = errno;
      g_set_error (error,
                   G_SPAWN_ERROR,
                   G_SPAWN_ERROR_FAILED,
                   _("Failed to create pipe for communicating with child process (%s)"),
                   g_strerror (errsv));
      return FALSE;
    }
  else
    return TRUE;
}

/* Based on execvp from GNU C Library */

static void
script_execute (const gchar *file,
                gchar      **argv,
                gchar      **envp,
                gboolean     search_path)
{
  /* Count the arguments.  */
  int argc = 0;
  while (argv[argc])
    ++argc;
  
  /* Construct an argument list for the shell.  */
  {
    gchar **new_argv;

    new_argv = g_new0 (gchar*, argc + 2); /* /bin/sh and NULL */
    
    new_argv[0] = (char *) "/bin/sh";
    new_argv[1] = (char *) file;
    while (argc > 0)
      {
	new_argv[argc + 1] = argv[argc];
	--argc;
      }

    /* Execute the shell. */
    if (envp)
      execve (new_argv[0], new_argv, envp);
    else
      execv (new_argv[0], new_argv);
    
    g_free (new_argv);
  }
}

static gchar*
my_strchrnul (const gchar *str, gchar c)
{
  gchar *p = (gchar*) str;
  while (*p && (*p != c))
    ++p;

  return p;
}

static gint
g_execute (const gchar *file,
           gchar      **argv,
           gchar      **envp,
           gboolean     search_path)
{
  if (*file == '\0')
    {
      /* We check the simple case first. */
      errno = ENOENT;
      return -1;
    }

  if (!search_path || strchr (file, '/') != NULL)
    {
      /* Don't search when it contains a slash. */
      if (envp)
        execve (file, argv, envp);
      else
        execv (file, argv);
      
      if (errno == ENOEXEC)
	script_execute (file, argv, envp, FALSE);
    }
  else
    {
      gboolean got_eacces = 0;
      const gchar *path, *p;
      gchar *name, *freeme;
      gsize len;
      gsize pathlen;

      path = g_getenv ("PATH");
      if (path == NULL)
	{
	  /* There is no `PATH' in the environment.  The default
	   * search path in libc is the current directory followed by
	   * the path `confstr' returns for `_CS_PATH'.
           */

          /* In GLib we put . last, for security, and don't use the
           * unportable confstr(); UNIX98 does not actually specify
           * what to search if PATH is unset. POSIX may, dunno.
           */
          
          path = "/bin:/usr/bin:.";
	}

      len = strlen (file) + 1;
      pathlen = strlen (path);
      freeme = name = g_malloc (pathlen + len + 1);
      
      /* Copy the file name at the top, including '\0'  */
      memcpy (name + pathlen + 1, file, len);
      name = name + pathlen;
      /* And add the slash before the filename  */
      *name = '/';

      p = path;
      do
	{
	  char *startp;

	  path = p;
	  p = my_strchrnul (path, ':');

	  if (p == path)
	    /* Two adjacent colons, or a colon at the beginning or the end
             * of `PATH' means to search the current directory.
             */
	    startp = name + 1;
	  else
	    startp = memcpy (name - (p - path), path, p - path);

	  /* Try to execute this name.  If it works, execv will not return.  */
          if (envp)
            execve (startp, argv, envp);
          else
            execv (startp, argv);
          
	  if (errno == ENOEXEC)
	    script_execute (startp, argv, envp, search_path);

	  switch (errno)
	    {
	    case EACCES:
	      /* Record the we got a `Permission denied' error.  If we end
               * up finding no executable we can use, we want to diagnose
               * that we did find one but were denied access.
               */
	      got_eacces = TRUE;

              /* FALL THRU */
              
	    case ENOENT:
#ifdef ESTALE
	    case ESTALE:
#endif
#ifdef ENOTDIR
	    case ENOTDIR:
#endif
	      /* Those errors indicate the file is missing or not executable
               * by us, in which case we want to just try the next path
               * directory.
               */
	      break;

	    default:
	      /* Some other error means we found an executable file, but
               * something went wrong executing it; return the error to our
               * caller.
               */
              g_free (freeme);
	      return -1;
	    }
	}
      while (*p++ != '\0');

      /* We tried every element and none of them worked.  */
      if (got_eacces)
	/* At least one failure was due to permissions, so report that
         * error.
         */
        errno = EACCES;

      g_free (freeme);
    }

  /* Return the error from the last attempt (probably ENOENT).  */
  return -1;
}

/**
 * g_spawn_close_pid:
 * @pid: The process reference to close
 *
 * On some platforms, notably Windows, the #GPid type represents a resource
 * which must be closed to prevent resource leaking. g_spawn_close_pid()
 * is provided for this purpose. It should be used on all platforms, even
 * though it doesn't do anything under UNIX.
 **/
void
g_spawn_close_pid (GPid pid)
{
}

#define __G_SPAWN_C__
#include "galiasdef.c"
