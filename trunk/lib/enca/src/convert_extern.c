/*
  @(#) $Id: convert_extern.c,v 1.9 2003/12/23 23:08:10 yeti Exp $
  interface to external converter programs

  Copyright (C) 2000-2003 David Necas (Yeti) <yeti@physics.muni.cz>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as published
  by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/
#include "common.h"
#ifdef ENABLE_EXTERNAL

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#else
pid_t waitpid(pid_t pid, int *status, int options);
#endif

/* We can't go on w/o this, defining struct stat manually is braindamaged. */
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif /* HAVE_LIMITS_H */

/* Resolve all the forking mess. */
#ifdef HAVE_WORKING_VFORK
#  ifdef HAVE_VFORK_H
#    include <vfork.h>
#  endif /* HAVE_VFORK_H */
#else /* HAVE_WORKING_VFORK */
#  define vfork fork
#endif /* HAVE_WORKING_VFORK */

#ifndef HAVE_CANONICALIZE_FILE_NAME
#  ifdef HAVE_REALPATH
static char* canonicalize_file_name(const char *fname);
#  else /* HAVE_REALPATH */
#    define canonicalize_file_name enca_strdup
#  endif /* HAVE_REALPATH */
#endif /* HAVE_CANONICALIZE_FILE_NAME */

/* external converter command */
static char *extern_converter = NULL;

/* Local prototypes. */
static int check_executability_one(const char *progpath);

/* fork and the child executes Settings.Converter on fname
   create temporary file containing stdin when fname == NULL and convert it
   passing special option STDOUT to converter (that is assumed to delete
   the temporary file itself)
   from_enc, to_enc are encoding names as should be passed to converter
   returns 0 on success, nonzero on failure;
   on critical failure (like we cannot fork()) it simply aborts */
int
convert_external(File *file,
                 const EncaEncoding from_enc)
{
  /* special fourth parameter passed to external converter to instruct it to
  send result to stdout */
  static const char *STDOUT_CONV = "-";

  pid_t pid;
  int status;
  File *tempfile = NULL;
  char *from_name, *target_name;

  if (*extern_converter == '\0') {
    fprintf(stderr, "%s: No external converter defined!\n", program_name);
    return ERR_CANNOT;
  }

  if (options.verbosity_level > 2)
    fprintf(stderr, "    launching `%s' to convert `%s'\n",
                    extern_converter, ffname_r(file->name));

  /* Is conversion of stdin requested? */
  if (file->name == NULL) {
    /* Then we have to copy it to a temporary file. */
    tempfile = file_temporary(file->buffer, 0);
    if (tempfile == NULL)
      return ERR_IOFAIL;

    if (copy_and_convert(file, tempfile, NULL) != 0) {
      file_unlink(tempfile->name);
      file_free(tempfile);
      return ERR_IOFAIL;
    }
  }

  /* Construct the charset names before fork() */
  from_name = enca_strconcat(enca_charset_name(from_enc.charset,
                                               ENCA_NAME_STYLE_ENCA),
                             enca_get_surface_name(from_enc.surface,
                                                   ENCA_NAME_STYLE_ENCA),
                             NULL);
  if (enca_charset_is_known(options.target_enc.charset)
      && (options.target_enc.surface & ENCA_SURFACE_UNKNOWN) == 0) {
    target_name
      = enca_strconcat(enca_charset_name(options.target_enc.charset,
                                         ENCA_NAME_STYLE_ENCA),
                       enca_get_surface_name(options.target_enc.surface,
                                             ENCA_NAME_STYLE_ENCA),
                       NULL);
  }
  else
    target_name = enca_strdup(options.target_enc_str);

  /* Fork. */
  pid = vfork();
  if (pid == 0) {
    /* Child. */
    if (tempfile)
      execlp(extern_converter, extern_converter,
             from_name, target_name, tempfile->name,
             STDOUT_CONV, NULL);
    else
      execlp(extern_converter, extern_converter,
             from_name, target_name, file->name, NULL);

    exit(ERR_EXEC);
  }

  /* Parent. */
  if (pid == -1) {
    fprintf(stderr, "%s: Cannot fork() to execute converter: %s\n",
                    program_name,
                    strerror(errno));
    exit(EXIT_TROUBLE);
  }
  /* Wait until the child returns. */
  if (waitpid(pid, &status, 0) == -1) {
    /* Error. */
    fprintf(stderr, "%s: wait_pid() error while waiting for converter: %s\n",
                    program_name,
                    strerror(errno));
    exit(EXIT_TROUBLE);
  }
  if (!WIFEXITED(status)) {
    /* Child exited abnormally. */
    fprintf(stderr, "%s: Child converter process has been murdered.\n",
                    program_name);
    exit(EXIT_TROUBLE);
  }

  enca_free(from_name);
  enca_free(target_name);

  if (tempfile) {
    unlink(tempfile->name);
    file_free(tempfile);
  }

  /* Child exited normally, test exit status. */
  if (WEXITSTATUS(status) != EXIT_SUCCESS) {
    /* This means child was unable to execute converter or converter failed. */
    fprintf(stderr, "%s: External converter failed (error code %d)\n",
                    program_name,
                    WEXITSTATUS(status));
    if (WEXITSTATUS(status) == ERR_EXEC)
      return ERR_EXEC;
    else
      return ERR_CANNOT;
  }
  /* Success!  Wow! */
  return ERR_OK;
}

/* set external converter to extc */
void
set_external_converter(const char *extc)
{
  enca_free(extern_converter);
  if (strchr(extc, '/') == NULL) {
    if (extc[0] == 'b' && extc[1] == '-') {
      extc += 2;
      fprintf(stderr, "%s: The `b-' prefix for standard external converters "
                      "is deprecated.\n"
                      "I'll pretend you said `%s'.\n",
                      program_name,
                      extc);
    }
    extern_converter = enca_strconcat(EXTCONV_DIR, "/", extc, NULL);
  }
  else
    extern_converter = enca_strdup(extc);
}

/* return nonzero if external converter seems ok */
int
check_external_converter(void)
{
  /* FIXME: This creates a race condition.  However we don't want to do all
   * the checking before every execlp() when conveting 500 files in a row,
   * and even if doing that, something can still sneak between stat() and
   * execlp(), so what.  This is just a simple sanity check, nothing strict.
   */
  if (*extern_converter == '\0'
      || !check_executability_one(extern_converter)) {
    fprintf(stderr, "%s: Converter `%s' doesn't seem to be executable.\n"
                    "Note as of enca-1.3 external converters must be\n"
                    "(a) one of the standard ones residing in %s\n"
                    "(b) specified with full path\n",
                    program_name,
                    extern_converter,
                    EXTCONV_DIR);
    return 0;
  }

  return 1;
}

/**
 * Checks whether @progpath is and executable we can execute it.
 * Tries to resolve relative paths and symlinks.
 **/
static int
check_executability_one(const char *progpath)
{
  static uid_t uid;
  static gid_t gid;
  static int check_executability_one_initialized = 0;

  struct stat st;
  char *fname = canonicalize_file_name(progpath);

  /* Is it a regular file at all? */
  if (stat(fname, &st) != 0
      || (st.st_mode & S_IFREG) == 0) {
    enca_free(fname);
    return 0;
  }

  /* Check executability by anyone, user, group. */
  enca_free(fname);
  if (st.st_mode & S_IXOTH)
    return 1;

  if (!check_executability_one_initialized) {
    uid = getuid();
    gid = getgid();
    check_executability_one_initialized = 1;
  }

  if ((st.st_mode & S_IXUSR) && st.st_uid == gid)
    return 1;

  if ((st.st_mode & S_IXGRP) && st.st_uid == uid)
    return 1;

  return 0;
}

#ifndef HAVE_CANONICALIZE_FILE_NAME
#  ifdef HAVE_REALPATH
#  ifndef FILENAME_MAX
#    define FILENAME_MAX 4096
#  endif /* FILENAME_MAX */
static char*
canonicalize_file_name(const char *fname)
{
  char *resolved = enca_malloc(FILENAME_MAX + 1);

  return realpath(fname, resolved);
}
#  endif /* HAVE_REALPATH */
#endif /* not HAVE_CANONICALIZE_FILE_NAME */

#endif /* ENABLE_EXTERNAL */
/* vim: ts=2
 */
