/* gspawn.h - Process launching
 *
 *  Copyright 2000 Red Hat, Inc.
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

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_SPAWN_H__
#define __G_SPAWN_H__

#include <glib/gerror.h>

G_BEGIN_DECLS

/* I'm not sure I remember our proposed naming convention here. */
#define G_SPAWN_ERROR g_spawn_error_quark ()

typedef enum
{
  G_SPAWN_ERROR_FORK,   /* fork failed due to lack of memory */
  G_SPAWN_ERROR_READ,   /* read or select on pipes failed */
  G_SPAWN_ERROR_CHDIR,  /* changing to working dir failed */
  G_SPAWN_ERROR_ACCES,  /* execv() returned EACCES */
  G_SPAWN_ERROR_PERM,   /* execv() returned EPERM */
  G_SPAWN_ERROR_2BIG,   /* execv() returned E2BIG */
  G_SPAWN_ERROR_NOEXEC, /* execv() returned ENOEXEC */
  G_SPAWN_ERROR_NAMETOOLONG, /* ""  "" ENAMETOOLONG */
  G_SPAWN_ERROR_NOENT,       /* ""  "" ENOENT */
  G_SPAWN_ERROR_NOMEM,       /* ""  "" ENOMEM */
  G_SPAWN_ERROR_NOTDIR,      /* ""  "" ENOTDIR */
  G_SPAWN_ERROR_LOOP,        /* ""  "" ELOOP   */
  G_SPAWN_ERROR_TXTBUSY,     /* ""  "" ETXTBUSY */
  G_SPAWN_ERROR_IO,          /* ""  "" EIO */
  G_SPAWN_ERROR_NFILE,       /* ""  "" ENFILE */
  G_SPAWN_ERROR_MFILE,       /* ""  "" EMFLE */
  G_SPAWN_ERROR_INVAL,       /* ""  "" EINVAL */
  G_SPAWN_ERROR_ISDIR,       /* ""  "" EISDIR */
  G_SPAWN_ERROR_LIBBAD,      /* ""  "" ELIBBAD */
  G_SPAWN_ERROR_FAILED       /* other fatal failure, error->message
                              * should explain
                              */
} GSpawnError;

typedef void (* GSpawnChildSetupFunc) (gpointer user_data);

typedef enum
{
  G_SPAWN_LEAVE_DESCRIPTORS_OPEN = 1 << 0,
  G_SPAWN_DO_NOT_REAP_CHILD      = 1 << 1,
  /* look for argv[0] in the path i.e. use execvp() */
  G_SPAWN_SEARCH_PATH            = 1 << 2,
  /* Dump output to /dev/null */
  G_SPAWN_STDOUT_TO_DEV_NULL     = 1 << 3,
  G_SPAWN_STDERR_TO_DEV_NULL     = 1 << 4,
  G_SPAWN_CHILD_INHERITS_STDIN   = 1 << 5,
  G_SPAWN_FILE_AND_ARGV_ZERO     = 1 << 6
} GSpawnFlags;

GQuark g_spawn_error_quark (void);

#ifdef G_OS_WIN32
#define g_spawn_async g_spawn_async_utf8
#define g_spawn_async_with_pipes g_spawn_async_with_pipes_utf8
#define g_spawn_sync g_spawn_sync_utf8
#define g_spawn_command_line_sync g_spawn_command_line_sync_utf8
#define g_spawn_command_line_async g_spawn_command_line_async_utf8
#endif

gboolean g_spawn_async (const gchar           *working_directory,
                        gchar                **argv,
                        gchar                **envp,
                        GSpawnFlags            flags,
                        GSpawnChildSetupFunc   child_setup,
                        gpointer               user_data,
                        GPid                  *child_pid,
                        GError               **error);


/* Opens pipes for non-NULL standard_output, standard_input, standard_error,
 * and returns the parent's end of the pipes.
 */
gboolean g_spawn_async_with_pipes (const gchar          *working_directory,
                                   gchar               **argv,
                                   gchar               **envp,
                                   GSpawnFlags           flags,
                                   GSpawnChildSetupFunc  child_setup,
                                   gpointer              user_data,
                                   GPid                 *child_pid,
                                   gint                 *standard_input,
                                   gint                 *standard_output,
                                   gint                 *standard_error,
                                   GError              **error);


/* If standard_output or standard_error are non-NULL, the full
 * standard output or error of the command will be placed there.
 */

gboolean g_spawn_sync         (const gchar          *working_directory,
                               gchar               **argv,
                               gchar               **envp,
                               GSpawnFlags           flags,
                               GSpawnChildSetupFunc  child_setup,
                               gpointer              user_data,
                               gchar               **standard_output,
                               gchar               **standard_error,
                               gint                 *exit_status,
                               GError              **error);

gboolean g_spawn_command_line_sync  (const gchar          *command_line,
                                     gchar               **standard_output,
                                     gchar               **standard_error,
                                     gint                 *exit_status,
                                     GError              **error);
gboolean g_spawn_command_line_async (const gchar          *command_line,
                                     GError              **error);

void g_spawn_close_pid (GPid pid);

G_END_DECLS

#endif /* __G_SPAWN_H__ */
