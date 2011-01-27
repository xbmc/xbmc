/***************************************************************************
                          libini.h  -  Header file of functions to
                                       manipulate an ini file.
                             -------------------
    begin                : Fri Apr 21 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 * To generate a swig wrapper use:
 *
 *     swig -<lang> -module <lang>ini -o libini_<lang>.c
 * e.g:
 *     swig -tcl -module tclini -o libini_tcl.c
 *
 * The resulting c file must be built as a runtime loadable library and
 * linked against the core ini library.  For importing the code use:
 *     TCL:  load libtclini
 *     PERL: use perlini; require perlini;
 ***************************************************************************/

#ifndef _libini_h_
#define _libini_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define INI_ADD_EXTRAS
#define INI_ADD_LIST_SUPPORT

#ifdef SWIG
%ignore ini_readString;
%include typemaps.i
%apply int    *INOUT { int    *value };
%apply long   *INOUT { long   *value };
%apply double *INOUT { double *value };
%rename (ini_readString) ini_readFileToBuffer;
#define INI_EXTERN
#define INI_STATIC
#endif /* SWIG */

//#ifdef _WINDOWS
//#   define INI_LINKAGE __stdcall
//#else
#   define INI_LINKAGE 
//#endif

/* DLL building support on win32 hosts */
#ifndef INI_EXTERN
#   ifdef DLL_EXPORT          /* defined by libtool (if required) */
#       define INI_EXTERN __declspec(dllexport)
#   endif
#   ifdef LIBINI_DLL_IMPORT   /* define if linking with this dll */
#       define INI_EXTERN extern __declspec(dllimport)
#   endif
#   ifndef INI_EXTERN         /* static linking or !_WIN32 */
#       define INI_EXTERN extern
#   endif
#endif

#ifndef INI_ADD_EXTRAS
#undef  INI_ADD_LIST_SUPPORT
#endif


typedef void* ini_fd_t;

/* Rev 1.2 Added new fuction */
INI_EXTERN ini_fd_t INI_LINKAGE ini_open     (const char *name, const char *mode,
                                              const char *comment);
INI_EXTERN int      INI_LINKAGE ini_close    (ini_fd_t fd);
INI_EXTERN int      INI_LINKAGE ini_flush    (ini_fd_t fd);
INI_EXTERN int      INI_LINKAGE ini_delete   (ini_fd_t fd);

/* Rev 1.2 Added these functions to make life a bit easier, can still be implemented
 * through ini_writeString though. */
INI_EXTERN int INI_LINKAGE ini_locateKey     (ini_fd_t fd, const char *key);
INI_EXTERN int INI_LINKAGE ini_locateHeading (ini_fd_t fd, const char *heading);
INI_EXTERN int INI_LINKAGE ini_deleteKey     (ini_fd_t fd);
INI_EXTERN int INI_LINKAGE ini_deleteHeading (ini_fd_t fd);

/* Returns the number of bytes required to be able to read the key as a
 * string from the file. (1 should be added to this length to account
 * for a NULL character).  If delimiters are used, returns the length
 * of the next data element in the key to be read */
INI_EXTERN int INI_LINKAGE ini_dataLength (ini_fd_t fd);

/* Default Data Type Operations
 * Arrays implemented to help with reading, for writing you should format the
 * complete array as a string and perform an ini_writeString. */
INI_EXTERN int INI_LINKAGE ini_readString  (ini_fd_t fd, char *str, size_t size);
INI_EXTERN int INI_LINKAGE ini_writeString (ini_fd_t fd, const char *str);
INI_EXTERN int INI_LINKAGE ini_readInt     (ini_fd_t fd, int *value);


#ifdef INI_ADD_EXTRAS
    /* Read Operations */
    INI_EXTERN int INI_LINKAGE ini_readLong    (ini_fd_t fd, long   *value);
    INI_EXTERN int INI_LINKAGE ini_readDouble  (ini_fd_t fd, double *value);
    INI_EXTERN int INI_LINKAGE ini_readBool    (ini_fd_t fd, int    *value);

    /* Write Operations */
    INI_EXTERN int INI_LINKAGE ini_writeInt    (ini_fd_t fd, int    value);
    INI_EXTERN int INI_LINKAGE ini_writeLong   (ini_fd_t fd, long   value);
    INI_EXTERN int INI_LINKAGE ini_writeDouble (ini_fd_t fd, double value);
    INI_EXTERN int INI_LINKAGE ini_writeBool   (ini_fd_t fd, int    value);

    /* Extra Functions */
    INI_EXTERN int INI_LINKAGE ini_append      (ini_fd_t fddst, ini_fd_t fdsrc);
#endif /* INI_ADD_EXTRAS */


#ifdef INI_ADD_LIST_SUPPORT
    /* Rev 1.1 Added - List Operations (Used for read operations only)
     * Be warned, once delimiters are set, every key that is read will be checked for the
     * presence of sub strings.  This will incure a speed hit and therefore once a line
     * has been read and list/array functionality is not required, set delimiters
     * back to NULL.
     */

    /* Returns the number of elements in an list being seperated by the provided delimiters */
    INI_EXTERN int INI_LINKAGE ini_listLength      (ini_fd_t fd);
    /* Change delimiters, default "" */
    INI_EXTERN int INI_LINKAGE ini_listDelims      (ini_fd_t fd, const char *delims);
    /* Set index to access in a list.  When read the index will automatically increment */
    INI_EXTERN int INI_LINKAGE ini_listIndex       (ini_fd_t fd, unsigned long index);
#endif /* INI_ADD_LIST_SUPPORT */


#ifdef SWIG
%{
#include <libini.h>
#define INI_STATIC static
typedef struct
{
    char  *buffer;
    size_t size;
} ini_buffer_t;
%}

%inline %{
/*************************************************************
 * SWIG helper functions to create C compatible string buffers
 *************************************************************/
INI_STATIC ini_buffer_t *ini_createBuffer (unsigned int size)
{
    ini_buffer_t *b;
    /* Allocate memory to structure */
    if (size == ( ((unsigned) -1 << 1) >> 1 ))
        return 0; /* Size is too big */
    b = malloc (sizeof (ini_buffer_t));
    if (!b)
        return 0;

    /* Allocate memory to buffer */
    b->buffer = malloc (sizeof (char) * (size + 1));
    if (!b->buffer)
    {
        free (b);
        return 0;
    }
    b->size      = size;
    b->buffer[0] = '\0';

    /* Returns address to tcl */
    return b;
}

INI_STATIC void ini_deleteBuffer (ini_buffer_t *buffer)
{
    if (!buffer)
        return;
    free (buffer->buffer);
    free (buffer);
}

INI_STATIC int ini_readFileToBuffer (ini_fd_t fd, ini_buffer_t *buffer)
{
    if (!buffer)
        return -1;
    return ini_readString (fd, buffer->buffer, buffer->size + 1);
}

INI_STATIC char *ini_getBuffer (ini_buffer_t *buffer)
{
    if (!buffer)
        return "";
    return buffer->buffer;
}

INI_STATIC int ini_setBuffer (ini_buffer_t *buffer, const char *str)
{
    size_t len;
    if (!buffer)
        return -1;
    len = strlen (str);
    if (len > buffer->size)
        len = buffer->size;

    memcpy (buffer->buffer, str, len);
    buffer->buffer[len] = '\0';
    return len;
}
%}
#endif /* SWIG */

#ifdef __cplusplus
}
#endif

#endif /* _libini_h_ */
