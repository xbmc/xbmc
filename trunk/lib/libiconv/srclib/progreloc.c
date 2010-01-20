/* Provide relocatable programs.
   Copyright (C) 2003-2009 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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
#include "progname.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Get declaration of _NSGetExecutablePath on MacOS X 10.2 or newer.  */
#if HAVE_MACH_O_DYLD_H
# include <mach-o/dyld.h>
#endif

#if defined _WIN32 || defined __WIN32__
# define WIN32_NATIVE
#endif

#if defined WIN32_NATIVE || defined __CYGWIN__
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include "canonicalize.h"
#include "relocatable.h"

#ifdef NO_XMALLOC
# include "areadlink.h"
# define xreadlink areadlink
#else
# include "xreadlink.h"
#endif

#ifdef NO_XMALLOC
# define xmalloc malloc
# define xstrdup strdup
#else
# include "xalloc.h"
#endif

/* Pathname support.
   ISSLASH(C)           tests whether C is a directory separator character.
   IS_PATH_WITH_DIR(P)  tests whether P contains a directory specification.
 */
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__ || defined __EMX__ || defined __DJGPP__
  /* Win32, Cygwin, OS/2, DOS */
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
# define HAS_DEVICE(P) \
    ((((P)[0] >= 'A' && (P)[0] <= 'Z') || ((P)[0] >= 'a' && (P)[0] <= 'z')) \
     && (P)[1] == ':')
# define IS_PATH_WITH_DIR(P) \
    (strchr (P, '/') != NULL || strchr (P, '\\') != NULL || HAS_DEVICE (P))
# define FILE_SYSTEM_PREFIX_LEN(P) (HAS_DEVICE (P) ? 2 : 0)
#else
  /* Unix */
# define ISSLASH(C) ((C) == '/')
# define IS_PATH_WITH_DIR(P) (strchr (P, '/') != NULL)
# define FILE_SYSTEM_PREFIX_LEN(P) 0
#endif

/* The results of open() in this file are not used with fchdir,
   therefore save some unnecessary work in fchdir.c.  */
#undef open
#undef close

#undef set_program_name


#if ENABLE_RELOCATABLE

#ifdef __linux__
/* File descriptor of the executable.
   (Only used to verify that we find the correct executable.)  */
static int executable_fd = -1;
#endif

/* Tests whether a given pathname may belong to the executable.  */
static bool
maybe_executable (const char *filename)
{
  /* Woe32 lacks the access() function, but Cygwin doesn't.  */
#if !(defined WIN32_NATIVE && !defined __CYGWIN__)
  if (access (filename, X_OK) < 0)
    return false;

#ifdef __linux__
  if (executable_fd >= 0)
    {
      /* If we already have an executable_fd, check that filename points to
	 the same inode.  */
      struct stat statexe;
      struct stat statfile;

      if (fstat (executable_fd, &statexe) >= 0)
	{
	  if (stat (filename, &statfile) < 0)
	    return false;
	  if (!(statfile.st_dev
		&& statfile.st_dev == statexe.st_dev
		&& statfile.st_ino == statexe.st_ino))
	    return false;
	}
    }
#endif
#endif

  return true;
}

/* Determine the full pathname of the current executable, freshly allocated.
   Return NULL if unknown.
   Guaranteed to work on Linux and Woe32.  Likely to work on the other
   Unixes (maybe except BeOS), under most conditions.  */
static char *
find_executable (const char *argv0)
{
#if defined WIN32_NATIVE || defined __CYGWIN__
  char location[MAX_PATH];
  int length = GetModuleFileName (NULL, location, sizeof (location));
  if (length < 0)
    return NULL;
  if (!IS_PATH_WITH_DIR (location))
    /* Shouldn't happen.  */
    return NULL;
  {
#if defined __CYGWIN__
    /* cygwin-1.5.13 (2005-03-01) or newer would also allow a Linux-like
       implementation: readlink of "/proc/self/exe".  But using the
       result of the Win32 system call is simpler and is consistent with the
       code in relocatable.c.  */
    /* On Cygwin, we need to convert paths coming from Win32 system calls
       to the Unix-like slashified notation.  */
    static char location_as_posix_path[2 * MAX_PATH];
    /* There's no error return defined for cygwin_conv_to_posix_path.
       See cygwin-api/func-cygwin-conv-to-posix-path.html.
       Does it overflow the buffer of expected size MAX_PATH or does it
       truncate the path?  I don't know.  Let's catch both.  */
    cygwin_conv_to_posix_path (location, location_as_posix_path);
    location_as_posix_path[MAX_PATH - 1] = '\0';
    if (strlen (location_as_posix_path) >= MAX_PATH - 1)
      /* A sign of buffer overflow or path truncation.  */
      return NULL;
    /* Call canonicalize_file_name, because Cygwin supports symbolic links.  */
    return canonicalize_file_name (location_as_posix_path);
#else
    return xstrdup (location);
#endif
  }
#else /* Unix && !Cygwin */
#ifdef __linux__
  /* The executable is accessible as /proc/<pid>/exe.  In newer Linux
     versions, also as /proc/self/exe.  Linux >= 2.1 provides a symlink
     to the true pathname; older Linux versions give only device and ino,
     enclosed in brackets, which we cannot use here.  */
  {
    char *link;

    link = xreadlink ("/proc/self/exe");
    if (link != NULL && link[0] != '[')
      return link;
    if (executable_fd < 0)
      executable_fd = open ("/proc/self/exe", O_RDONLY, 0);

    {
      char buf[6+10+5];
      sprintf (buf, "/proc/%d/exe", getpid ());
      link = xreadlink (buf);
      if (link != NULL && link[0] != '[')
	return link;
      if (executable_fd < 0)
	executable_fd = open (buf, O_RDONLY, 0);
    }
  }
#endif
#if HAVE_MACH_O_DYLD_H && HAVE__NSGETEXECUTABLEPATH
  /* On MacOS X 10.2 or newer, the function
       int _NSGetExecutablePath (char *buf, uint32_t *bufsize);
     can be used to retrieve the executable's full path.  */
  char location[4096];
  unsigned int length = sizeof (location);
  if (_NSGetExecutablePath (location, &length) == 0
      && location[0] == '/')
    return canonicalize_file_name (location);
#endif
  /* Guess the executable's full path.  We assume the executable has been
     called via execlp() or execvp() with properly set up argv[0].  The
     login(1) convention to add a '-' prefix to argv[0] is not supported.  */
  {
    bool has_slash = false;
    {
      const char *p;
      for (p = argv0; *p; p++)
	if (*p == '/')
	  {
	    has_slash = true;
	    break;
	  }
    }
    if (!has_slash)
      {
	/* exec searches paths without slashes in the directory list given
	   by $PATH.  */
	const char *path = getenv ("PATH");

	if (path != NULL)
	  {
	    const char *p;
	    const char *p_next;

	    for (p = path; *p; p = p_next)
	      {
		const char *q;
		size_t p_len;
		char *concat_name;

		for (q = p; *q; q++)
		  if (*q == ':')
		    break;
		p_len = q - p;
		p_next = (*q == '\0' ? q : q + 1);

		/* We have a path item at p, of length p_len.
		   Now concatenate the path item and argv0.  */
		concat_name = (char *) xmalloc (p_len + strlen (argv0) + 2);
#ifdef NO_XMALLOC
		if (concat_name == NULL)
		  return NULL;
#endif
		if (p_len == 0)
		  /* An empty PATH element designates the current directory.  */
		  strcpy (concat_name, argv0);
		else
		  {
		    memcpy (concat_name, p, p_len);
		    concat_name[p_len] = '/';
		    strcpy (concat_name + p_len + 1, argv0);
		  }
		if (maybe_executable (concat_name))
		  return canonicalize_file_name (concat_name);
		free (concat_name);
	      }
	  }
	/* Not found in the PATH, assume the current directory.  */
      }
    /* exec treats paths containing slashes as relative to the current
       directory.  */
    if (maybe_executable (argv0))
      return canonicalize_file_name (argv0);
  }
  /* No way to find the executable.  */
  return NULL;
#endif
}

/* Full pathname of executable, or NULL.  */
static char *executable_fullname;

static void
prepare_relocate (const char *orig_installprefix, const char *orig_installdir,
		  const char *argv0)
{
  char *curr_prefix;

  /* Determine the full pathname of the current executable.  */
  executable_fullname = find_executable (argv0);

  /* Determine the current installation prefix from it.  */
  curr_prefix = compute_curr_prefix (orig_installprefix, orig_installdir,
				     executable_fullname);
  if (curr_prefix != NULL)
    {
      /* Now pass this prefix to all copies of the relocate.c source file.  */
      set_relocation_prefix (orig_installprefix, curr_prefix);

      free (curr_prefix);
    }
}

/* Set program_name, based on argv[0], and original installation prefix and
   directory, for relocatability.  */
void
set_program_name_and_installdir (const char *argv0,
				 const char *orig_installprefix,
				 const char *orig_installdir)
{
  const char *argv0_stripped = argv0;

  /* Relocatable programs are renamed to .bin by install-reloc.  Or, more
     generally, their suffix is changed from $exeext to .bin$exeext.
     Remove the ".bin" here.  */
  {
    size_t argv0_len = strlen (argv0);
    const size_t exeext_len = sizeof (EXEEXT) - sizeof ("");
    if (argv0_len > 4 + exeext_len)
      if (memcmp (argv0 + argv0_len - exeext_len - 4, ".bin", 4) == 0)
	{
	  if (sizeof (EXEEXT) > sizeof (""))
	    {
	      /* Compare using an inlined copy of c_strncasecmp(), because
		 the filenames may have undergone a case conversion since
		 they were packaged.  In other words, EXEEXT may be ".exe"
		 on one system and ".EXE" on another.  */
	      static const char exeext[] = EXEEXT;
	      const char *s1 = argv0 + argv0_len - exeext_len;
	      const char *s2 = exeext;
	      for (; *s1 != '\0'; s1++, s2++)
		{
		  unsigned char c1 = *s1;
		  unsigned char c2 = *s2;
		  if ((c1 >= 'A' && c1 <= 'Z' ? c1 - 'A' + 'a' : c1)
		      != (c2 >= 'A' && c2 <= 'Z' ? c2 - 'A' + 'a' : c2))
		    goto done_stripping;
		}
	    }
	  /* Remove ".bin" before EXEEXT or its equivalent.  */
	  {
	    char *shorter = (char *) xmalloc (argv0_len - 4 + 1);
#ifdef NO_XMALLOC
	    if (shorter != NULL)
#endif
	      {
		memcpy (shorter, argv0, argv0_len - exeext_len - 4);
		if (sizeof (EXEEXT) > sizeof (""))
		  memcpy (shorter + argv0_len - exeext_len - 4,
			  argv0 + argv0_len - exeext_len - 4,
			  exeext_len);
		shorter[argv0_len - 4] = '\0';
		argv0_stripped = shorter;
	      }
	  }
	 done_stripping: ;
      }
  }

  set_program_name (argv0_stripped);

  prepare_relocate (orig_installprefix, orig_installdir, argv0);
}

/* Return the full pathname of the current executable, based on the earlier
   call to set_program_name_and_installdir.  Return NULL if unknown.  */
char *
get_full_program_name (void)
{
  return executable_fullname;
}

#endif
