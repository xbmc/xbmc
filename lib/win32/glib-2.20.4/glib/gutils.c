/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1998  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * MT safe for the unix part, FIXME: make the win32 part MT safe as well.
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>		/* For tolower() */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_CRT_EXTERNS_H 
#include <crt_externs.h> /* for _NSGetEnviron */
#endif

/* implement gutils's inline functions
 */
#define	G_IMPLEMENT_INLINES 1
#define	__G_UTILS_C__
#include "glib.h"
#include "gprintfint.h"
#include "gthreadprivate.h"
#include "glibintl.h"
#include "galias.h"

#ifdef	MAXPATHLEN
#define	G_PATH_LENGTH	MAXPATHLEN
#elif	defined (PATH_MAX)
#define	G_PATH_LENGTH	PATH_MAX
#elif   defined (_PC_PATH_MAX)
#define	G_PATH_LENGTH	sysconf(_PC_PATH_MAX)
#else	
#define G_PATH_LENGTH   2048
#endif

#ifdef G_PLATFORM_WIN32
#  define STRICT		/* Strict typing, please */
#  include <windows.h>
#  undef STRICT
#  ifndef GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
#    define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 2
#    define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 4
#  endif
#  include <lmcons.h>		/* For UNLEN */
#endif /* G_PLATFORM_WIN32 */

#ifdef G_OS_WIN32
#  include <direct.h>
#  include <shlobj.h>
   /* older SDK (e.g. msvc 5.0) does not have these*/
#  ifndef CSIDL_MYMUSIC
#    define CSIDL_MYMUSIC 13
#  endif
#  ifndef CSIDL_MYVIDEO
#    define CSIDL_MYVIDEO 14
#  endif
#  ifndef CSIDL_INTERNET_CACHE
#    define CSIDL_INTERNET_CACHE 32
#  endif
#  ifndef CSIDL_COMMON_APPDATA
#    define CSIDL_COMMON_APPDATA 35
#  endif
#  ifndef CSIDL_MYPICTURES
#    define CSIDL_MYPICTURES 0x27
#  endif
#  ifndef CSIDL_COMMON_DOCUMENTS
#    define CSIDL_COMMON_DOCUMENTS 46
#  endif
#  ifndef CSIDL_PROFILE
#    define CSIDL_PROFILE 40
#  endif
#  include <process.h>
#endif

#ifdef HAVE_CARBON
#include <CoreServices/CoreServices.h>
#endif

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif

const guint glib_major_version = GLIB_MAJOR_VERSION;
const guint glib_minor_version = GLIB_MINOR_VERSION;
const guint glib_micro_version = GLIB_MICRO_VERSION;
const guint glib_interface_age = GLIB_INTERFACE_AGE;
const guint glib_binary_age = GLIB_BINARY_AGE;

#ifdef G_PLATFORM_WIN32

static HMODULE glib_dll = NULL;

#ifdef DLL_EXPORT

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
      glib_dll = hinstDLL;

  return TRUE;
}

#endif

gchar *
_glib_get_dll_directory (void)
{
  gchar *retval;
  gchar *p;
  wchar_t wc_fn[MAX_PATH];

#ifdef DLL_EXPORT
  if (glib_dll == NULL)
    return NULL;
#endif

  /* This code is different from that in
   * g_win32_get_package_installation_directory_of_module() in that
   * here we return the actual folder where the GLib DLL is. We don't
   * do the check for it being in a "bin" or "lib" subfolder and then
   * returning the parent of that.
   *
   * In a statically built GLib, glib_dll will be NULL and we will
   * thus look up the application's .exe file's location.
   */
  if (!GetModuleFileNameW (glib_dll, wc_fn, MAX_PATH))
    return NULL;

  retval = g_utf16_to_utf8 (wc_fn, -1, NULL, NULL, NULL);

  p = strrchr (retval, G_DIR_SEPARATOR);
  if (p == NULL)
    {
      /* Wtf? */
      return NULL;
    }
  *p = '\0';

  return retval;
}

#endif

/**
 * glib_check_version:
 * @required_major: the required major version.
 * @required_minor: the required minor version.
 * @required_micro: the required micro version.
 *
 * Checks that the GLib library in use is compatible with the
 * given version. Generally you would pass in the constants
 * #GLIB_MAJOR_VERSION, #GLIB_MINOR_VERSION, #GLIB_MICRO_VERSION
 * as the three arguments to this function; that produces
 * a check that the library in use is compatible with
 * the version of GLib the application or module was compiled
 * against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * Return value: %NULL if the GLib library is compatible with the
 *   given version, or a string describing the version mismatch.
 *   The returned string is owned by GLib and must not be modified
 *   or freed.
 *
 * Since: 2.6
 **/
const gchar *
glib_check_version (guint required_major,
                    guint required_minor,
                    guint required_micro)
{
  gint glib_effective_micro = 100 * GLIB_MINOR_VERSION + GLIB_MICRO_VERSION;
  gint required_effective_micro = 100 * required_minor + required_micro;

  if (required_major > GLIB_MAJOR_VERSION)
    return "GLib version too old (major mismatch)";
  if (required_major < GLIB_MAJOR_VERSION)
    return "GLib version too new (major mismatch)";
  if (required_effective_micro < glib_effective_micro - GLIB_BINARY_AGE)
    return "GLib version too new (micro mismatch)";
  if (required_effective_micro > glib_effective_micro)
    return "GLib version too old (micro mismatch)";
  return NULL;
}

#if !defined (HAVE_MEMMOVE) && !defined (HAVE_WORKING_BCOPY)
/**
 * g_memmove: 
 * @dest: the destination address to copy the bytes to.
 * @src: the source address to copy the bytes from.
 * @len: the number of bytes to copy.
 *
 * Copies a block of memory @len bytes long, from @src to @dest.
 * The source and destination areas may overlap.
 *
 * In order to use this function, you must include 
 * <filename>string.h</filename> yourself, because this macro will 
 * typically simply resolve to memmove() and GLib does not include 
 * <filename>string.h</filename> for you.
 */
void 
g_memmove (gpointer      dest, 
	   gconstpointer src, 
	   gulong        len)
{
  gchar* destptr = dest;
  const gchar* srcptr = src;
  if (src + len < dest || dest + len < src)
    {
      bcopy (src, dest, len);
      return;
    }
  else if (dest <= src)
    {
      while (len--)
	*(destptr++) = *(srcptr++);
    }
  else
    {
      destptr += len;
      srcptr += len;
      while (len--)
	*(--destptr) = *(--srcptr);
    }
}
#endif /* !HAVE_MEMMOVE && !HAVE_WORKING_BCOPY */

#ifdef G_OS_WIN32
#undef g_atexit
#endif

/**
 * g_atexit:
 * @func: the function to call on normal program termination.
 * 
 * Specifies a function to be called at normal program termination.
 *
 * Since GLib 2.8.2, on Windows g_atexit() actually is a preprocessor
 * macro that maps to a call to the atexit() function in the C
 * library. This means that in case the code that calls g_atexit(),
 * i.e. atexit(), is in a DLL, the function will be called when the
 * DLL is detached from the program. This typically makes more sense
 * than that the function is called when the GLib DLL is detached,
 * which happened earlier when g_atexit() was a function in the GLib
 * DLL.
 *
 * The behaviour of atexit() in the context of dynamically loaded
 * modules is not formally specified and varies wildly.
 *
 * On POSIX systems, calling g_atexit() (or atexit()) in a dynamically
 * loaded module which is unloaded before the program terminates might
 * well cause a crash at program exit.
 *
 * Some POSIX systems implement atexit() like Windows, and have each
 * dynamically loaded module maintain an own atexit chain that is
 * called when the module is unloaded.
 *
 * On other POSIX systems, before a dynamically loaded module is
 * unloaded, the registered atexit functions (if any) residing in that
 * module are called, regardless where the code that registered them
 * resided. This is presumably the most robust approach.
 *
 * As can be seen from the above, for portability it's best to avoid
 * calling g_atexit() (or atexit()) except in the main executable of a
 * program.
 */
void
g_atexit (GVoidFunc func)
{
  gint result;
  const gchar *error = NULL;

  /* keep this in sync with glib.h */

#ifdef	G_NATIVE_ATEXIT
  result = ATEXIT (func);
  if (result)
    error = g_strerror (errno);
#elif defined (HAVE_ATEXIT)
#  ifdef NeXT /* @#%@! NeXTStep */
  result = !atexit ((void (*)(void)) func);
  if (result)
    error = g_strerror (errno);
#  else
  result = atexit ((void (*)(void)) func);
  if (result)
    error = g_strerror (errno);
#  endif /* NeXT */
#elif defined (HAVE_ON_EXIT)
  result = on_exit ((void (*)(int, void *)) func, NULL);
  if (result)
    error = g_strerror (errno);
#else
  result = 0;
  error = "no implementation";
#endif /* G_NATIVE_ATEXIT */

  if (error)
    g_error ("Could not register atexit() function: %s", error);
}

/* Based on execvp() from GNU Libc.
 * Some of this code is cut-and-pasted into gspawn.c
 */

static gchar*
my_strchrnul (const gchar *str, 
	      gchar        c)
{
  gchar *p = (gchar*)str;
  while (*p && (*p != c))
    ++p;

  return p;
}

#ifdef G_OS_WIN32

static gchar *inner_find_program_in_path (const gchar *program);

gchar*
g_find_program_in_path (const gchar *program)
{
  const gchar *last_dot = strrchr (program, '.');

  if (last_dot == NULL ||
      strchr (last_dot, '\\') != NULL ||
      strchr (last_dot, '/') != NULL)
    {
      const gint program_length = strlen (program);
      gchar *pathext = g_build_path (";",
				     ".exe;.cmd;.bat;.com",
				     g_getenv ("PATHEXT"),
				     NULL);
      gchar *p;
      gchar *decorated_program;
      gchar *retval;

      p = pathext;
      do
	{
	  gchar *q = my_strchrnul (p, ';');

	  decorated_program = g_malloc (program_length + (q-p) + 1);
	  memcpy (decorated_program, program, program_length);
	  memcpy (decorated_program+program_length, p, q-p);
	  decorated_program [program_length + (q-p)] = '\0';
	  
	  retval = inner_find_program_in_path (decorated_program);
	  g_free (decorated_program);

	  if (retval != NULL)
	    {
	      g_free (pathext);
	      return retval;
	    }
	  p = q;
	} while (*p++ != '\0');
      g_free (pathext);
      return NULL;
    }
  else
    return inner_find_program_in_path (program);
}

#endif

/**
 * g_find_program_in_path:
 * @program: a program name in the GLib file name encoding
 * 
 * Locates the first executable named @program in the user's path, in the
 * same way that execvp() would locate it. Returns an allocated string
 * with the absolute path name, or %NULL if the program is not found in
 * the path. If @program is already an absolute path, returns a copy of
 * @program if @program exists and is executable, and %NULL otherwise.
 *  
 * On Windows, if @program does not have a file type suffix, tries
 * with the suffixes .exe, .cmd, .bat and .com, and the suffixes in
 * the <envar>PATHEXT</envar> environment variable. 
 * 
 * On Windows, it looks for the file in the same way as CreateProcess() 
 * would. This means first in the directory where the executing
 * program was loaded from, then in the current directory, then in the
 * Windows 32-bit system directory, then in the Windows directory, and
 * finally in the directories in the <envar>PATH</envar> environment 
 * variable. If the program is found, the return value contains the 
 * full name including the type suffix.
 *
 * Return value: absolute path, or %NULL
 **/
#ifdef G_OS_WIN32
static gchar *
inner_find_program_in_path (const gchar *program)
#else
gchar*
g_find_program_in_path (const gchar *program)
#endif
{
  const gchar *path, *p;
  gchar *name, *freeme;
#ifdef G_OS_WIN32
  const gchar *path_copy;
  gchar *filename = NULL, *appdir = NULL;
  gchar *sysdir = NULL, *windir = NULL;
  int n;
  wchar_t wfilename[MAXPATHLEN], wsysdir[MAXPATHLEN],
    wwindir[MAXPATHLEN];
#endif
  gsize len;
  gsize pathlen;

  g_return_val_if_fail (program != NULL, NULL);

  /* If it is an absolute path, or a relative path including subdirectories,
   * don't look in PATH.
   */
  if (g_path_is_absolute (program)
      || strchr (program, G_DIR_SEPARATOR) != NULL
#ifdef G_OS_WIN32
      || strchr (program, '/') != NULL
#endif
      )
    {
      if (g_file_test (program, G_FILE_TEST_IS_EXECUTABLE) &&
	  !g_file_test (program, G_FILE_TEST_IS_DIR))
        return g_strdup (program);
      else
        return NULL;
    }
  
  path = g_getenv ("PATH");
#if defined(G_OS_UNIX) || defined(G_OS_BEOS)
  if (path == NULL)
    {
      /* There is no `PATH' in the environment.  The default
       * search path in GNU libc is the current directory followed by
       * the path `confstr' returns for `_CS_PATH'.
       */
      
      /* In GLib we put . last, for security, and don't use the
       * unportable confstr(); UNIX98 does not actually specify
       * what to search if PATH is unset. POSIX may, dunno.
       */
      
      path = "/bin:/usr/bin:.";
    }
#else
  n = GetModuleFileNameW (NULL, wfilename, MAXPATHLEN);
  if (n > 0 && n < MAXPATHLEN)
    filename = g_utf16_to_utf8 (wfilename, -1, NULL, NULL, NULL);
  
  n = GetSystemDirectoryW (wsysdir, MAXPATHLEN);
  if (n > 0 && n < MAXPATHLEN)
    sysdir = g_utf16_to_utf8 (wsysdir, -1, NULL, NULL, NULL);
  
  n = GetWindowsDirectoryW (wwindir, MAXPATHLEN);
  if (n > 0 && n < MAXPATHLEN)
    windir = g_utf16_to_utf8 (wwindir, -1, NULL, NULL, NULL);
  
  if (filename)
    {
      appdir = g_path_get_dirname (filename);
      g_free (filename);
    }
  
  path = g_strdup (path);

  if (windir)
    {
      const gchar *tem = path;
      path = g_strconcat (windir, ";", path, NULL);
      g_free ((gchar *) tem);
      g_free (windir);
    }
  
  if (sysdir)
    {
      const gchar *tem = path;
      path = g_strconcat (sysdir, ";", path, NULL);
      g_free ((gchar *) tem);
      g_free (sysdir);
    }
  
  {
    const gchar *tem = path;
    path = g_strconcat (".;", path, NULL);
    g_free ((gchar *) tem);
  }
  
  if (appdir)
    {
      const gchar *tem = path;
      path = g_strconcat (appdir, ";", path, NULL);
      g_free ((gchar *) tem);
      g_free (appdir);
    }

  path_copy = path;
#endif
  
  len = strlen (program) + 1;
  pathlen = strlen (path);
  freeme = name = g_malloc (pathlen + len + 1);
  
  /* Copy the file name at the top, including '\0'  */
  memcpy (name + pathlen + 1, program, len);
  name = name + pathlen;
  /* And add the slash before the filename  */
  *name = G_DIR_SEPARATOR;
  
  p = path;
  do
    {
      char *startp;

      path = p;
      p = my_strchrnul (path, G_SEARCHPATH_SEPARATOR);

      if (p == path)
        /* Two adjacent colons, or a colon at the beginning or the end
         * of `PATH' means to search the current directory.
         */
        startp = name + 1;
      else
        startp = memcpy (name - (p - path), path, p - path);

      if (g_file_test (startp, G_FILE_TEST_IS_EXECUTABLE) &&
	  !g_file_test (startp, G_FILE_TEST_IS_DIR))
        {
          gchar *ret;
          ret = g_strdup (startp);
          g_free (freeme);
#ifdef G_OS_WIN32
	  g_free ((gchar *) path_copy);
#endif
          return ret;
        }
    }
  while (*p++ != '\0');
  
  g_free (freeme);
#ifdef G_OS_WIN32
  g_free ((gchar *) path_copy);
#endif

  return NULL;
}

static gboolean
debug_key_matches (const gchar *key,
		   const gchar *token,
		   guint        length)
{
  for (; length; length--, key++, token++)
    {
      char k = (*key   == '_') ? '-' : tolower (*key  );
      char t = (*token == '_') ? '-' : tolower (*token);

      if (k != t)
        return FALSE;
    }

  return *key == '\0';
}

/**
 * g_parse_debug_string:
 * @string: a list of debug options separated by colons, spaces, or
 * commas, or %NULL.
 * @keys: pointer to an array of #GDebugKey which associate 
 *     strings with bit flags.
 * @nkeys: the number of #GDebugKey<!-- -->s in the array.
 *
 * Parses a string containing debugging options
 * into a %guint containing bit flags. This is used 
 * within GDK and GTK+ to parse the debug options passed on the
 * command line or through environment variables.
 *
 * If @string is equal to "all", all flags are set.  If @string
 * is equal to "help", all the available keys in @keys are printed
 * out to standard error.
 *
 * Returns: the combined set of bit flags.
 */
guint	     
g_parse_debug_string  (const gchar     *string, 
		       const GDebugKey *keys, 
		       guint	        nkeys)
{
  guint i;
  guint result = 0;
  
  if (string == NULL)
    return 0;

  /* this function is used by gmem.c/gslice.c initialization code,
   * so introducing malloc dependencies here would require adaptions
   * of those code portions.
   */
  
  if (!g_ascii_strcasecmp (string, "all"))
    {
      for (i=0; i<nkeys; i++)
	result |= keys[i].value;
    }
  else if (!g_ascii_strcasecmp (string, "help"))
    {
      /* using stdio directly for the reason stated above */
      fprintf (stderr, "Supported debug values: ");
      for (i=0; i<nkeys; i++)
	fprintf (stderr, " %s", keys[i].key);
      fprintf (stderr, "\n");
    }
  else
    {
      const gchar *p = string;
      const gchar *q;
      
      while (*p)
	{
	  q = strpbrk (p, ":;, \t");
	  if (!q)
	    q = p + strlen(p);
	  
	  for (i = 0; i < nkeys; i++)
	    if (debug_key_matches (keys[i].key, p, q - p))
	      result |= keys[i].value;
	  
	  p = q;
	  if (*p)
	    p++;
	}
    }
  
  return result;
}

/**
 * g_basename:
 * @file_name: the name of the file.
 * 
 * Gets the name of the file without any leading directory components.  
 * It returns a pointer into the given file name string.
 * 
 * Return value: the name of the file without any leading directory components.
 *
 * Deprecated:2.2: Use g_path_get_basename() instead, but notice that
 * g_path_get_basename() allocates new memory for the returned string, unlike
 * this function which returns a pointer into the argument.
 **/
G_CONST_RETURN gchar*
g_basename (const gchar	   *file_name)
{
  register gchar *base;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  
  base = strrchr (file_name, G_DIR_SEPARATOR);

#ifdef G_OS_WIN32
  {
    gchar *q = strrchr (file_name, '/');
    if (base == NULL || (q != NULL && q > base))
	base = q;
  }
#endif

  if (base)
    return base + 1;

#ifdef G_OS_WIN32
  if (g_ascii_isalpha (file_name[0]) && file_name[1] == ':')
    return (gchar*) file_name + 2;
#endif /* G_OS_WIN32 */
  
  return (gchar*) file_name;
}

/**
 * g_path_get_basename:
 * @file_name: the name of the file.
 *
 * Gets the last component of the filename. If @file_name ends with a 
 * directory separator it gets the component before the last slash. If 
 * @file_name consists only of directory separators (and on Windows, 
 * possibly a drive letter), a single separator is returned. If
 * @file_name is empty, it gets ".".
 *
 * Return value: a newly allocated string containing the last component of 
 *   the filename.
 */
gchar*
g_path_get_basename (const gchar   *file_name)
{
  register gssize base;             
  register gssize last_nonslash;    
  gsize len;    
  gchar *retval;
 
  g_return_val_if_fail (file_name != NULL, NULL);

  if (file_name[0] == '\0')
    /* empty string */
    return g_strdup (".");
  
  last_nonslash = strlen (file_name) - 1;

  while (last_nonslash >= 0 && G_IS_DIR_SEPARATOR (file_name [last_nonslash]))
    last_nonslash--;

  if (last_nonslash == -1)
    /* string only containing slashes */
    return g_strdup (G_DIR_SEPARATOR_S);

#ifdef G_OS_WIN32
  if (last_nonslash == 1 && g_ascii_isalpha (file_name[0]) && file_name[1] == ':')
    /* string only containing slashes and a drive */
    return g_strdup (G_DIR_SEPARATOR_S);
#endif /* G_OS_WIN32 */

  base = last_nonslash;

  while (base >=0 && !G_IS_DIR_SEPARATOR (file_name [base]))
    base--;

#ifdef G_OS_WIN32
  if (base == -1 && g_ascii_isalpha (file_name[0]) && file_name[1] == ':')
    base = 1;
#endif /* G_OS_WIN32 */

  len = last_nonslash - base;
  retval = g_malloc (len + 1);
  memcpy (retval, file_name + base + 1, len);
  retval [len] = '\0';
  return retval;
}

/**
 * g_path_is_absolute:
 * @file_name: a file name.
 *
 * Returns %TRUE if the given @file_name is an absolute file name,
 * i.e. it contains a full path from the root directory such as "/usr/local"
 * on UNIX or "C:\windows" on Windows systems.
 *
 * Returns: %TRUE if @file_name is an absolute path. 
 */
gboolean
g_path_is_absolute (const gchar *file_name)
{
  g_return_val_if_fail (file_name != NULL, FALSE);
  
  if (G_IS_DIR_SEPARATOR (file_name[0]))
    return TRUE;

#ifdef G_OS_WIN32
  /* Recognize drive letter on native Windows */
  if (g_ascii_isalpha (file_name[0]) && 
      file_name[1] == ':' && G_IS_DIR_SEPARATOR (file_name[2]))
    return TRUE;
#endif /* G_OS_WIN32 */

  return FALSE;
}

/**
 * g_path_skip_root:
 * @file_name: a file name.
 *
 * Returns a pointer into @file_name after the root component, i.e. after
 * the "/" in UNIX or "C:\" under Windows. If @file_name is not an absolute
 * path it returns %NULL.
 *
 * Returns: a pointer into @file_name after the root component.
 */
G_CONST_RETURN gchar*
g_path_skip_root (const gchar *file_name)
{
  g_return_val_if_fail (file_name != NULL, NULL);
  
#ifdef G_PLATFORM_WIN32
  /* Skip \\server\share or //server/share */
  if (G_IS_DIR_SEPARATOR (file_name[0]) &&
      G_IS_DIR_SEPARATOR (file_name[1]) &&
      file_name[2] &&
      !G_IS_DIR_SEPARATOR (file_name[2]))
    {
      gchar *p;

      p = strchr (file_name + 2, G_DIR_SEPARATOR);
#ifdef G_OS_WIN32
      {
	gchar *q = strchr (file_name + 2, '/');
	if (p == NULL || (q != NULL && q < p))
	  p = q;
      }
#endif
      if (p &&
	  p > file_name + 2 &&
	  p[1])
	{
	  file_name = p + 1;

	  while (file_name[0] && !G_IS_DIR_SEPARATOR (file_name[0]))
	    file_name++;

	  /* Possibly skip a backslash after the share name */
	  if (G_IS_DIR_SEPARATOR (file_name[0]))
	    file_name++;

	  return (gchar *)file_name;
	}
    }
#endif
  
  /* Skip initial slashes */
  if (G_IS_DIR_SEPARATOR (file_name[0]))
    {
      while (G_IS_DIR_SEPARATOR (file_name[0]))
	file_name++;
      return (gchar *)file_name;
    }

#ifdef G_OS_WIN32
  /* Skip X:\ */
  if (g_ascii_isalpha (file_name[0]) && file_name[1] == ':' && G_IS_DIR_SEPARATOR (file_name[2]))
    return (gchar *)file_name + 3;
#endif

  return NULL;
}

/**
 * g_path_get_dirname:
 * @file_name: the name of the file.
 *
 * Gets the directory components of a file name.  If the file name has no
 * directory components "." is returned.  The returned string should be
 * freed when no longer needed.
 * 
 * Returns: the directory components of the file.
 */
gchar*
g_path_get_dirname (const gchar	   *file_name)
{
  register gchar *base;
  register gsize len;    
  
  g_return_val_if_fail (file_name != NULL, NULL);
  
  base = strrchr (file_name, G_DIR_SEPARATOR);
#ifdef G_OS_WIN32
  {
    gchar *q = strrchr (file_name, '/');
    if (base == NULL || (q != NULL && q > base))
	base = q;
  }
#endif
  if (!base)
    {
#ifdef G_OS_WIN32
      if (g_ascii_isalpha (file_name[0]) && file_name[1] == ':')
	{
	  gchar drive_colon_dot[4];

	  drive_colon_dot[0] = file_name[0];
	  drive_colon_dot[1] = ':';
	  drive_colon_dot[2] = '.';
	  drive_colon_dot[3] = '\0';

	  return g_strdup (drive_colon_dot);
	}
#endif
    return g_strdup (".");
    }

  while (base > file_name && G_IS_DIR_SEPARATOR (*base))
    base--;

#ifdef G_OS_WIN32
  /* base points to the char before the last slash.
   *
   * In case file_name is the root of a drive (X:\) or a child of the
   * root of a drive (X:\foo), include the slash.
   *
   * In case file_name is the root share of an UNC path
   * (\\server\share), add a slash, returning \\server\share\ .
   *
   * In case file_name is a direct child of a share in an UNC path
   * (\\server\share\foo), include the slash after the share name,
   * returning \\server\share\ .
   */
  if (base == file_name + 1 && g_ascii_isalpha (file_name[0]) && file_name[1] == ':')
    base++;
  else if (G_IS_DIR_SEPARATOR (file_name[0]) &&
	   G_IS_DIR_SEPARATOR (file_name[1]) &&
	   file_name[2] &&
	   !G_IS_DIR_SEPARATOR (file_name[2]) &&
	   base >= file_name + 2)
    {
      const gchar *p = file_name + 2;
      while (*p && !G_IS_DIR_SEPARATOR (*p))
	p++;
      if (p == base + 1)
	{
	  len = (guint) strlen (file_name) + 1;
	  base = g_new (gchar, len + 1);
	  strcpy (base, file_name);
	  base[len-1] = G_DIR_SEPARATOR;
	  base[len] = 0;
	  return base;
	}
      if (G_IS_DIR_SEPARATOR (*p))
	{
	  p++;
	  while (*p && !G_IS_DIR_SEPARATOR (*p))
	    p++;
	  if (p == base + 1)
	    base++;
	}
    }
#endif

  len = (guint) 1 + base - file_name;
  
  base = g_new (gchar, len + 1);
  g_memmove (base, file_name, len);
  base[len] = 0;
  
  return base;
}

/**
 * g_get_current_dir:
 *
 * Gets the current directory.
 * The returned string should be freed when no longer needed. The encoding 
 * of the returned string is system defined. On Windows, it is always UTF-8.
 * 
 * Returns: the current directory.
 */
gchar*
g_get_current_dir (void)
{
#ifdef G_OS_WIN32

  gchar *dir = NULL;
  wchar_t dummy[2], *wdir;
  int len;

  len = GetCurrentDirectoryW (2, dummy);
  wdir = g_new (wchar_t, len);

  if (GetCurrentDirectoryW (len, wdir) == len - 1)
    dir = g_utf16_to_utf8 (wdir, -1, NULL, NULL, NULL);
  
  g_free (wdir);

  if (dir == NULL)
    dir = g_strdup ("\\");

  return dir;

#else

  gchar *buffer = NULL;
  gchar *dir = NULL;
  static gulong max_len = 0;

  if (max_len == 0) 
    max_len = (G_PATH_LENGTH == -1) ? 2048 : G_PATH_LENGTH;
  
  /* We don't use getcwd(3) on SUNOS, because, it does a popen("pwd")
   * and, if that wasn't bad enough, hangs in doing so.
   */
#if	(defined (sun) && !defined (__SVR4)) || !defined(HAVE_GETCWD)
  buffer = g_new (gchar, max_len + 1);
  *buffer = 0;
  dir = getwd (buffer);
#else	/* !sun || !HAVE_GETCWD */
  while (max_len < G_MAXULONG / 2)
    {
      g_free (buffer);
      buffer = g_new (gchar, max_len + 1);
      *buffer = 0;
      dir = getcwd (buffer, max_len);

      if (dir || errno != ERANGE)
	break;

      max_len *= 2;
    }
#endif	/* !sun || !HAVE_GETCWD */
  
  if (!dir || !*buffer)
    {
      /* hm, should we g_error() out here?
       * this can happen if e.g. "./" has mode \0000
       */
      buffer[0] = G_DIR_SEPARATOR;
      buffer[1] = 0;
    }

  dir = g_strdup (buffer);
  g_free (buffer);
  
  return dir;
#endif /* !Win32 */
}

/**
 * g_getenv:
 * @variable: the environment variable to get, in the GLib file name encoding.
 * 
 * Returns the value of an environment variable. The name and value
 * are in the GLib file name encoding. On UNIX, this means the actual
 * bytes which might or might not be in some consistent character set
 * and encoding. On Windows, it is in UTF-8. On Windows, in case the
 * environment variable's value contains references to other
 * environment variables, they are expanded.
 * 
 * Return value: the value of the environment variable, or %NULL if
 * the environment variable is not found. The returned string may be
 * overwritten by the next call to g_getenv(), g_setenv() or
 * g_unsetenv().
 **/
G_CONST_RETURN gchar*
g_getenv (const gchar *variable)
{
#ifndef G_OS_WIN32

  g_return_val_if_fail (variable != NULL, NULL);

  return getenv (variable);

#else /* G_OS_WIN32 */

  GQuark quark;
  gchar *value;
  wchar_t dummy[2], *wname, *wvalue;
  int len;

  g_return_val_if_fail (variable != NULL, NULL);
  g_return_val_if_fail (g_utf8_validate (variable, -1, NULL), NULL);

  /* On Windows NT, it is relatively typical that environment
   * variables contain references to other environment variables. If
   * so, use ExpandEnvironmentStrings(). (In an ideal world, such
   * environment variables would be stored in the Registry as
   * REG_EXPAND_SZ type values, and would then get automatically
   * expanded before a program sees them. But there is broken software
   * that stores environment variables as REG_SZ values even if they
   * contain references to other environment variables.)
   */

  wname = g_utf8_to_utf16 (variable, -1, NULL, NULL, NULL);

  len = GetEnvironmentVariableW (wname, dummy, 2);

  if (len == 0)
    {
      g_free (wname);
      return NULL;
    }
  else if (len == 1)
    len = 2;

  wvalue = g_new (wchar_t, len);

  if (GetEnvironmentVariableW (wname, wvalue, len) != len - 1)
    {
      g_free (wname);
      g_free (wvalue);
      return NULL;
    }

  if (wcschr (wvalue, L'%') != NULL)
    {
      wchar_t *tem = wvalue;

      len = ExpandEnvironmentStringsW (wvalue, dummy, 2);

      if (len > 0)
	{
	  wvalue = g_new (wchar_t, len);

	  if (ExpandEnvironmentStringsW (tem, wvalue, len) != len)
	    {
	      g_free (wvalue);
	      wvalue = tem;
	    }
	  else
	    g_free (tem);
	}
    }

  value = g_utf16_to_utf8 (wvalue, -1, NULL, NULL, NULL);

  g_free (wname);
  g_free (wvalue);

  quark = g_quark_from_string (value);
  g_free (value);
  
  return g_quark_to_string (quark);

#endif /* G_OS_WIN32 */
}

/* _g_getenv_nomalloc
 * this function does a getenv() without doing any kind of allocation
 * through glib. it's suitable for chars <= 127 only (both, for the
 * variable name and the contents) and for contents < 1024 chars in
 * length. also, it aliases "" to a NULL return value.
 **/
const gchar*
_g_getenv_nomalloc (const gchar *variable,
                    gchar        buffer[1024])
{
  const gchar *retval = getenv (variable);
  if (retval && retval[0])
    {
      gint l = strlen (retval);
      if (l < 1024)
        {
          strncpy (buffer, retval, l);
          buffer[l] = 0;
          return buffer;
        }
    }
  return NULL;
}

/**
 * g_setenv:
 * @variable: the environment variable to set, must not contain '='.
 * @value: the value for to set the variable to.
 * @overwrite: whether to change the variable if it already exists.
 *
 * Sets an environment variable. Both the variable's name and value
 * should be in the GLib file name encoding. On UNIX, this means that
 * they can be any sequence of bytes. On Windows, they should be in
 * UTF-8.
 *
 * Note that on some systems, when variables are overwritten, the memory 
 * used for the previous variables and its value isn't reclaimed.
 *
 * Returns: %FALSE if the environment variable couldn't be set.
 *
 * Since: 2.4
 */
gboolean
g_setenv (const gchar *variable, 
	  const gchar *value, 
	  gboolean     overwrite)
{
#ifndef G_OS_WIN32

  gint result;
#ifndef HAVE_SETENV
  gchar *string;
#endif

  g_return_val_if_fail (variable != NULL, FALSE);
  g_return_val_if_fail (strchr (variable, '=') == NULL, FALSE);

#ifdef HAVE_SETENV
  result = setenv (variable, value, overwrite);
#else
  if (!overwrite && getenv (variable) != NULL)
    return TRUE;
  
  /* This results in a leak when you overwrite existing
   * settings. It would be fairly easy to fix this by keeping
   * our own parallel array or hash table.
   */
  string = g_strconcat (variable, "=", value, NULL);
  result = putenv (string);
#endif
  return result == 0;

#else /* G_OS_WIN32 */

  gboolean retval;
  wchar_t *wname, *wvalue, *wassignment;
  gchar *tem;

  g_return_val_if_fail (variable != NULL, FALSE);
  g_return_val_if_fail (strchr (variable, '=') == NULL, FALSE);
  g_return_val_if_fail (g_utf8_validate (variable, -1, NULL), FALSE);
  g_return_val_if_fail (g_utf8_validate (value, -1, NULL), FALSE);

  if (!overwrite && g_getenv (variable) != NULL)
    return TRUE;

  /* We want to (if possible) set both the environment variable copy
   * kept by the C runtime and the one kept by the system.
   *
   * We can't use only the C runtime's putenv or _wputenv() as that
   * won't work for arbitrary Unicode strings in a "non-Unicode" app
   * (with main() and not wmain()). In a "main()" app the C runtime
   * initializes the C runtime's environment table by converting the
   * real (wide char) environment variables to system codepage, thus
   * breaking those that aren't representable in the system codepage.
   *
   * As the C runtime's putenv() will also set the system copy, we do
   * the putenv() first, then call SetEnvironmentValueW ourselves.
   */

  wname = g_utf8_to_utf16 (variable, -1, NULL, NULL, NULL);
  wvalue = g_utf8_to_utf16 (value, -1, NULL, NULL, NULL);
  tem = g_strconcat (variable, "=", value, NULL);
  wassignment = g_utf8_to_utf16 (tem, -1, NULL, NULL, NULL);
    
  g_free (tem);
  _wputenv (wassignment);
  g_free (wassignment);

  retval = (SetEnvironmentVariableW (wname, wvalue) != 0);

  g_free (wname);
  g_free (wvalue);

  return retval;

#endif /* G_OS_WIN32 */
}

#ifdef HAVE__NSGETENVIRON
#define environ (*_NSGetEnviron())
#elif !defined(G_OS_WIN32)

/* According to the Single Unix Specification, environ is not in 
 * any system header, although unistd.h often declares it.
 */
extern char **environ;
#endif

/**
 * g_unsetenv:
 * @variable: the environment variable to remove, must not contain '='.
 * 
 * Removes an environment variable from the environment.
 *
 * Note that on some systems, when variables are overwritten, the memory 
 * used for the previous variables and its value isn't reclaimed.
 * Furthermore, this function can't be guaranteed to operate in a 
 * threadsafe way.
 *
 * Since: 2.4 
 **/
void
g_unsetenv (const gchar *variable)
{
#ifndef G_OS_WIN32

#ifdef HAVE_UNSETENV
  g_return_if_fail (variable != NULL);
  g_return_if_fail (strchr (variable, '=') == NULL);

  unsetenv (variable);
#else /* !HAVE_UNSETENV */
  int len;
  gchar **e, **f;

  g_return_if_fail (variable != NULL);
  g_return_if_fail (strchr (variable, '=') == NULL);

  len = strlen (variable);
  
  /* Mess directly with the environ array.
   * This seems to be the only portable way to do this.
   *
   * Note that we remove *all* environment entries for
   * the variable name, not just the first.
   */
  e = f = environ;
  while (*e != NULL) 
    {
      if (strncmp (*e, variable, len) != 0 || (*e)[len] != '=') 
	{
	  *f = *e;
	  f++;
	}
      e++;
    }
  *f = NULL;
#endif /* !HAVE_UNSETENV */

#else  /* G_OS_WIN32 */

  wchar_t *wname, *wassignment;
  gchar *tem;

  g_return_if_fail (variable != NULL);
  g_return_if_fail (strchr (variable, '=') == NULL);
  g_return_if_fail (g_utf8_validate (variable, -1, NULL));

  wname = g_utf8_to_utf16 (variable, -1, NULL, NULL, NULL);
  tem = g_strconcat (variable, "=", NULL);
  wassignment = g_utf8_to_utf16 (tem, -1, NULL, NULL, NULL);
    
  g_free (tem);
  _wputenv (wassignment);
  g_free (wassignment);

  SetEnvironmentVariableW (wname, NULL);

  g_free (wname);

#endif /* G_OS_WIN32 */
}

/**
 * g_listenv:
 *
 * Gets the names of all variables set in the environment.
 * 
 * Returns: a %NULL-terminated list of strings which must be freed
 * with g_strfreev().
 *
 * Programs that want to be portable to Windows should typically use
 * this function and g_getenv() instead of using the environ array
 * from the C library directly. On Windows, the strings in the environ
 * array are in system codepage encoding, while in most of the typical
 * use cases for environment variables in GLib-using programs you want
 * the UTF-8 encoding that this function and g_getenv() provide.
 *
 * Since: 2.8
 */
gchar **
g_listenv (void)
{
#ifndef G_OS_WIN32
  gchar **result, *eq;
  gint len, i, j;

  len = g_strv_length (environ);
  result = g_new0 (gchar *, len + 1);
  
  j = 0;
  for (i = 0; i < len; i++)
    {
      eq = strchr (environ[i], '=');
      if (eq)
	result[j++] = g_strndup (environ[i], eq - environ[i]);
    }

  result[j] = NULL;

  return result;
#else
  gchar **result, *eq;
  gint len = 0, j;
  wchar_t *p, *q;

  p = (wchar_t *) GetEnvironmentStringsW ();
  if (p != NULL)
    {
      q = p;
      while (*q)
	{
	  q += wcslen (q) + 1;
	  len++;
	}
    }
  result = g_new0 (gchar *, len + 1);

  j = 0;
  q = p;
  while (*q)
    {
      result[j] = g_utf16_to_utf8 (q, -1, NULL, NULL, NULL);
      if (result[j] != NULL)
	{
	  eq = strchr (result[j], '=');
	  if (eq && eq > result[j])
	    {
	      *eq = '\0';
	      j++;
	    }
	  else
	    g_free (result[j]);
	}
      q += wcslen (q) + 1;
    }
  result[j] = NULL;
  FreeEnvironmentStringsW (p);

  return result;
#endif
}

G_LOCK_DEFINE_STATIC (g_utils_global);

static	gchar	*g_tmp_dir = NULL;
static	gchar	*g_user_name = NULL;
static	gchar	*g_real_name = NULL;
static	gchar	*g_home_dir = NULL;
static	gchar	*g_host_name = NULL;

#ifdef G_OS_WIN32
/* System codepage versions of the above, kept at file level so that they,
 * too, are produced only once.
 */
static	gchar	*g_tmp_dir_cp = NULL;
static	gchar	*g_user_name_cp = NULL;
static	gchar	*g_real_name_cp = NULL;
static	gchar	*g_home_dir_cp = NULL;
#endif

static  gchar   *g_user_data_dir = NULL;
static  gchar  **g_system_data_dirs = NULL;
static  gchar   *g_user_cache_dir = NULL;
static  gchar   *g_user_config_dir = NULL;
static  gchar  **g_system_config_dirs = NULL;

static  gchar  **g_user_special_dirs = NULL;

/* fifteen minutes of fame for everybody */
#define G_USER_DIRS_EXPIRE      15 * 60

#ifdef G_OS_WIN32

static gchar *
get_special_folder (int csidl)
{
  wchar_t path[MAX_PATH+1];
  HRESULT hr;
  LPITEMIDLIST pidl = NULL;
  BOOL b;
  gchar *retval = NULL;

  hr = SHGetSpecialFolderLocation (NULL, csidl, &pidl);
  if (hr == S_OK)
    {
      b = SHGetPathFromIDListW (pidl, path);
      if (b)
	retval = g_utf16_to_utf8 (path, -1, NULL, NULL, NULL);
      CoTaskMemFree (pidl);
    }
  return retval;
}

static char *
get_windows_directory_root (void)
{
  wchar_t wwindowsdir[MAX_PATH];

  if (GetWindowsDirectoryW (wwindowsdir, G_N_ELEMENTS (wwindowsdir)))
    {
      /* Usually X:\Windows, but in terminal server environments
       * might be an UNC path, AFAIK.
       */
      char *windowsdir = g_utf16_to_utf8 (wwindowsdir, -1, NULL, NULL, NULL);
      char *p;

      if (windowsdir == NULL)
	return g_strdup ("C:\\");

      p = (char *) g_path_skip_root (windowsdir);
      if (G_IS_DIR_SEPARATOR (p[-1]) && p[-2] != ':')
	p--;
      *p = '\0';
      return windowsdir;
    }
  else
    return g_strdup ("C:\\");
}

#endif

/* HOLDS: g_utils_global_lock */
static void
g_get_any_init_do (void)
{
  gchar hostname[100];

  g_tmp_dir = g_strdup (g_getenv ("TMPDIR"));
  if (!g_tmp_dir)
    g_tmp_dir = g_strdup (g_getenv ("TMP"));
  if (!g_tmp_dir)
    g_tmp_dir = g_strdup (g_getenv ("TEMP"));

#ifdef G_OS_WIN32
  if (!g_tmp_dir)
    g_tmp_dir = get_windows_directory_root ();
#else  
#ifdef P_tmpdir
  if (!g_tmp_dir)
    {
      gsize k;    
      g_tmp_dir = g_strdup (P_tmpdir);
      k = strlen (g_tmp_dir);
      if (k > 1 && G_IS_DIR_SEPARATOR (g_tmp_dir[k - 1]))
	g_tmp_dir[k - 1] = '\0';
    }
#endif
  
  if (!g_tmp_dir)
    {
      g_tmp_dir = g_strdup ("/tmp");
    }
#endif	/* !G_OS_WIN32 */
  
#ifdef G_OS_WIN32
  /* We check $HOME first for Win32, though it is a last resort for Unix
   * where we prefer the results of getpwuid().
   */
  g_home_dir = g_strdup (g_getenv ("HOME"));

  /* Only believe HOME if it is an absolute path and exists */
  if (g_home_dir)
    {
      if (!(g_path_is_absolute (g_home_dir) &&
	    g_file_test (g_home_dir, G_FILE_TEST_IS_DIR)))
	{
	  g_free (g_home_dir);
	  g_home_dir = NULL;
	}
    }
  
  /* In case HOME is Unix-style (it happens), convert it to
   * Windows style.
   */
  if (g_home_dir)
    {
      gchar *p;
      while ((p = strchr (g_home_dir, '/')) != NULL)
	*p = '\\';
    }

  if (!g_home_dir)
    {
      /* USERPROFILE is probably the closest equivalent to $HOME? */
      if (g_getenv ("USERPROFILE") != NULL)
	g_home_dir = g_strdup (g_getenv ("USERPROFILE"));
    }

  if (!g_home_dir)
    g_home_dir = get_special_folder (CSIDL_PROFILE);
  
  if (!g_home_dir)
    g_home_dir = get_windows_directory_root ();
#endif /* G_OS_WIN32 */
  
#ifdef HAVE_PWD_H
  {
    struct passwd *pw = NULL;
    gpointer buffer = NULL;
    gint error;
    gchar *logname;

#  if defined (HAVE_POSIX_GETPWUID_R) || defined (HAVE_NONPOSIX_GETPWUID_R)
    struct passwd pwd;
#    ifdef _SC_GETPW_R_SIZE_MAX  
    /* This reurns the maximum length */
    glong bufsize = sysconf (_SC_GETPW_R_SIZE_MAX);
    
    if (bufsize < 0)
      bufsize = 64;
#    else /* _SC_GETPW_R_SIZE_MAX */
    glong bufsize = 64;
#    endif /* _SC_GETPW_R_SIZE_MAX */

    logname = (gchar *) g_getenv ("LOGNAME");
        
    do
      {
	g_free (buffer);
	/* we allocate 6 extra bytes to work around a bug in 
	 * Mac OS < 10.3. See #156446
	 */
	buffer = g_malloc (bufsize + 6);
	errno = 0;
	
#    ifdef HAVE_POSIX_GETPWUID_R
	if (logname) {
	  error = getpwnam_r (logname, &pwd, buffer, bufsize, &pw);
	  if (!pw || (pw->pw_uid != getuid ())) {
	    /* LOGNAME is lying, fall back to looking up the uid */
	    error = getpwuid_r (getuid (), &pwd, buffer, bufsize, &pw);
	  }
	} else {
	  error = getpwuid_r (getuid (), &pwd, buffer, bufsize, &pw);
	}
	error = error < 0 ? errno : error;
#    else /* HAVE_NONPOSIX_GETPWUID_R */
   /* HPUX 11 falls into the HAVE_POSIX_GETPWUID_R case */
#      if defined(_AIX) || defined(__hpux)
	error = getpwuid_r (getuid (), &pwd, buffer, bufsize);
	pw = error == 0 ? &pwd : NULL;
#      else /* !_AIX */
	if (logname) {
	  pw = getpwnam_r (logname, &pwd, buffer, bufsize);
	  if (!pw || (pw->pw_uid != getuid ())) {
	    /* LOGNAME is lying, fall back to looking up the uid */
	    pw = getpwuid_r (getuid (), &pwd, buffer, bufsize);
	  }
	} else {
	  pw = getpwuid_r (getuid (), &pwd, buffer, bufsize);
	}
	error = pw ? 0 : errno;
#      endif /* !_AIX */            
#    endif /* HAVE_NONPOSIX_GETPWUID_R */
	
	if (!pw)
	  {
	    /* we bail out prematurely if the user id can't be found
	     * (should be pretty rare case actually), or if the buffer
	     * should be sufficiently big and lookups are still not
	     * successfull.
	     */
	    if (error == 0 || error == ENOENT)
	      {
		g_warning ("getpwuid_r(): failed due to unknown user id (%lu)",
			   (gulong) getuid ());
		break;
	      }
	    if (bufsize > 32 * 1024)
	      {
		g_warning ("getpwuid_r(): failed due to: %s.",
			   g_strerror (error));
		break;
	      }
	    
	    bufsize *= 2;
	  }
      }
    while (!pw);
#  endif /* HAVE_POSIX_GETPWUID_R || HAVE_NONPOSIX_GETPWUID_R */
    
    if (!pw)
      {
	setpwent ();
	pw = getpwuid (getuid ());
	endpwent ();
      }
    if (pw)
      {
	g_user_name = g_strdup (pw->pw_name);

	if (pw->pw_gecos && *pw->pw_gecos != '\0') 
	  {
	    gchar **gecos_fields;
	    gchar **name_parts;

	    /* split the gecos field and substitute '&' */
	    gecos_fields = g_strsplit (pw->pw_gecos, ",", 0);
	    name_parts = g_strsplit (gecos_fields[0], "&", 0);
	    pw->pw_name[0] = g_ascii_toupper (pw->pw_name[0]);
	    g_real_name = g_strjoinv (pw->pw_name, name_parts);
	    g_strfreev (gecos_fields);
	    g_strfreev (name_parts);
	  }

	if (!g_home_dir)
	  g_home_dir = g_strdup (pw->pw_dir);
      }
    g_free (buffer);
  }
  
#else /* !HAVE_PWD_H */
  
#ifdef G_OS_WIN32
  {
    guint len = UNLEN+1;
    wchar_t buffer[UNLEN+1];
    
    if (GetUserNameW (buffer, (LPDWORD) &len))
      {
	g_user_name = g_utf16_to_utf8 (buffer, -1, NULL, NULL, NULL);
	g_real_name = g_strdup (g_user_name);
      }
  }
#endif /* G_OS_WIN32 */

#endif /* !HAVE_PWD_H */

#ifndef G_OS_WIN32
  if (!g_home_dir)
    g_home_dir = g_strdup (g_getenv ("HOME"));
#endif

#ifdef __EMX__
  /* change '\\' in %HOME% to '/' */
  g_strdelimit (g_home_dir, "\\",'/');
#endif
  if (!g_user_name)
    g_user_name = g_strdup ("somebody");
  if (!g_real_name)
    g_real_name = g_strdup ("Unknown");

  {
#ifndef G_OS_WIN32
    gboolean hostname_fail = (gethostname (hostname, sizeof (hostname)) == -1);
#else
    DWORD size = sizeof (hostname);
    gboolean hostname_fail = (!GetComputerName (hostname, &size));
#endif
    g_host_name = g_strdup (hostname_fail ? "localhost" : hostname);
  }

#ifdef G_OS_WIN32
  g_tmp_dir_cp = g_locale_from_utf8 (g_tmp_dir, -1, NULL, NULL, NULL);
  g_user_name_cp = g_locale_from_utf8 (g_user_name, -1, NULL, NULL, NULL);
  g_real_name_cp = g_locale_from_utf8 (g_real_name, -1, NULL, NULL, NULL);

  if (!g_tmp_dir_cp)
    g_tmp_dir_cp = g_strdup ("\\");
  if (!g_user_name_cp)
    g_user_name_cp = g_strdup ("somebody");
  if (!g_real_name_cp)
    g_real_name_cp = g_strdup ("Unknown");

  /* home_dir might be NULL, unlike tmp_dir, user_name and
   * real_name.
   */
  if (g_home_dir)
    g_home_dir_cp = g_locale_from_utf8 (g_home_dir, -1, NULL, NULL, NULL);
  else
    g_home_dir_cp = NULL;
#endif /* G_OS_WIN32 */
}

static inline void
g_get_any_init (void)
{
  if (!g_tmp_dir)
    g_get_any_init_do ();
}

static inline void
g_get_any_init_locked (void)
{
  G_LOCK (g_utils_global);
  g_get_any_init ();
  G_UNLOCK (g_utils_global);
}


/**
 * g_get_user_name:
 *
 * Gets the user name of the current user. The encoding of the returned
 * string is system-defined. On UNIX, it might be the preferred file name
 * encoding, or something else, and there is no guarantee that it is even
 * consistent on a machine. On Windows, it is always UTF-8.
 *
 * Returns: the user name of the current user.
 */
G_CONST_RETURN gchar*
g_get_user_name (void)
{
  g_get_any_init_locked ();
  return g_user_name;
}

/**
 * g_get_real_name:
 *
 * Gets the real name of the user. This usually comes from the user's entry 
 * in the <filename>passwd</filename> file. The encoding of the returned 
 * string is system-defined. (On Windows, it is, however, always UTF-8.) 
 * If the real user name cannot be determined, the string "Unknown" is 
 * returned.
 *
 * Returns: the user's real name.
 */
G_CONST_RETURN gchar*
g_get_real_name (void)
{
  g_get_any_init_locked ();
  return g_real_name;
}

/**
 * g_get_home_dir:
 *
 * Gets the current user's home directory as defined in the 
 * password database.
 *
 * Note that in contrast to traditional UNIX tools, this function 
 * prefers <filename>passwd</filename> entries over the <envar>HOME</envar> 
 * environment variable. 
 *
 * One of the reasons for this decision is that applications in many 
 * cases need special handling to deal with the case where 
 * <envar>HOME</envar> is
 * <simplelist>
 *   <member>Not owned by the user</member>
 *   <member>Not writeable</member>
 *   <member>Not even readable</member>
 * </simplelist>
 * Since applications are in general <emphasis>not</emphasis> written 
 * to deal with these situations it was considered better to make 
 * g_get_home_dir() not pay attention to <envar>HOME</envar> and to 
 * return the real home directory for the user. If applications
 * want to pay attention to <envar>HOME</envar>, they can do:
 * |[
 *  const char *homedir = g_getenv ("HOME");
 *   if (!homedir)
 *      homedir = g_get_home_dir (<!-- -->);
 * ]|
 *
 * Returns: the current user's home directory
 */
G_CONST_RETURN gchar*
g_get_home_dir (void)
{
  g_get_any_init_locked ();
  return g_home_dir;
}

/**
 * g_get_tmp_dir:
 *
 * Gets the directory to use for temporary files. This is found from 
 * inspecting the environment variables <envar>TMPDIR</envar>, 
 * <envar>TMP</envar>, and <envar>TEMP</envar> in that order. If none 
 * of those are defined "/tmp" is returned on UNIX and "C:\" on Windows. 
 * The encoding of the returned string is system-defined. On Windows, 
 * it is always UTF-8. The return value is never %NULL.
 *
 * Returns: the directory to use for temporary files.
 */
G_CONST_RETURN gchar*
g_get_tmp_dir (void)
{
  g_get_any_init_locked ();
  return g_tmp_dir;
}

/**
 * g_get_host_name:
 *
 * Return a name for the machine. 
 *
 * The returned name is not necessarily a fully-qualified domain name,
 * or even present in DNS or some other name service at all. It need
 * not even be unique on your local network or site, but usually it
 * is. Callers should not rely on the return value having any specific
 * properties like uniqueness for security purposes. Even if the name
 * of the machine is changed while an application is running, the
 * return value from this function does not change. The returned
 * string is owned by GLib and should not be modified or freed. If no
 * name can be determined, a default fixed string "localhost" is
 * returned.
 *
 * Returns: the host name of the machine.
 *
 * Since: 2.8
 */
const gchar *
g_get_host_name (void)
{
  g_get_any_init_locked ();
  return g_host_name;
}

G_LOCK_DEFINE_STATIC (g_prgname);
static gchar *g_prgname = NULL;

/**
 * g_get_prgname:
 *
 * Gets the name of the program. This name should <emphasis>not</emphasis> 
 * be localized, contrast with g_get_application_name().
 * (If you are using GDK or GTK+ the program name is set in gdk_init(), 
 * which is called by gtk_init(). The program name is found by taking 
 * the last component of <literal>argv[0]</literal>.)
 *
 * Returns: the name of the program. The returned string belongs 
 * to GLib and must not be modified or freed.
 */
gchar*
g_get_prgname (void)
{
  gchar* retval;

  G_LOCK (g_prgname);
#ifdef G_OS_WIN32
  if (g_prgname == NULL)
    {
      static gboolean beenhere = FALSE;

      if (!beenhere)
	{
	  gchar *utf8_buf = NULL;
	  wchar_t buf[MAX_PATH+1];

	  beenhere = TRUE;
	  if (GetModuleFileNameW (GetModuleHandle (NULL),
				  buf, G_N_ELEMENTS (buf)) > 0)
	    utf8_buf = g_utf16_to_utf8 (buf, -1, NULL, NULL, NULL);

	  if (utf8_buf)
	    {
	      g_prgname = g_path_get_basename (utf8_buf);
	      g_free (utf8_buf);
	    }
	}
    }
#endif
  retval = g_prgname;
  G_UNLOCK (g_prgname);

  return retval;
}

/**
 * g_set_prgname:
 * @prgname: the name of the program.
 *
 * Sets the name of the program. This name should <emphasis>not</emphasis> 
 * be localized, contrast with g_set_application_name(). Note that for 
 * thread-safety reasons this function can only be called once.
 */
void
g_set_prgname (const gchar *prgname)
{
  G_LOCK (g_prgname);
  g_free (g_prgname);
  g_prgname = g_strdup (prgname);
  G_UNLOCK (g_prgname);
}

G_LOCK_DEFINE_STATIC (g_application_name);
static gchar *g_application_name = NULL;

/**
 * g_get_application_name:
 * 
 * Gets a human-readable name for the application, as set by
 * g_set_application_name(). This name should be localized if
 * possible, and is intended for display to the user.  Contrast with
 * g_get_prgname(), which gets a non-localized name. If
 * g_set_application_name() has not been called, returns the result of
 * g_get_prgname() (which may be %NULL if g_set_prgname() has also not
 * been called).
 * 
 * Return value: human-readable application name. may return %NULL
 *
 * Since: 2.2
 **/
G_CONST_RETURN gchar*
g_get_application_name (void)
{
  gchar* retval;

  G_LOCK (g_application_name);
  retval = g_application_name;
  G_UNLOCK (g_application_name);

  if (retval == NULL)
    return g_get_prgname ();
  
  return retval;
}

/**
 * g_set_application_name:
 * @application_name: localized name of the application
 *
 * Sets a human-readable name for the application. This name should be
 * localized if possible, and is intended for display to the user.
 * Contrast with g_set_prgname(), which sets a non-localized name.
 * g_set_prgname() will be called automatically by gtk_init(),
 * but g_set_application_name() will not.
 *
 * Note that for thread safety reasons, this function can only
 * be called once.
 *
 * The application name will be used in contexts such as error messages,
 * or when displaying an application's name in the task list.
 * 
 * Since: 2.2
 **/
void
g_set_application_name (const gchar *application_name)
{
  gboolean already_set = FALSE;
	
  G_LOCK (g_application_name);
  if (g_application_name)
    already_set = TRUE;
  else
    g_application_name = g_strdup (application_name);
  G_UNLOCK (g_application_name);

  if (already_set)
    g_warning ("g_set_application() name called multiple times");
}

/**
 * g_get_user_data_dir:
 * 
 * Returns a base directory in which to access application data such
 * as icons that is customized for a particular user.  
 *
 * On UNIX platforms this is determined using the mechanisms described in
 * the <ulink url="http://www.freedesktop.org/Standards/basedir-spec">
 * XDG Base Directory Specification</ulink>
 * 
 * Return value: a string owned by GLib that must not be modified 
 *               or freed.
 * Since: 2.6
 **/
G_CONST_RETURN gchar*
g_get_user_data_dir (void)
{
  gchar *data_dir;  

  G_LOCK (g_utils_global);

  if (!g_user_data_dir)
    {
#ifdef G_OS_WIN32
      data_dir = get_special_folder (CSIDL_PERSONAL);
#else
      data_dir = (gchar *) g_getenv ("XDG_DATA_HOME");

      if (data_dir && data_dir[0])
        data_dir = g_strdup (data_dir);
#endif
      if (!data_dir || !data_dir[0])
	{
	  g_get_any_init ();

	  if (g_home_dir)
	    data_dir = g_build_filename (g_home_dir, ".local", 
					 "share", NULL);
	  else
	    data_dir = g_build_filename (g_tmp_dir, g_user_name, ".local", 
					 "share", NULL);
	}

      g_user_data_dir = data_dir;
    }
  else
    data_dir = g_user_data_dir;

  G_UNLOCK (g_utils_global);

  return data_dir;
}

static void
g_init_user_config_dir (void)
{
  gchar *config_dir;

  if (!g_user_config_dir)
    {
#ifdef G_OS_WIN32
      config_dir = get_special_folder (CSIDL_APPDATA);
#else
      config_dir = (gchar *) g_getenv ("XDG_CONFIG_HOME");

      if (config_dir && config_dir[0])
	config_dir = g_strdup (config_dir);
#endif
      if (!config_dir || !config_dir[0])
	{
	  g_get_any_init ();

	  if (g_home_dir)
	    config_dir = g_build_filename (g_home_dir, ".config", NULL);
	  else
	    config_dir = g_build_filename (g_tmp_dir, g_user_name, ".config", NULL);
	}

      g_user_config_dir = config_dir;
    }
}

/**
 * g_get_user_config_dir:
 * 
 * Returns a base directory in which to store user-specific application 
 * configuration information such as user preferences and settings. 
 *
 * On UNIX platforms this is determined using the mechanisms described in
 * the <ulink url="http://www.freedesktop.org/Standards/basedir-spec">
 * XDG Base Directory Specification</ulink>
 * 
 * Return value: a string owned by GLib that must not be modified 
 *               or freed.
 * Since: 2.6
 **/
G_CONST_RETURN gchar*
g_get_user_config_dir (void)
{
  G_LOCK (g_utils_global);

  g_init_user_config_dir ();

  G_UNLOCK (g_utils_global);

  return g_user_config_dir;
}

/**
 * g_get_user_cache_dir:
 * 
 * Returns a base directory in which to store non-essential, cached
 * data specific to particular user.
 *
 * On UNIX platforms this is determined using the mechanisms described in
 * the <ulink url="http://www.freedesktop.org/Standards/basedir-spec">
 * XDG Base Directory Specification</ulink>
 * 
 * Return value: a string owned by GLib that must not be modified 
 *               or freed.
 * Since: 2.6
 **/
G_CONST_RETURN gchar*
g_get_user_cache_dir (void)
{
  gchar *cache_dir;  

  G_LOCK (g_utils_global);

  if (!g_user_cache_dir)
    {
#ifdef G_OS_WIN32
      cache_dir = get_special_folder (CSIDL_INTERNET_CACHE); /* XXX correct? */
#else
      cache_dir = (gchar *) g_getenv ("XDG_CACHE_HOME");

      if (cache_dir && cache_dir[0])
          cache_dir = g_strdup (cache_dir);
#endif
      if (!cache_dir || !cache_dir[0])
	{
	  g_get_any_init ();
	
	  if (g_home_dir)
	    cache_dir = g_build_filename (g_home_dir, ".cache", NULL);
	  else
	    cache_dir = g_build_filename (g_tmp_dir, g_user_name, ".cache", NULL);
	}
      g_user_cache_dir = cache_dir;
    }
  else
    cache_dir = g_user_cache_dir;

  G_UNLOCK (g_utils_global);

  return cache_dir;
}

#ifdef HAVE_CARBON

static gchar *
find_folder (OSType type)
{
  gchar *filename = NULL;
  FSRef  found;

  if (FSFindFolder (kUserDomain, type, kDontCreateFolder, &found) == noErr)
    {
      CFURLRef url = CFURLCreateFromFSRef (kCFAllocatorSystemDefault, &found);

      if (url)
	{
	  CFStringRef path = CFURLCopyFileSystemPath (url, kCFURLPOSIXPathStyle);

	  if (path)
	    {
	      filename = g_strdup (CFStringGetCStringPtr (path, kCFStringEncodingUTF8));

	      if (! filename)
		{
		  filename = g_new0 (gchar, CFStringGetLength (path) * 3 + 1);

		  CFStringGetCString (path, filename,
				      CFStringGetLength (path) * 3 + 1,
				      kCFStringEncodingUTF8);
		}

	      CFRelease (path);
	    }

	  CFRelease (url);
	}
    }

  return filename;
}

static void
load_user_special_dirs (void)
{
  g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] = find_folder (kDesktopFolderType);
  g_user_special_dirs[G_USER_DIRECTORY_DOCUMENTS] = find_folder (kDocumentsFolderType);
  g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = find_folder (kDesktopFolderType); /* XXX correct ? */
  g_user_special_dirs[G_USER_DIRECTORY_MUSIC] = find_folder (kMusicDocumentsFolderType);
  g_user_special_dirs[G_USER_DIRECTORY_PICTURES] = find_folder (kPictureDocumentsFolderType);
  g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = NULL;
  g_user_special_dirs[G_USER_DIRECTORY_TEMPLATES] = NULL;
  g_user_special_dirs[G_USER_DIRECTORY_VIDEOS] = find_folder (kMovieDocumentsFolderType);
}

#endif /* HAVE_CARBON */

#if defined(G_OS_WIN32)
static void
load_user_special_dirs (void)
{
  typedef HRESULT (WINAPI *t_SHGetKnownFolderPath) (const GUID *rfid,
						    DWORD dwFlags,
						    HANDLE hToken,
						    PWSTR *ppszPath);
  t_SHGetKnownFolderPath p_SHGetKnownFolderPath;
  static const GUID FOLDERID_Downloads =
    { 0x374de290, 0x123f, 0x4565, { 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b } };
  static const GUID FOLDERID_Public =
    { 0xDFDF76A2, 0xC82A, 0x4D63, { 0x90, 0x6A, 0x56, 0x44, 0xAC, 0x45, 0x73, 0x85 } };
  wchar_t *wcp;

  p_SHGetKnownFolderPath = (t_SHGetKnownFolderPath) GetProcAddress (LoadLibrary ("shell32.dll"),
								    "SHGetKnownFolderPath");

  g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
  g_user_special_dirs[G_USER_DIRECTORY_DOCUMENTS] = get_special_folder (CSIDL_PERSONAL);

  if (p_SHGetKnownFolderPath == NULL)
    {
      g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
    }
  else
    {
      wcp = NULL;
      (*p_SHGetKnownFolderPath) (&FOLDERID_Downloads, 0, NULL, &wcp);
      g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = g_utf16_to_utf8 (wcp, -1, NULL, NULL, NULL);
      if (g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] == NULL)
	g_user_special_dirs[G_USER_DIRECTORY_DOWNLOAD] = get_special_folder (CSIDL_DESKTOPDIRECTORY);
      CoTaskMemFree (wcp);
    }

  g_user_special_dirs[G_USER_DIRECTORY_MUSIC] = get_special_folder (CSIDL_MYMUSIC);
  g_user_special_dirs[G_USER_DIRECTORY_PICTURES] = get_special_folder (CSIDL_MYPICTURES);

  if (p_SHGetKnownFolderPath == NULL)
    {
      /* XXX */
      g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = get_special_folder (CSIDL_COMMON_DOCUMENTS);
    }
  else
    {
      wcp = NULL;
      (*p_SHGetKnownFolderPath) (&FOLDERID_Public, 0, NULL, &wcp);
      g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = g_utf16_to_utf8 (wcp, -1, NULL, NULL, NULL);
      if (g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] == NULL)
	g_user_special_dirs[G_USER_DIRECTORY_PUBLIC_SHARE] = get_special_folder (CSIDL_COMMON_DOCUMENTS);
      CoTaskMemFree (wcp);
    }
  
  g_user_special_dirs[G_USER_DIRECTORY_TEMPLATES] = get_special_folder (CSIDL_TEMPLATES);
  g_user_special_dirs[G_USER_DIRECTORY_VIDEOS] = get_special_folder (CSIDL_MYVIDEO);
}
#endif /* G_OS_WIN32 */

static void g_init_user_config_dir (void);

#if defined(G_OS_UNIX) && !defined(HAVE_CARBON)

/* adapted from xdg-user-dir-lookup.c
 *
 * Copyright (C) 2007 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
static void
load_user_special_dirs (void)
{
  gchar *config_file;
  gchar *data;
  gchar **lines;
  gint n_lines, i;
  
  g_init_user_config_dir ();
  config_file = g_build_filename (g_user_config_dir,
                                  "user-dirs.dirs",
                                  NULL);
  
  if (!g_file_get_contents (config_file, &data, NULL, NULL))
    {
      g_free (config_file);
      return;
    }

  lines = g_strsplit (data, "\n", -1);
  n_lines = g_strv_length (lines);
  g_free (data);
  
  for (i = 0; i < n_lines; i++)
    {
      gchar *buffer = lines[i];
      gchar *d, *p;
      gint len;
      gboolean is_relative = FALSE;
      GUserDirectory directory;

      /* Remove newline at end */
      len = strlen (buffer);
      if (len > 0 && buffer[len - 1] == '\n')
	buffer[len - 1] = 0;
      
      p = buffer;
      while (*p == ' ' || *p == '\t')
	p++;
      
      if (strncmp (p, "XDG_DESKTOP_DIR", strlen ("XDG_DESKTOP_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DESKTOP;
          p += strlen ("XDG_DESKTOP_DIR");
        }
      else if (strncmp (p, "XDG_DOCUMENTS_DIR", strlen ("XDG_DOCUMENTS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DOCUMENTS;
          p += strlen ("XDG_DOCUMENTS_DIR");
        }
      else if (strncmp (p, "XDG_DOWNLOAD_DIR", strlen ("XDG_DOWNLOAD_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DOWNLOAD;
          p += strlen ("XDG_DOWNLOAD_DIR");
        }
      else if (strncmp (p, "XDG_MUSIC_DIR", strlen ("XDG_MUSIC_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_MUSIC;
          p += strlen ("XDG_MUSIC_DIR");
        }
      else if (strncmp (p, "XDG_PICTURES_DIR", strlen ("XDG_PICTURES_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PICTURES;
          p += strlen ("XDG_PICTURES_DIR");
        }
      else if (strncmp (p, "XDG_PUBLICSHARE_DIR", strlen ("XDG_PUBLICSHARE_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PUBLIC_SHARE;
          p += strlen ("XDG_PUBLICSHARE_DIR");
        }
      else if (strncmp (p, "XDG_TEMPLATES_DIR", strlen ("XDG_TEMPLATES_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_TEMPLATES;
          p += strlen ("XDG_TEMPLATES_DIR");
        }
      else if (strncmp (p, "XDG_VIDEOS_DIR", strlen ("XDG_VIDEOS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_VIDEOS;
          p += strlen ("XDG_VIDEOS_DIR");
        }
      else
	continue;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '=')
	continue;
      p++;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '"')
	continue;
      p++;

      if (strncmp (p, "$HOME", 5) == 0)
	{
	  p += 5;
	  is_relative = TRUE;
	}
      else if (*p != '/')
	continue;

      d = strrchr (p, '"');
      if (!d)
        continue;
      *d = 0;

      d = p;
      
      /* remove trailing slashes */
      len = strlen (d);
      if (d[len - 1] == '/')
        d[len - 1] = 0;
      
      if (is_relative)
        {
          g_get_any_init ();
          g_user_special_dirs[directory] = g_build_filename (g_home_dir, d, NULL);
        }
      else
	g_user_special_dirs[directory] = g_strdup (d);
    }

  g_strfreev (lines);
  g_free (config_file);
}

#endif /* G_OS_UNIX && !HAVE_CARBON */

/**
 * g_get_user_special_dir:
 * @directory: the logical id of special directory
 *
 * Returns the full path of a special directory using its logical id.
 *
 * On Unix this is done using the XDG special user directories.
 * For compatibility with existing practise, %G_USER_DIRECTORY_DESKTOP
 * falls back to <filename>$HOME/Desktop</filename> when XDG special
 * user directories have not been set up. 
 *
 * Depending on the platform, the user might be able to change the path
 * of the special directory without requiring the session to restart; GLib
 * will not reflect any change once the special directories are loaded.
 *
 * Return value: the path to the specified special directory, or %NULL
 *   if the logical id was not found. The returned string is owned by
 *   GLib and should not be modified or freed.
 *
 * Since: 2.14
 */
G_CONST_RETURN gchar *
g_get_user_special_dir (GUserDirectory directory)
{
  g_return_val_if_fail (directory >= G_USER_DIRECTORY_DESKTOP &&
                        directory < G_USER_N_DIRECTORIES, NULL);

  G_LOCK (g_utils_global);

  if (G_UNLIKELY (g_user_special_dirs == NULL))
    {
      g_user_special_dirs = g_new0 (gchar *, G_USER_N_DIRECTORIES);

      load_user_special_dirs ();

      /* Special-case desktop for historical compatibility */
      if (g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] == NULL)
        {
          g_get_any_init ();

          g_user_special_dirs[G_USER_DIRECTORY_DESKTOP] =
            g_build_filename (g_home_dir, "Desktop", NULL);
        }
    }

  G_UNLOCK (g_utils_global);

  return g_user_special_dirs[directory];
}

#ifdef G_OS_WIN32

#undef g_get_system_data_dirs

static HMODULE
get_module_for_address (gconstpointer address)
{
  /* Holds the g_utils_global lock */

  static gboolean beenhere = FALSE;
  typedef BOOL (WINAPI *t_GetModuleHandleExA) (DWORD, LPCTSTR, HMODULE *);
  static t_GetModuleHandleExA p_GetModuleHandleExA = NULL;
  HMODULE hmodule = NULL;

  if (!address)
    return NULL;

  if (!beenhere)
    {
      p_GetModuleHandleExA =
	(t_GetModuleHandleExA) GetProcAddress (LoadLibrary ("kernel32.dll"),
					       "GetModuleHandleExA");
      beenhere = TRUE;
    }

  if (p_GetModuleHandleExA == NULL ||
      !(*p_GetModuleHandleExA) (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
				address, &hmodule))
    {
      MEMORY_BASIC_INFORMATION mbi;
      VirtualQuery (address, &mbi, sizeof (mbi));
      hmodule = (HMODULE) mbi.AllocationBase;
    }

  return hmodule;
}

static gchar *
get_module_share_dir (gconstpointer address)
{
  HMODULE hmodule;
  gchar *filename;
  gchar *retval;

  hmodule = get_module_for_address (address);
  if (hmodule == NULL)
    return NULL;

  filename = g_win32_get_package_installation_directory_of_module (hmodule);
  retval = g_build_filename (filename, "share", NULL);
  g_free (filename);

  return retval;
}

G_CONST_RETURN gchar * G_CONST_RETURN *
g_win32_get_system_data_dirs_for_module (void (*address_of_function)())
{
  GArray *data_dirs;
  HMODULE hmodule;
  static GHashTable *per_module_data_dirs = NULL;
  gchar **retval;
  gchar *p;
  gchar *exe_root;
      
  if (address_of_function)
    {
      G_LOCK (g_utils_global);
      hmodule = get_module_for_address (address_of_function);
      if (hmodule != NULL)
	{
	  if (per_module_data_dirs == NULL)
	    per_module_data_dirs = g_hash_table_new (NULL, NULL);
	  else
	    {
	      retval = g_hash_table_lookup (per_module_data_dirs, hmodule);
	      
	      if (retval != NULL)
		{
		  G_UNLOCK (g_utils_global);
		  return (G_CONST_RETURN gchar * G_CONST_RETURN *) retval;
		}
	    }
	}
    }

  data_dirs = g_array_new (TRUE, TRUE, sizeof (char *));

  /* Documents and Settings\All Users\Application Data */
  p = get_special_folder (CSIDL_COMMON_APPDATA);
  if (p)
    g_array_append_val (data_dirs, p);
  
  /* Documents and Settings\All Users\Documents */
  p = get_special_folder (CSIDL_COMMON_DOCUMENTS);
  if (p)
    g_array_append_val (data_dirs, p);
	
  /* Using the above subfolders of Documents and Settings perhaps
   * makes sense from a Windows perspective.
   *
   * But looking at the actual use cases of this function in GTK+
   * and GNOME software, what we really want is the "share"
   * subdirectory of the installation directory for the package
   * our caller is a part of.
   *
   * The address_of_function parameter, if non-NULL, points to a
   * function in the calling module. Use that to determine that
   * module's installation folder, and use its "share" subfolder.
   *
   * Additionally, also use the "share" subfolder of the installation
   * locations of GLib and the .exe file being run.
   *
   * To guard against none of the above being what is really wanted,
   * callers of this function should have Win32-specific code to look
   * up their installation folder themselves, and handle a subfolder
   * "share" of it in the same way as the folders returned from this
   * function.
   */

  p = get_module_share_dir (address_of_function);
  if (p)
    g_array_append_val (data_dirs, p);
    
  if (glib_dll != NULL)
    {
      gchar *glib_root = g_win32_get_package_installation_directory_of_module (glib_dll);
      p = g_build_filename (glib_root, "share", NULL);
      if (p)
	g_array_append_val (data_dirs, p);
      g_free (glib_root);
    }
  
  exe_root = g_win32_get_package_installation_directory_of_module (NULL);
  p = g_build_filename (exe_root, "share", NULL);
  if (p)
    g_array_append_val (data_dirs, p);
  g_free (exe_root);

  retval = (gchar **) g_array_free (data_dirs, FALSE);

  if (address_of_function)
    {
      if (hmodule != NULL)
	g_hash_table_insert (per_module_data_dirs, hmodule, retval);
      G_UNLOCK (g_utils_global);
    }

  return (G_CONST_RETURN gchar * G_CONST_RETURN *) retval;
}

#endif

/**
 * g_get_system_data_dirs:
 * 
 * Returns an ordered list of base directories in which to access 
 * system-wide application data.
 *
 * On UNIX platforms this is determined using the mechanisms described in
 * the <ulink url="http://www.freedesktop.org/Standards/basedir-spec">
 * XDG Base Directory Specification</ulink>
 * 
 * On Windows the first elements in the list are the Application Data
 * and Documents folders for All Users. (These can be determined only
 * on Windows 2000 or later and are not present in the list on other
 * Windows versions.) See documentation for CSIDL_COMMON_APPDATA and
 * CSIDL_COMMON_DOCUMENTS.
 *
 * Then follows the "share" subfolder in the installation folder for
 * the package containing the DLL that calls this function, if it can
 * be determined.
 * 
 * Finally the list contains the "share" subfolder in the installation
 * folder for GLib, and in the installation folder for the package the
 * application's .exe file belongs to.
 *
 * The installation folders above are determined by looking up the
 * folder where the module (DLL or EXE) in question is located. If the
 * folder's name is "bin", its parent is used, otherwise the folder
 * itself.
 *
 * Note that on Windows the returned list can vary depending on where
 * this function is called.
 *
 * Return value: a %NULL-terminated array of strings owned by GLib that must 
 *               not be modified or freed.
 * Since: 2.6
 **/
G_CONST_RETURN gchar * G_CONST_RETURN * 
g_get_system_data_dirs (void)
{
  gchar **data_dir_vector;

  G_LOCK (g_utils_global);

  if (!g_system_data_dirs)
    {
#ifdef G_OS_WIN32
      data_dir_vector = (gchar **) g_win32_get_system_data_dirs_for_module (NULL);
#else
      gchar *data_dirs = (gchar *) g_getenv ("XDG_DATA_DIRS");

      if (!data_dirs || !data_dirs[0])
          data_dirs = "/usr/local/share/:/usr/share/";

      data_dir_vector = g_strsplit (data_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
#endif

      g_system_data_dirs = data_dir_vector;
    }
  else
    data_dir_vector = g_system_data_dirs;

  G_UNLOCK (g_utils_global);

  return (G_CONST_RETURN gchar * G_CONST_RETURN *) data_dir_vector;
}

/**
 * g_get_system_config_dirs:
 * 
 * Returns an ordered list of base directories in which to access 
 * system-wide configuration information.
 *
 * On UNIX platforms this is determined using the mechanisms described in
 * the <ulink url="http://www.freedesktop.org/Standards/basedir-spec">
 * XDG Base Directory Specification</ulink>
 * 
 * Return value: a %NULL-terminated array of strings owned by GLib that must 
 *               not be modified or freed.
 * Since: 2.6
 **/
G_CONST_RETURN gchar * G_CONST_RETURN *
g_get_system_config_dirs (void)
{
  gchar *conf_dirs, **conf_dir_vector;

  G_LOCK (g_utils_global);

  if (!g_system_config_dirs)
    {
#ifdef G_OS_WIN32
      conf_dirs = get_special_folder (CSIDL_COMMON_APPDATA);
      if (conf_dirs)
	{
	  conf_dir_vector = g_strsplit (conf_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
	  g_free (conf_dirs);
	}
      else
	{
	  /* Return empty list */
	  conf_dir_vector = g_strsplit ("", G_SEARCHPATH_SEPARATOR_S, 0);
	}
#else
      conf_dirs = (gchar *) g_getenv ("XDG_CONFIG_DIRS");

      if (!conf_dirs || !conf_dirs[0])
          conf_dirs = "/etc/xdg";

      conf_dir_vector = g_strsplit (conf_dirs, G_SEARCHPATH_SEPARATOR_S, 0);
#endif

      g_system_config_dirs = conf_dir_vector;
    }
  else
    conf_dir_vector = g_system_config_dirs;
  G_UNLOCK (g_utils_global);

  return (G_CONST_RETURN gchar * G_CONST_RETURN *) conf_dir_vector;
}

#ifndef G_OS_WIN32

static GHashTable *alias_table = NULL;

/* read an alias file for the locales */
static void
read_aliases (gchar *file)
{
  FILE *fp;
  char buf[256];
  
  if (!alias_table)
    alias_table = g_hash_table_new (g_str_hash, g_str_equal);
  fp = fopen (file,"r");
  if (!fp)
    return;
  while (fgets (buf, 256, fp))
    {
      char *p, *q;

      g_strstrip (buf);

      /* Line is a comment */
      if ((buf[0] == '#') || (buf[0] == '\0'))
	continue;

      /* Reads first column */
      for (p = buf, q = NULL; *p; p++) {
	if ((*p == '\t') || (*p == ' ') || (*p == ':')) {
	  *p = '\0';
	  q = p+1;
	  while ((*q == '\t') || (*q == ' ')) {
	    q++;
	  }
	  break;
	}
      }
      /* The line only had one column */
      if (!q || *q == '\0')
	continue;
      
      /* Read second column */
      for (p = q; *p; p++) {
	if ((*p == '\t') || (*p == ' ')) {
	  *p = '\0';
	  break;
	}
      }

      /* Add to alias table if necessary */
      if (!g_hash_table_lookup (alias_table, buf)) {
	g_hash_table_insert (alias_table, g_strdup (buf), g_strdup (q));
      }
    }
  fclose (fp);
}

#endif

static char *
unalias_lang (char *lang)
{
#ifndef G_OS_WIN32
  char *p;
  int i;

  if (!alias_table)
    read_aliases ("/usr/share/locale/locale.alias");

  i = 0;
  while ((p = g_hash_table_lookup (alias_table, lang)) && (strcmp (p, lang) != 0))
    {
      lang = p;
      if (i++ == 30)
        {
          static gboolean said_before = FALSE;
	  if (!said_before)
            g_warning ("Too many alias levels for a locale, "
		       "may indicate a loop");
	  said_before = TRUE;
	  return lang;
	}
    }
#endif
  return lang;
}

/* Mask for components of locale spec. The ordering here is from
 * least significant to most significant
 */
enum
{
  COMPONENT_CODESET =   1 << 0,
  COMPONENT_TERRITORY = 1 << 1,
  COMPONENT_MODIFIER =  1 << 2
};

/* Break an X/Open style locale specification into components
 */
static guint
explode_locale (const gchar *locale,
		gchar      **language, 
		gchar      **territory, 
		gchar      **codeset, 
		gchar      **modifier)
{
  const gchar *uscore_pos;
  const gchar *at_pos;
  const gchar *dot_pos;

  guint mask = 0;

  uscore_pos = strchr (locale, '_');
  dot_pos = strchr (uscore_pos ? uscore_pos : locale, '.');
  at_pos = strchr (dot_pos ? dot_pos : (uscore_pos ? uscore_pos : locale), '@');

  if (at_pos)
    {
      mask |= COMPONENT_MODIFIER;
      *modifier = g_strdup (at_pos);
    }
  else
    at_pos = locale + strlen (locale);

  if (dot_pos)
    {
      mask |= COMPONENT_CODESET;
      *codeset = g_strndup (dot_pos, at_pos - dot_pos);
    }
  else
    dot_pos = at_pos;

  if (uscore_pos)
    {
      mask |= COMPONENT_TERRITORY;
      *territory = g_strndup (uscore_pos, dot_pos - uscore_pos);
    }
  else
    uscore_pos = dot_pos;

  *language = g_strndup (locale, uscore_pos - locale);

  return mask;
}

/*
 * Compute all interesting variants for a given locale name -
 * by stripping off different components of the value.
 *
 * For simplicity, we assume that the locale is in
 * X/Open format: language[_territory][.codeset][@modifier]
 *
 * TODO: Extend this to handle the CEN format (see the GNUlibc docs)
 *       as well. We could just copy the code from glibc wholesale
 *       but it is big, ugly, and complicated, so I'm reluctant
 *       to do so when this should handle 99% of the time...
 */
GSList *
_g_compute_locale_variants (const gchar *locale)
{
  GSList *retval = NULL;

  gchar *language = NULL;
  gchar *territory = NULL;
  gchar *codeset = NULL;
  gchar *modifier = NULL;

  guint mask;
  guint i;

  g_return_val_if_fail (locale != NULL, NULL);

  mask = explode_locale (locale, &language, &territory, &codeset, &modifier);

  /* Iterate through all possible combinations, from least attractive
   * to most attractive.
   */
  for (i = 0; i <= mask; i++)
    if ((i & ~mask) == 0)
      {
	gchar *val = g_strconcat (language,
				  (i & COMPONENT_TERRITORY) ? territory : "",
				  (i & COMPONENT_CODESET) ? codeset : "",
				  (i & COMPONENT_MODIFIER) ? modifier : "",
				  NULL);
	retval = g_slist_prepend (retval, val);
      }

  g_free (language);
  if (mask & COMPONENT_CODESET)
    g_free (codeset);
  if (mask & COMPONENT_TERRITORY)
    g_free (territory);
  if (mask & COMPONENT_MODIFIER)
    g_free (modifier);

  return retval;
}

/* The following is (partly) taken from the gettext package.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

static const gchar *
guess_category_value (const gchar *category_name)
{
  const gchar *retval;

  /* The highest priority value is the `LANGUAGE' environment
     variable.  This is a GNU extension.  */
  retval = g_getenv ("LANGUAGE");
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

  /* `LANGUAGE' is not set.  So we have to proceed with the POSIX
     methods of looking to `LC_ALL', `LC_xxx', and `LANG'.  On some
     systems this can be done by the `setlocale' function itself.  */

  /* Setting of LC_ALL overwrites all other.  */
  retval = g_getenv ("LC_ALL");  
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

  /* Next comes the name of the desired category.  */
  retval = g_getenv (category_name);
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

  /* Last possibility is the LANG environment variable.  */
  retval = g_getenv ("LANG");
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

#ifdef G_PLATFORM_WIN32
  /* g_win32_getlocale() first checks for LC_ALL, LC_MESSAGES and
   * LANG, which we already did above. Oh well. The main point of
   * calling g_win32_getlocale() is to get the thread's locale as used
   * by Windows and the Microsoft C runtime (in the "English_United
   * States" format) translated into the Unixish format.
   */
  retval = g_win32_getlocale ();
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;
#endif  

  return NULL;
}

typedef struct _GLanguageNamesCache GLanguageNamesCache;

struct _GLanguageNamesCache {
  gchar *languages;
  gchar **language_names;
};

static void
language_names_cache_free (gpointer data)
{
  GLanguageNamesCache *cache = data;
  g_free (cache->languages);
  g_strfreev (cache->language_names);
  g_free (cache);
}

/**
 * g_get_language_names:
 * 
 * Computes a list of applicable locale names, which can be used to 
 * e.g. construct locale-dependent filenames or search paths. The returned 
 * list is sorted from most desirable to least desirable and always contains 
 * the default locale "C".
 *
 * For example, if LANGUAGE=de:en_US, then the returned list is
 * "de", "en_US", "en", "C".
 *
 * This function consults the environment variables <envar>LANGUAGE</envar>, 
 * <envar>LC_ALL</envar>, <envar>LC_MESSAGES</envar> and <envar>LANG</envar> 
 * to find the list of locales specified by the user.
 * 
 * Return value: a %NULL-terminated array of strings owned by GLib 
 *    that must not be modified or freed.
 *
 * Since: 2.6
 **/
G_CONST_RETURN gchar * G_CONST_RETURN * 
g_get_language_names (void)
{
  static GStaticPrivate cache_private = G_STATIC_PRIVATE_INIT;
  GLanguageNamesCache *cache = g_static_private_get (&cache_private);
  const gchar *value;

  if (!cache)
    {
      cache = g_new0 (GLanguageNamesCache, 1);
      g_static_private_set (&cache_private, cache, language_names_cache_free);
    }

  value = guess_category_value ("LC_MESSAGES");
  if (!value)
    value = "C";

  if (!(cache->languages && strcmp (cache->languages, value) == 0))
    {
      gchar **languages;
      gchar **alist, **a;
      GSList *list, *l;
      gint i;

      g_free (cache->languages);
      g_strfreev (cache->language_names);
      cache->languages = g_strdup (value);

      alist = g_strsplit (value, ":", 0);
      list = NULL;
      for (a = alist; *a; a++)
	{
	  gchar *b = unalias_lang (*a);
	  list = g_slist_concat (list, _g_compute_locale_variants (b));
	}
      g_strfreev (alist);
      list = g_slist_append (list, g_strdup ("C"));

      cache->language_names = languages = g_new (gchar *, g_slist_length (list) + 1);
      for (l = list, i = 0; l; l = l->next, i++)
	languages[i] = l->data;
      languages[i] = NULL;

      g_slist_free (list);
    }

  return (G_CONST_RETURN gchar * G_CONST_RETURN *) cache->language_names;
}

/**
 * g_direct_hash:
 * @v: a #gpointer key
 *
 * Converts a gpointer to a hash value.
 * It can be passed to g_hash_table_new() as the @hash_func parameter, 
 * when using pointers as keys in a #GHashTable.
 *
 * Returns: a hash value corresponding to the key.
 */
guint
g_direct_hash (gconstpointer v)
{
  return GPOINTER_TO_UINT (v);
}

/**
 * g_direct_equal:
 * @v1: a key.
 * @v2: a key to compare with @v1.
 *
 * Compares two #gpointer arguments and returns %TRUE if they are equal.
 * It can be passed to g_hash_table_new() as the @key_equal_func
 * parameter, when using pointers as keys in a #GHashTable.
 * 
 * Returns: %TRUE if the two keys match.
 */
gboolean
g_direct_equal (gconstpointer v1,
		gconstpointer v2)
{
  return v1 == v2;
}

/**
 * g_int_equal:
 * @v1: a pointer to a #gint key.
 * @v2: a pointer to a #gint key to compare with @v1.
 *
 * Compares the two #gint values being pointed to and returns 
 * %TRUE if they are equal.
 * It can be passed to g_hash_table_new() as the @key_equal_func
 * parameter, when using pointers to integers as keys in a #GHashTable.
 * 
 * Returns: %TRUE if the two keys match.
 */
gboolean
g_int_equal (gconstpointer v1,
	     gconstpointer v2)
{
  return *((const gint*) v1) == *((const gint*) v2);
}

/**
 * g_int_hash:
 * @v: a pointer to a #gint key
 *
 * Converts a pointer to a #gint to a hash value.
 * It can be passed to g_hash_table_new() as the @hash_func parameter, 
 * when using pointers to integers values as keys in a #GHashTable.
 *
 * Returns: a hash value corresponding to the key.
 */
guint
g_int_hash (gconstpointer v)
{
  return *(const gint*) v;
}

/**
 * g_nullify_pointer:
 * @nullify_location: the memory address of the pointer.
 * 
 * Set the pointer at the specified location to %NULL.
 **/
void
g_nullify_pointer (gpointer *nullify_location)
{
  g_return_if_fail (nullify_location != NULL);

  *nullify_location = NULL;
}

/**
 * g_get_codeset:
 * 
 * Get the codeset for the current locale.
 * 
 * Return value: a newly allocated string containing the name
 * of the codeset. This string must be freed with g_free().
 **/
gchar *
g_get_codeset (void)
{
  const gchar *charset;

  g_get_charset (&charset);

  return g_strdup (charset);
}

/* This is called from g_thread_init(). It's used to
 * initialize some static data in a threadsafe way.
 */
void
_g_utils_thread_init (void)
{
  g_get_language_names ();
}

#ifdef G_OS_WIN32

/**
 * _glib_get_locale_dir:
 *
 * Return the path to the share\locale or lib\locale subfolder of the
 * GLib installation folder. The path is in the system codepage. We
 * have to use system codepage as bindtextdomain() doesn't have a
 * UTF-8 interface.
 */
static gchar *
_glib_get_locale_dir (void)
{
  gchar *install_dir = NULL, *locale_dir;
  gchar *retval = NULL;

  if (glib_dll != NULL)
    install_dir = g_win32_get_package_installation_directory_of_module (glib_dll);

  if (install_dir)
    {
      /*
       * Append "/share/locale" or "/lib/locale" depending on whether
       * autoconfigury detected GNU gettext or not.
       */
      const char *p = GLIB_LOCALE_DIR + strlen (GLIB_LOCALE_DIR);
      while (*--p != '/')
	;
      while (*--p != '/')
	;

      locale_dir = g_build_filename (install_dir, p, NULL);

      retval = g_win32_locale_filename_from_utf8 (locale_dir);

      g_free (install_dir);
      g_free (locale_dir);
    }

  if (retval)
    return retval;
  else
    return g_strdup ("");
}

#undef GLIB_LOCALE_DIR

#endif /* G_OS_WIN32 */

/**
 * glib_gettext:
 * @str: The string to be translated
 *
 * Returns the translated string from the glib translations.
 * This is an internal function and should only be used by
 * the internals of glib (such as libgio).
 *
 * Returns: the transation of @str to the current locale
 */
G_CONST_RETURN gchar *
glib_gettext (const gchar *str)
{
  static gboolean _glib_gettext_initialized = FALSE;

  if (!_glib_gettext_initialized)
    {
#ifdef G_OS_WIN32
      gchar *tmp = _glib_get_locale_dir ();
      bindtextdomain (GETTEXT_PACKAGE, tmp);
      g_free (tmp);
#else
      bindtextdomain (GETTEXT_PACKAGE, GLIB_LOCALE_DIR);
#endif
#    ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#    endif
      _glib_gettext_initialized = TRUE;
    }
  
  return g_dgettext (GETTEXT_PACKAGE, str);
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

/* Binary compatibility versions. Not for newly compiled code. */

#undef g_find_program_in_path

gchar*
g_find_program_in_path (const gchar *program)
{
  gchar *utf8_program = g_locale_to_utf8 (program, -1, NULL, NULL, NULL);
  gchar *utf8_retval = g_find_program_in_path_utf8 (utf8_program);
  gchar *retval;

  g_free (utf8_program);
  if (utf8_retval == NULL)
    return NULL;
  retval = g_locale_from_utf8 (utf8_retval, -1, NULL, NULL, NULL);
  g_free (utf8_retval);

  return retval;
}

#undef g_get_current_dir

gchar*
g_get_current_dir (void)
{
  gchar *utf8_dir = g_get_current_dir_utf8 ();
  gchar *dir = g_locale_from_utf8 (utf8_dir, -1, NULL, NULL, NULL);
  g_free (utf8_dir);
  return dir;
}

#undef g_getenv

G_CONST_RETURN gchar*
g_getenv (const gchar *variable)
{
  gchar *utf8_variable = g_locale_to_utf8 (variable, -1, NULL, NULL, NULL);
  const gchar *utf8_value = g_getenv_utf8 (utf8_variable);
  gchar *value;
  GQuark quark;

  g_free (utf8_variable);
  if (!utf8_value)
    return NULL;
  value = g_locale_from_utf8 (utf8_value, -1, NULL, NULL, NULL);
  quark = g_quark_from_string (value);
  g_free (value);

  return g_quark_to_string (quark);
}

#undef g_setenv

gboolean
g_setenv (const gchar *variable, 
	  const gchar *value, 
	  gboolean     overwrite)
{
  gchar *utf8_variable = g_locale_to_utf8 (variable, -1, NULL, NULL, NULL);
  gchar *utf8_value = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
  gboolean retval = g_setenv_utf8 (utf8_variable, utf8_value, overwrite);

  g_free (utf8_variable);
  g_free (utf8_value);

  return retval;
}

#undef g_unsetenv

void
g_unsetenv (const gchar *variable)
{
  gchar *utf8_variable = g_locale_to_utf8 (variable, -1, NULL, NULL, NULL);

  g_unsetenv_utf8 (utf8_variable);

  g_free (utf8_variable);
}

#undef g_get_user_name

G_CONST_RETURN gchar*
g_get_user_name (void)
{
  g_get_any_init_locked ();
  return g_user_name_cp;
}

#undef g_get_real_name

G_CONST_RETURN gchar*
g_get_real_name (void)
{
  g_get_any_init_locked ();
  return g_real_name_cp;
}

#undef g_get_home_dir

G_CONST_RETURN gchar*
g_get_home_dir (void)
{
  g_get_any_init_locked ();
  return g_home_dir_cp;
}

#undef g_get_tmp_dir

G_CONST_RETURN gchar*
g_get_tmp_dir (void)
{
  g_get_any_init_locked ();
  return g_tmp_dir_cp;
}

#endif

#define __G_UTILS_C__
#include "galiasdef.c"
