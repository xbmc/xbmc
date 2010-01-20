/* A GNU-like <stdio.h>.

   Copyright (C) 2004, 2007-2009 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if defined __need_FILE || defined __need___FILE
/* Special invocation convention inside glibc header files.  */

#@INCLUDE_NEXT@ @NEXT_STDIO_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_STDIO_H

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_STDIO_H@

#ifndef _GL_STDIO_H
#define _GL_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#if (@GNULIB_FSEEKO@ && @REPLACE_FSEEKO@) \
  || (@GNULIB_FTELLO@ && @REPLACE_FTELLO@) \
  || (@GNULIB_GETDELIM@ && !@HAVE_DECL_GETDELIM@) \
  || (@GNULIB_GETLINE@ && (!@HAVE_DECL_GETLINE@ || @REPLACE_GETLINE@))
/* Get off_t and ssize_t.  */
# include <sys/types.h>
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#  define __attribute__(Spec) /* empty */
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif


/* The definition of GL_LINK_WARNING is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif


#if @GNULIB_FPRINTF_POSIX@
# if @REPLACE_FPRINTF@
#  define fprintf rpl_fprintf
extern int fprintf (FILE *fp, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
# endif
#elif @GNULIB_FPRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# define fprintf rpl_fprintf
extern int fprintf (FILE *fp, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
#elif defined GNULIB_POSIXCHECK
# undef fprintf
# define fprintf \
    (GL_LINK_WARNING ("fprintf is not always POSIX compliant - " \
                      "use gnulib module fprintf-posix for portable " \
                      "POSIX compliance"), \
     fprintf)
#endif

#if @GNULIB_VFPRINTF_POSIX@
# if @REPLACE_VFPRINTF@
#  define vfprintf rpl_vfprintf
extern int vfprintf (FILE *fp, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#elif @GNULIB_VFPRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# define vfprintf rpl_vfprintf
extern int vfprintf (FILE *fp, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)));
#elif defined GNULIB_POSIXCHECK
# undef vfprintf
# define vfprintf(s,f,a) \
    (GL_LINK_WARNING ("vfprintf is not always POSIX compliant - " \
                      "use gnulib module vfprintf-posix for portable " \
                      "POSIX compliance"), \
     vfprintf (s, f, a))
#endif

#if @GNULIB_PRINTF_POSIX@
# if @REPLACE_PRINTF@
/* Don't break __attribute__((format(printf,M,N))).  */
#  define printf __printf__
extern int printf (const char *format, ...)
       __attribute__ ((__format__ (__printf__, 1, 2)));
# endif
#elif @GNULIB_PRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
/* Don't break __attribute__((format(printf,M,N))).  */
# define printf __printf__
extern int printf (const char *format, ...)
       __attribute__ ((__format__ (__printf__, 1, 2)));
#elif defined GNULIB_POSIXCHECK
# undef printf
# define printf \
    (GL_LINK_WARNING ("printf is not always POSIX compliant - " \
                      "use gnulib module printf-posix for portable " \
                      "POSIX compliance"), \
     printf)
/* Don't break __attribute__((format(printf,M,N))).  */
# define format(kind,m,n) format (__##kind##__, m, n)
# define __format__(kind,m,n) __format__ (__##kind##__, m, n)
# define ____printf____ __printf__
# define ____scanf____ __scanf__
# define ____strftime____ __strftime__
# define ____strfmon____ __strfmon__
#endif

#if @GNULIB_VPRINTF_POSIX@
# if @REPLACE_VPRINTF@
#  define vprintf rpl_vprintf
extern int vprintf (const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 1, 0)));
# endif
#elif @GNULIB_VPRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# define vprintf rpl_vprintf
extern int vprintf (const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 1, 0)));
#elif defined GNULIB_POSIXCHECK
# undef vprintf
# define vprintf(f,a) \
    (GL_LINK_WARNING ("vprintf is not always POSIX compliant - " \
                      "use gnulib module vprintf-posix for portable " \
                      "POSIX compliance"), \
     vprintf (f, a))
#endif

#if @GNULIB_SNPRINTF@
# if @REPLACE_SNPRINTF@
#  define snprintf rpl_snprintf
# endif
# if @REPLACE_SNPRINTF@ || !@HAVE_DECL_SNPRINTF@
extern int snprintf (char *str, size_t size, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 3, 4)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef snprintf
# define snprintf \
    (GL_LINK_WARNING ("snprintf is unportable - " \
                      "use gnulib module snprintf for portability"), \
     snprintf)
#endif

#if @GNULIB_VSNPRINTF@
# if @REPLACE_VSNPRINTF@
#  define vsnprintf rpl_vsnprintf
# endif
# if @REPLACE_VSNPRINTF@ || !@HAVE_DECL_VSNPRINTF@
extern int vsnprintf (char *str, size_t size, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 3, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vsnprintf
# define vsnprintf(b,s,f,a) \
    (GL_LINK_WARNING ("vsnprintf is unportable - " \
                      "use gnulib module vsnprintf for portability"), \
     vsnprintf (b, s, f, a))
#endif

#if @GNULIB_SPRINTF_POSIX@
# if @REPLACE_SPRINTF@
#  define sprintf rpl_sprintf
extern int sprintf (char *str, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef sprintf
# define sprintf \
    (GL_LINK_WARNING ("sprintf is not always POSIX compliant - " \
                      "use gnulib module sprintf-posix for portable " \
                      "POSIX compliance"), \
     sprintf)
#endif

#if @GNULIB_VSPRINTF_POSIX@
# if @REPLACE_VSPRINTF@
#  define vsprintf rpl_vsprintf
extern int vsprintf (char *str, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vsprintf
# define vsprintf(b,f,a) \
    (GL_LINK_WARNING ("vsprintf is not always POSIX compliant - " \
                      "use gnulib module vsprintf-posix for portable " \
                      "POSIX compliance"), \
     vsprintf (b, f, a))
#endif

#if @GNULIB_DPRINTF@
# if @REPLACE_DPRINTF@
#  define dprintf rpl_dprintf
# endif
# if @REPLACE_DPRINTF@ || !@HAVE_DPRINTF@
extern int dprintf (int fd, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef dprintf
# define dprintf(d,f,a) \
    (GL_LINK_WARNING ("dprintf is unportable - " \
                      "use gnulib module dprintf for portability"), \
     dprintf (d, f, a))
#endif

#if @GNULIB_VDPRINTF@
# if @REPLACE_VDPRINTF@
#  define vdprintf rpl_vdprintf
# endif
# if @REPLACE_VDPRINTF@ || !@HAVE_VDPRINTF@
extern int vdprintf (int fd, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vdprintf
# define vdprintf(d,f,a) \
    (GL_LINK_WARNING ("vdprintf is unportable - " \
                      "use gnulib module vdprintf for portability"), \
     vdprintf (d, f, a))
#endif

#if @GNULIB_VASPRINTF@
# if @REPLACE_VASPRINTF@
#  define asprintf rpl_asprintf
#  define vasprintf rpl_vasprintf
# endif
# if @REPLACE_VASPRINTF@ || !@HAVE_VASPRINTF@
  /* Write formatted output to a string dynamically allocated with malloc().
     If the memory allocation succeeds, store the address of the string in
     *RESULT and return the number of resulting bytes, excluding the trailing
     NUL.  Upon memory allocation error, or some other error, return -1.  */
  extern int asprintf (char **result, const char *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
  extern int vasprintf (char **result, const char *format, va_list args)
    __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#endif

#if @GNULIB_OBSTACK_PRINTF@
# if @REPLACE_OBSTACK_PRINTF@
#  define obstack_printf rpl_osbtack_printf
#  define obstack_vprintf rpl_obstack_vprintf
# endif
# if @REPLACE_OBSTACK_PRINTF@ || !@HAVE_DECL_OBSTACK_PRINTF@
  struct obstack;
  /* Grow an obstack with formatted output.  Return the number of
     bytes added to OBS.  No trailing nul byte is added, and the
     object should be closed with obstack_finish before use.  Upon
     memory allocation error, call obstack_alloc_failed_handler.  Upon
     other error, return -1.  */
  extern int obstack_printf (struct obstack *obs, const char *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
  extern int obstack_vprintf (struct obstack *obs, const char *format,
			      va_list args)
    __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#endif

#if @GNULIB_FOPEN@
# if @REPLACE_FOPEN@
#  undef fopen
#  define fopen rpl_fopen
extern FILE * fopen (const char *filename, const char *mode);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fopen
# define fopen(f,m) \
   (GL_LINK_WARNING ("fopen on Win32 platforms is not POSIX compatible - " \
                     "use gnulib module fopen for portability"), \
    fopen (f, m))
#endif

#if @GNULIB_FREOPEN@
# if @REPLACE_FREOPEN@
#  undef freopen
#  define freopen rpl_freopen
extern FILE * freopen (const char *filename, const char *mode, FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef freopen
# define freopen(f,m,s) \
   (GL_LINK_WARNING ("freopen on Win32 platforms is not POSIX compatible - " \
                     "use gnulib module freopen for portability"), \
    freopen (f, m, s))
#endif

#if @GNULIB_FSEEKO@
# if @REPLACE_FSEEKO@
/* Provide fseek, fseeko functions that are aware of a preceding
   fflush(), and which detect pipes.  */
#  define fseeko rpl_fseeko
extern int fseeko (FILE *fp, off_t offset, int whence);
#  define fseek(fp, offset, whence) fseeko (fp, (off_t)(offset), whence)
# endif
#elif defined GNULIB_POSIXCHECK
# undef fseeko
# define fseeko(f,o,w) \
   (GL_LINK_WARNING ("fseeko is unportable - " \
                     "use gnulib module fseeko for portability"), \
    fseeko (f, o, w))
#endif

#if @GNULIB_FSEEK@ && @REPLACE_FSEEK@
extern int rpl_fseek (FILE *fp, long offset, int whence);
# undef fseek
# if defined GNULIB_POSIXCHECK
#  define fseek(f,o,w) \
     (GL_LINK_WARNING ("fseek cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use fseeko function for handling of large files"), \
      rpl_fseek (f, o, w))
# else
#  define fseek rpl_fseek
# endif
#elif defined GNULIB_POSIXCHECK
# ifndef fseek
#  define fseek(f,o,w) \
     (GL_LINK_WARNING ("fseek cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use fseeko function for handling of large files"), \
      fseek (f, o, w))
# endif
#endif

#if @GNULIB_FTELLO@
# if @REPLACE_FTELLO@
#  define ftello rpl_ftello
extern off_t ftello (FILE *fp);
#  define ftell(fp) ftello (fp)
# endif
#elif defined GNULIB_POSIXCHECK
# undef ftello
# define ftello(f) \
   (GL_LINK_WARNING ("ftello is unportable - " \
                     "use gnulib module ftello for portability"), \
    ftello (f))
#endif

#if @GNULIB_FTELL@ && @REPLACE_FTELL@
extern long rpl_ftell (FILE *fp);
# undef ftell
# if GNULIB_POSIXCHECK
#  define ftell(f) \
     (GL_LINK_WARNING ("ftell cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use ftello function for handling of large files"), \
      rpl_ftell (f))
# else
#  define ftell rpl_ftell
# endif
#elif defined GNULIB_POSIXCHECK
# ifndef ftell
#  define ftell(f) \
     (GL_LINK_WARNING ("ftell cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use ftello function for handling of large files"), \
      ftell (f))
# endif
#endif

#if @GNULIB_FFLUSH@
# if @REPLACE_FFLUSH@
#  define fflush rpl_fflush
  /* Flush all pending data on STREAM according to POSIX rules.  Both
     output and seekable input streams are supported.
     Note! LOSS OF DATA can occur if fflush is applied on an input stream
     that is _not_seekable_ or on an update stream that is _not_seekable_
     and in which the most recent operation was input.  Seekability can
     be tested with lseek(fileno(fp),0,SEEK_CUR).  */
  extern int fflush (FILE *gl_stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fflush
# define fflush(f) \
   (GL_LINK_WARNING ("fflush is not always POSIX compliant - " \
                     "use gnulib module fflush for portable " \
                     "POSIX compliance"), \
    fflush (f))
#endif

#if @GNULIB_FPURGE@
# if @REPLACE_FPURGE@
#  define fpurge rpl_fpurge
# endif
# if @REPLACE_FPURGE@ || !@HAVE_DECL_FPURGE@
  /* Discard all pending buffered I/O data on STREAM.
     STREAM must not be wide-character oriented.
     Return 0 if successful.  Upon error, return -1 and set errno.  */
  extern int fpurge (FILE *gl_stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fpurge
# define fpurge(f) \
   (GL_LINK_WARNING ("fpurge is not always present - " \
                     "use gnulib module fpurge for portability"), \
    fpurge (f))
#endif

#if @GNULIB_FCLOSE@
# if @REPLACE_FCLOSE@
#  define fclose rpl_fclose
  /* Close STREAM and its underlying file descriptor.  */
extern int fclose (FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fclose
# define fclose(f) \
   (GL_LINK_WARNING ("fclose is not always POSIX compliant - " \
                     "use gnulib module fclose for portable " \
                     "POSIX compliance"), \
    fclose (f))
#endif

#if @GNULIB_FPUTC@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef fputc
# define fputc rpl_fputc
extern int fputc (int c, FILE *stream);
#endif

#if @GNULIB_PUTC@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef putc
# define putc rpl_fputc
extern int putc (int c, FILE *stream);
#endif

#if @GNULIB_PUTCHAR@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef putchar
# define putchar rpl_putchar
extern int putchar (int c);
#endif

#if @GNULIB_FPUTS@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef fputs
# define fputs rpl_fputs
extern int fputs (const char *string, FILE *stream);
#endif

#if @GNULIB_PUTS@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef puts
# define puts rpl_puts
extern int puts (const char *string);
#endif

#if @GNULIB_FWRITE@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef fwrite
# define fwrite rpl_fwrite
extern size_t fwrite (const void *ptr, size_t s, size_t n, FILE *stream);
#endif

#if @GNULIB_GETDELIM@
# if !@HAVE_DECL_GETDELIM@
/* Read input, up to (and including) the next occurrence of DELIMITER, from
   STREAM, store it in *LINEPTR (and NUL-terminate it).
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getdelim (char **lineptr, size_t *linesize, int delimiter,
			 FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef getdelim
# define getdelim(l, s, d, f)					    \
  (GL_LINK_WARNING ("getdelim is unportable - "			    \
		    "use gnulib module getdelim for portability"),  \
   getdelim (l, s, d, f))
#endif

#if @GNULIB_GETLINE@
# if @REPLACE_GETLINE@
#  undef getline
#  define getline rpl_getline
# endif
# if !@HAVE_DECL_GETLINE@ || @REPLACE_GETLINE@
/* Read a line, up to (and including) the next newline, from STREAM, store it
   in *LINEPTR (and NUL-terminate it).
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getline (char **lineptr, size_t *linesize, FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef getline
# define getline(l, s, f)						\
  (GL_LINK_WARNING ("getline is unportable - "				\
		    "use gnulib module getline for portability"),	\
   getline (l, s, f))
#endif

#if @GNULIB_PERROR@
# if @REPLACE_PERROR@
#  define perror rpl_perror
/* Print a message to standard error, describing the value of ERRNO,
   (if STRING is not NULL and not empty) prefixed with STRING and ": ",
   and terminated with a newline.  */
extern void perror (const char *string);
# endif
#elif defined GNULIB_POSIXCHECK
# undef perror
# define perror(s) \
    (GL_LINK_WARNING ("perror is not always POSIX compliant - " \
                      "use gnulib module perror for portability"), \
     perror (s))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _GL_STDIO_H */
#endif /* _GL_STDIO_H */
#endif
