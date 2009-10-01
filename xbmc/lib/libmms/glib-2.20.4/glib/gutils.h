/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_UTILS_H__
#define __G_UTILS_H__

#include <glib/gtypes.h>
#include <stdarg.h>

G_BEGIN_DECLS

#ifdef G_OS_WIN32

/* On Win32, the canonical directory separator is the backslash, and
 * the search path separator is the semicolon. Note that also the
 * (forward) slash works as directory separator.
 */
#define G_DIR_SEPARATOR '\\'
#define G_DIR_SEPARATOR_S "\\"
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR || (c) == '/')
#define G_SEARCHPATH_SEPARATOR ';'
#define G_SEARCHPATH_SEPARATOR_S ";"

#else  /* !G_OS_WIN32 */

/* Unix */

#define G_DIR_SEPARATOR '/'
#define G_DIR_SEPARATOR_S "/"
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR)
#define G_SEARCHPATH_SEPARATOR ':'
#define G_SEARCHPATH_SEPARATOR_S ":"

#endif /* !G_OS_WIN32 */

/* Define G_VA_COPY() to do the right thing for copying va_list variables.
 * glibconfig.h may have already defined G_VA_COPY as va_copy or __va_copy.
 */
#if !defined (G_VA_COPY)
#  if defined (__GNUC__) && defined (__PPC__) && (defined (_CALL_SYSV) || defined (_WIN32))
#    define G_VA_COPY(ap1, ap2)	  (*(ap1) = *(ap2))
#  elif defined (G_VA_COPY_AS_ARRAY)
#    define G_VA_COPY(ap1, ap2)	  g_memmove ((ap1), (ap2), sizeof (va_list))
#  else /* va_list is a pointer */
#    define G_VA_COPY(ap1, ap2)	  ((ap1) = (ap2))
#  endif /* va_list is a pointer */
#endif /* !G_VA_COPY */

/* inlining hassle. for compilers that don't allow the `inline' keyword,
 * mostly because of strict ANSI C compliance or dumbness, we try to fall
 * back to either `__inline__' or `__inline'.
 * G_CAN_INLINE is defined in glibconfig.h if the compiler seems to be 
 * actually *capable* to do function inlining, in which case inline 
 * function bodies do make sense. we also define G_INLINE_FUNC to properly 
 * export the function prototypes if no inlining can be performed.
 * inline function bodies have to be special cased with G_CAN_INLINE and a
 * .c file specific macro to allow one compiled instance with extern linkage
 * of the functions by defining G_IMPLEMENT_INLINES and the .c file macro.
 */
#if defined (G_HAVE_INLINE) && defined (__GNUC__) && defined (__STRICT_ANSI__)
#  undef inline
#  define inline __inline__
#elif !defined (G_HAVE_INLINE)
#  undef inline
#  if defined (G_HAVE___INLINE__)
#    define inline __inline__
#  elif defined (G_HAVE___INLINE)
#    define inline __inline
#  else /* !inline && !__inline__ && !__inline */
#    define inline  /* don't inline, then */
#  endif
#endif
#ifdef G_IMPLEMENT_INLINES
#  define G_INLINE_FUNC
#  undef  G_CAN_INLINE
#elif defined (__GNUC__) 
#  define G_INLINE_FUNC static __inline __attribute__ ((unused))
#elif defined (G_CAN_INLINE) 
#  define G_INLINE_FUNC static inline
#else /* can't inline */
#  define G_INLINE_FUNC
#endif /* !G_INLINE_FUNC */

/* Retrive static string info
 */
#ifdef G_OS_WIN32
#define g_get_user_name g_get_user_name_utf8
#define g_get_real_name g_get_real_name_utf8
#define g_get_home_dir g_get_home_dir_utf8
#define g_get_tmp_dir g_get_tmp_dir_utf8
#endif

G_CONST_RETURN gchar* g_get_user_name        (void);
G_CONST_RETURN gchar* g_get_real_name        (void);
G_CONST_RETURN gchar* g_get_home_dir         (void);
G_CONST_RETURN gchar* g_get_tmp_dir          (void);
G_CONST_RETURN gchar* g_get_host_name	     (void);
gchar*                g_get_prgname          (void);
void                  g_set_prgname          (const gchar *prgname);
G_CONST_RETURN gchar* g_get_application_name (void);
void                  g_set_application_name (const gchar *application_name);

G_CONST_RETURN gchar*    g_get_user_data_dir      (void);
G_CONST_RETURN gchar*    g_get_user_config_dir    (void);
G_CONST_RETURN gchar*    g_get_user_cache_dir     (void);
G_CONST_RETURN gchar* G_CONST_RETURN * g_get_system_data_dirs   (void);

#ifdef G_OS_WIN32
/* This functions is not part of the public GLib API */
G_CONST_RETURN gchar* G_CONST_RETURN * g_win32_get_system_data_dirs_for_module (void (*address_of_function)());
#endif

#if defined (G_OS_WIN32) && defined (G_CAN_INLINE) && !defined (__cplusplus)
/* This function is not part of the public GLib API either. Just call
 * g_get_system_data_dirs() in your code, never mind that that is
 * actually a macro and you will in fact call this inline function.
 */
static inline G_CONST_RETURN gchar * G_CONST_RETURN *
_g_win32_get_system_data_dirs (void)
{
  return g_win32_get_system_data_dirs_for_module ((void (*)()) &_g_win32_get_system_data_dirs);
}
#define g_get_system_data_dirs _g_win32_get_system_data_dirs
#endif

G_CONST_RETURN gchar* G_CONST_RETURN * g_get_system_config_dirs (void);

G_CONST_RETURN gchar* G_CONST_RETURN * g_get_language_names (void);

/**
 * GUserDirectory:
 * @G_USER_DIRECTORY_DESKTOP: the user's Desktop directory
 * @G_USER_DIRECTORY_DOCUMENTS: the user's Documents directory
 * @G_USER_DIRECTORY_DOWNLOAD: the user's Downloads directory
 * @G_USER_DIRECTORY_MUSIC: the user's Music directory
 * @G_USER_DIRECTORY_PICTURES: the user's Pictures directory
 * @G_USER_DIRECTORY_PUBLIC_SHARE: the user's shared directory
 * @G_USER_DIRECTORY_TEMPLATES: the user's Templates directory
 * @G_USER_DIRECTORY_VIDEOS: the user's Movies directory
 * @G_USER_N_DIRECTORIES: the number of enum values
 *
 * These are logical ids for special directories which are defined
 * depending on the platform used. You should use g_get_user_special_dir()
 * to retrieve the full path associated to the logical id.
 *
 * The #GUserDirectory enumeration can be extended at later date. Not
 * every platform has a directory for every logical id in this
 * enumeration.
 *
 * Since: 2.14
 */
typedef enum {
  G_USER_DIRECTORY_DESKTOP,
  G_USER_DIRECTORY_DOCUMENTS,
  G_USER_DIRECTORY_DOWNLOAD,
  G_USER_DIRECTORY_MUSIC,
  G_USER_DIRECTORY_PICTURES,
  G_USER_DIRECTORY_PUBLIC_SHARE,
  G_USER_DIRECTORY_TEMPLATES,
  G_USER_DIRECTORY_VIDEOS,

  G_USER_N_DIRECTORIES
} GUserDirectory;

G_CONST_RETURN gchar* g_get_user_special_dir (GUserDirectory directory);

typedef struct _GDebugKey	GDebugKey;
struct _GDebugKey
{
  const gchar *key;
  guint	       value;
};

/* Miscellaneous utility functions
 */
guint                 g_parse_debug_string (const gchar     *string,
					    const GDebugKey *keys,
					    guint            nkeys);

gint                  g_snprintf           (gchar       *string,
					    gulong       n,
					    gchar const *format,
					    ...) G_GNUC_PRINTF (3, 4);
gint                  g_vsnprintf          (gchar       *string,
					    gulong       n,
					    gchar const *format,
					    va_list      args);

/* Check if a file name is an absolute path */
gboolean              g_path_is_absolute   (const gchar *file_name);

/* In case of absolute paths, skip the root part */
G_CONST_RETURN gchar* g_path_skip_root     (const gchar *file_name);

#ifndef G_DISABLE_DEPRECATED

/* These two functions are deprecated and will be removed in the next
 * major release of GLib. Use g_path_get_dirname/g_path_get_basename
 * instead. Whatch out! The string returned by g_path_get_basename
 * must be g_freed, while the string returned by g_basename must not.*/
G_CONST_RETURN gchar* g_basename           (const gchar *file_name);
#define g_dirname g_path_get_dirname

#endif /* G_DISABLE_DEPRECATED */

#ifdef G_OS_WIN32
#define g_get_current_dir g_get_current_dir_utf8
#endif

/* The returned strings are newly allocated with g_malloc() */
gchar*                g_get_current_dir    (void);
gchar*                g_path_get_basename  (const gchar *file_name) G_GNUC_MALLOC;
gchar*                g_path_get_dirname   (const gchar *file_name) G_GNUC_MALLOC;

/* Set the pointer at the specified location to NULL */
void                  g_nullify_pointer    (gpointer    *nullify_location);

/* return the environment string for the variable. The returned memory
 * must not be freed. */
#ifdef G_OS_WIN32
#define g_getenv g_getenv_utf8
#define g_setenv g_setenv_utf8
#define g_unsetenv g_unsetenv_utf8
#define g_find_program_in_path g_find_program_in_path_utf8
#endif

G_CONST_RETURN gchar* g_getenv             (const gchar *variable);
gboolean              g_setenv             (const gchar *variable,
					    const gchar *value,
					    gboolean     overwrite);
void                  g_unsetenv           (const gchar *variable);
gchar**               g_listenv            (void);

/* private */
const gchar*	     _g_getenv_nomalloc	   (const gchar	*variable,
					    gchar        buffer[1024]);

/* we try to provide a useful equivalent for ATEXIT if it is
 * not defined, but use is actually abandoned. people should
 * use g_atexit() instead.
 */
typedef	void		(*GVoidFunc)		(void);
#ifndef ATEXIT
# define ATEXIT(proc)	g_ATEXIT(proc)
#else
# define G_NATIVE_ATEXIT
#endif /* ATEXIT */
/* we use a GLib function as a replacement for ATEXIT, so
 * the programmer is not required to check the return value
 * (if there is any in the implementation) and doesn't encounter
 * missing include files.
 */
void	g_atexit		(GVoidFunc    func);

#ifdef G_OS_WIN32
/* It's a bad idea to wrap atexit() on Windows. If the GLib DLL calls
 * atexit(), the function will be called when the GLib DLL is detached
 * from the program, which is not what the caller wants. The caller
 * wants the function to be called when it *itself* exits (or is
 * detached, in case the caller, too, is a DLL).
 */
int atexit (void (*)(void));
#define g_atexit(func) atexit(func)
#endif

/* Look for an executable in PATH, following execvp() rules */
gchar*  g_find_program_in_path  (const gchar *program);

/* Bit tests
 */
G_INLINE_FUNC gint	g_bit_nth_lsf (gulong  mask,
				       gint    nth_bit) G_GNUC_CONST;
G_INLINE_FUNC gint	g_bit_nth_msf (gulong  mask,
				       gint    nth_bit) G_GNUC_CONST;
G_INLINE_FUNC guint	g_bit_storage (gulong  number) G_GNUC_CONST;

/* Trash Stacks
 * elements need to be >= sizeof (gpointer)
 */
typedef struct _GTrashStack     GTrashStack;
struct _GTrashStack
{
  GTrashStack *next;
};

G_INLINE_FUNC void	g_trash_stack_push	(GTrashStack **stack_p,
						 gpointer      data_p);
G_INLINE_FUNC gpointer	g_trash_stack_pop	(GTrashStack **stack_p);
G_INLINE_FUNC gpointer	g_trash_stack_peek	(GTrashStack **stack_p);
G_INLINE_FUNC guint	g_trash_stack_height	(GTrashStack **stack_p);

/* inline function implementations
 */
#if defined (G_CAN_INLINE) || defined (__G_UTILS_C__)
G_INLINE_FUNC gint
g_bit_nth_lsf (gulong mask,
	       gint   nth_bit)
{
  if (G_UNLIKELY (nth_bit < -1))
    nth_bit = -1;
  while (nth_bit < ((GLIB_SIZEOF_LONG * 8) - 1))
    {
      nth_bit++;
      if (mask & (1UL << nth_bit))
	return nth_bit;
    }
  return -1;
}
G_INLINE_FUNC gint
g_bit_nth_msf (gulong mask,
	       gint   nth_bit)
{
  if (nth_bit < 0 || G_UNLIKELY (nth_bit > GLIB_SIZEOF_LONG * 8))
    nth_bit = GLIB_SIZEOF_LONG * 8;
  while (nth_bit > 0)
    {
      nth_bit--;
      if (mask & (1UL << nth_bit))
	return nth_bit;
    }
  return -1;
}
G_INLINE_FUNC guint
g_bit_storage (gulong number)
{
#if defined(__GNUC__) && (__GNUC__ >= 4) && defined(__OPTIMIZE__)
  return G_LIKELY (number) ?
	   ((GLIB_SIZEOF_LONG * 8 - 1) ^ __builtin_clzl(number)) + 1 : 1;
#else
  register guint n_bits = 0;
  
  do
    {
      n_bits++;
      number >>= 1;
    }
  while (number);
  return n_bits;
#endif
}
G_INLINE_FUNC void
g_trash_stack_push (GTrashStack **stack_p,
		    gpointer      data_p)
{
  GTrashStack *data = (GTrashStack *) data_p;

  data->next = *stack_p;
  *stack_p = data;
}
G_INLINE_FUNC gpointer
g_trash_stack_pop (GTrashStack **stack_p)
{
  GTrashStack *data;

  data = *stack_p;
  if (data)
    {
      *stack_p = data->next;
      /* NULLify private pointer here, most platforms store NULL as
       * subsequent 0 bytes
       */
      data->next = NULL;
    }

  return data;
}
G_INLINE_FUNC gpointer
g_trash_stack_peek (GTrashStack **stack_p)
{
  GTrashStack *data;

  data = *stack_p;

  return data;
}
G_INLINE_FUNC guint
g_trash_stack_height (GTrashStack **stack_p)
{
  GTrashStack *data;
  guint i = 0;

  for (data = *stack_p; data; data = data->next)
    i++;

  return i;
}
#endif  /* G_CAN_INLINE || __G_UTILS_C__ */

/* Glib version.
 * we prefix variable declarations so they can
 * properly get exported in windows dlls.
 */
GLIB_VAR const guint glib_major_version;
GLIB_VAR const guint glib_minor_version;
GLIB_VAR const guint glib_micro_version;
GLIB_VAR const guint glib_interface_age;
GLIB_VAR const guint glib_binary_age;

const gchar * glib_check_version (guint required_major,
                                  guint required_minor,
                                  guint required_micro);

#define GLIB_CHECK_VERSION(major,minor,micro)    \
    (GLIB_MAJOR_VERSION > (major) || \
     (GLIB_MAJOR_VERSION == (major) && GLIB_MINOR_VERSION > (minor)) || \
     (GLIB_MAJOR_VERSION == (major) && GLIB_MINOR_VERSION == (minor) && \
      GLIB_MICRO_VERSION >= (micro)))

G_END_DECLS

#ifndef G_DISABLE_DEPRECATED

/*
 * This macro is deprecated. This DllMain() is too complex. It is
 * recommended to write an explicit minimal DLlMain() that just saves
 * the handle to the DLL and then use that handle instead, for
 * instance passing it to
 * g_win32_get_package_installation_directory_of_module().
 *
 * On Windows, this macro defines a DllMain function that stores the
 * actual DLL name that the code being compiled will be included in.
 * STATIC should be empty or 'static'. DLL_NAME is the name of the
 * (pointer to the) char array where the DLL name will be stored. If
 * this is used, you must also include <windows.h>. If you need a more complex
 * DLL entry point function, you cannot use this.
 *
 * On non-Windows platforms, expands to nothing.
 */

#ifndef G_PLATFORM_WIN32
# define G_WIN32_DLLMAIN_FOR_DLL_NAME(static, dll_name)
#else
# define G_WIN32_DLLMAIN_FOR_DLL_NAME(static, dll_name)			\
static char *dll_name;							\
									\
BOOL WINAPI								\
DllMain (HINSTANCE hinstDLL,						\
	 DWORD     fdwReason,						\
	 LPVOID    lpvReserved)						\
{									\
  wchar_t wcbfr[1000];							\
  char *tem;								\
  switch (fdwReason)							\
    {									\
    case DLL_PROCESS_ATTACH:						\
      GetModuleFileNameW ((HMODULE) hinstDLL, wcbfr, G_N_ELEMENTS (wcbfr)); \
      tem = g_utf16_to_utf8 (wcbfr, -1, NULL, NULL, NULL);		\
      dll_name = g_path_get_basename (tem);				\
      g_free (tem);							\
      break;								\
    }									\
									\
  return TRUE;								\
}

#endif	/* !G_DISABLE_DEPRECATED */

#endif /* G_PLATFORM_WIN32 */

#endif /* __G_UTILS_H__ */
