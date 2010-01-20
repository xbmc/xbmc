/* @(#) $Id: common.h,v 1.28 2005/02/27 12:08:56 yeti Exp $ */
#ifndef COMMON_H
#define COMMON_H 1

#ifndef PACKAGE_NAME
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  else /* HAVE_CONFIG_H */
#    define PACKAGE_NAME "Enca"
#    define PACKAGE_TARNAME "enca"
#    define PACKAGE_VERSION ""
#    define DEFAULT_EXTERNAL_CONVERTER ""
#    define DEFAULT_CONVERTER_LIST "built-in"
#  endif /* HAVE_CONFIG_H */
#endif /* not PACKAGE_NAME */

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>

#include "../lib/internal.h"

/* define or correctly redefine EXIT_* values */
#if !(defined EXIT_SUCCESS) || (EXIT_SUCCESS != 0)
#  define EXIT_SUCCESS 0
#  define EXIT_FAILURE 1
#endif /* !(defined EXIT_SUCCESS) || (EXIT_SUCCESS != 0) */
#define EXIT_TROUBLE 2

/* str- an mem- function, theoretically they are all in string.h */
#ifdef HAVE_STRING_H
#  include <string.h>
#else /* HAVE_STRING_H */
#  ifdef HAVE_STRINGS_H
#    include <strings.h>
#  endif /* HAVE_STRINGS_H */
#endif /* HAVE_STRING_H */

#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#else /* HAVE_ERRNO_H */
extern int errno;
#endif /* HAVE_ERRNO_H */

/* portable isatty(), assume never on tty when neither isatty/ttyname is
   available */
#ifdef HAVE_ISATTY
#  define enca_isatty(fd) (isatty(fd))
#elif HAVE_TTYNAME
#  define enca_isatty (ttyname(fd) != NULL)
#else /* HAVE_ISATTY || HAVE_TTYNAME */
#  define enca_isatty (0)
#endif /* HAVE_ISATTY || HAVE_TTYNAME */

/* make sure STDIN_FILENO and STDOUT_FILENO are defined */
#ifndef STDIN_FILENO
#  define STDIN_FILENO 0
#endif /* not STDIN_FILENO */

#ifndef STDOUT_FILENO
#  define STDOUT_FILENO 1
#endif /* not STDOUT_FILENO */

/* Conversion error codes:
   ok
   conversion between these encodings is not possible
   i/o failure,
   child died
   converter library is cheating
   malformed input,
   cannot exec external converter

   FIXME: this is an ISO C violation E[a-z0-9]+ are reserved for error names
 */
typedef enum {
  ERR_OK =  0,
  ERR_CANNOT,
  ERR_IOFAIL,
  ERR_CHILD,
  ERR_LIBCOM,
  ERR_MALFORM,
  ERR_EXEC = 15
} ConvertErrorCode;

/* Output type. */
typedef enum {
  OTYPE_CS2CS = 0,
  OTYPE_RFC1345,
  OTYPE_HUMAN,
  OTYPE_DETAILS,
  OTYPE_CANON,
  OTYPE_ICONV,
  OTYPE_MIME,
  OTYPE_CONVERT,
  OTYPE_ALIASES
} OutputType;

typedef void *pointer; /* untyped pointer */
typedef const void *cpointer; /* constant untyped pointer */
typedef unsigned char byte; /* byte */

/* Forward type declarations. */
typedef struct _Abbreviation Abbreviation;
typedef struct _Buffer Buffer;
typedef struct _File File;
typedef struct _Options Options;

/* Struct abbreviation. */
struct _Abbreviation {
  const char *name; /* full name */
  cpointer data; /* corresponding value */
};

/* Struct I/O buffer. */
struct _Buffer {
  size_t size; /* buffer size */
  ssize_t pos; /* position in buffer */
  byte *data; /* the buffer itself buffer */
};

/* Struct file stream. */
struct _File {
  char *name; /* file name, NULL if stdin/stdout */
  Buffer *buffer; /* buffer for i/o operations */
  FILE *stream; /* the stream, NULL when not opened */
  off_t size; /* file size, nonsense when name == NULL */
};

/* Struct options. */
struct _Options {
  int verbosity_level; /* Verbosity level. */
  char *language; /* Language of analysed files. */
  OutputType output_type; /* What kind of action is expected after guess. */
  EncaEncoding target_enc; /* Target encoding for conversion. */
  char *target_enc_str; /* How user specified the target encoding. */
  int prefix_filename; /* Do prepend filename: before results? */
};

/* Enca options. */
extern Options options;
/* Path-stripped argv[0] for everybody. */
extern char *program_name;
/* size of the main i/o buffer */
extern size_t buffer_size;

/* Function prototypes. */
Buffer *buffer_new(size_t size);
void    buffer_free(Buffer *buf);

const Abbreviation* expand_abbreviation(const char *name,
                                        const Abbreviation *atable,
                                        size_t size,
                                        const char *object_name);

const char* ffname_r(const char *fname);
const char* ffname_w(const char *fname);
ssize_t     file_read(File *file);
byte*       file_getline(File *file);
ssize_t     file_write(File *file);
File*       file_temporary(Buffer *buffer,
                           int ulink);
off_t       file_seek(File *file,
                      off_t offset,
                      int whence);
int         file_truncate(File *file,
                          off_t length);
int         file_unlink(const char *fname);
int         file_open(File *file,
                      const char *mode);
int         file_close(File *file);
File*       file_new(const char *fname,
                     Buffer *buffer);
void        file_free(File *file);
char**      process_opt(int argc,
                        char *argv[]);
void print_aliases(size_t cs);
OutputType get_output_type(void);

int         copy_and_convert       (File *file_from,
                                    File *file_to,
                                    const byte *xlat);
const char* format_request_string  (EncaEncoding e1,
                                    EncaEncoding e2,
                                    EncaSurface mask);
int         convert                (File *file,
                                    EncaEncoding from_enc);
int         add_converter          (const char *cname);
int         external_converter_listed (void);
void        print_converter_list   (void);
void        set_requested_enc      (EncaEncoding enc);

#ifdef HAVE_LIBRECODE
int         convert_recode         (File *file,
                                    EncaEncoding from_enc);
#endif /* HAVE_LIBRECODE */

#ifdef HAVE_GOOD_ICONV
int         convert_iconv          (File *file,
                                    EncaEncoding from_enc);
#endif /* HAVE_GOOD_ICONV */

#ifdef ENABLE_EXTERNAL
int         convert_external         (File *file,
                                      EncaEncoding from_enc);
void        set_external_converter   (const char *extc);
int         check_external_converter (void);
#endif /* ENABLE_EXTERNAL */

char*       detect_lang            (const char *lang);
#ifdef HAVE_NL_LANGINFO
const char* get_lang_codeset       (void);
#endif /* HAVE_NL_LANGINFO */

#endif /* not COMMON_H */
/* vim: ts=2
 */
