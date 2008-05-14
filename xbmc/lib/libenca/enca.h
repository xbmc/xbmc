/* @(#) $Id: enca.h,v 1.11 2005/02/27 12:08:55 yeti Exp $ */
/* This header file is in the public domain. */
#ifndef ENCA_H
#define ENCA_H

#include <stdlib.h>
/* According to autoconf stdlib may not be enough for size_t */
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Enumerated types */

typedef enum { /*< flags >*/
  ENCA_SURFACE_EOL_CR    = 1 << 0,
  ENCA_SURFACE_EOL_LF    = 1 << 1,
  ENCA_SURFACE_EOL_CRLF  = 1 << 2,
  ENCA_SURFACE_EOL_MIX   = 1 << 3,
  ENCA_SURFACE_EOL_BIN   = 1 << 4,
  ENCA_SURFACE_MASK_EOL  = (ENCA_SURFACE_EOL_CR
                            | ENCA_SURFACE_EOL_LF
                            | ENCA_SURFACE_EOL_CRLF
                            | ENCA_SURFACE_EOL_MIX
                            | ENCA_SURFACE_EOL_BIN),
  ENCA_SURFACE_PERM_21    = 1 << 5,
  ENCA_SURFACE_PERM_4321  = 1 << 6,
  ENCA_SURFACE_PERM_MIX   = 1 << 7,
  ENCA_SURFACE_MASK_PERM  = (ENCA_SURFACE_PERM_21
                             | ENCA_SURFACE_PERM_4321
                             | ENCA_SURFACE_PERM_MIX),
  ENCA_SURFACE_QP        = 1 << 8,
  ENCA_SURFACE_REMOVE    = 1 << 13,
  ENCA_SURFACE_UNKNOWN   = 1 << 14,
  ENCA_SURFACE_MASK_ALL  = (ENCA_SURFACE_MASK_EOL
                            | ENCA_SURFACE_MASK_PERM
                            | ENCA_SURFACE_QP
                            | ENCA_SURFACE_REMOVE)
} EncaSurface;

typedef enum {
  ENCA_NAME_STYLE_ENCA,
  ENCA_NAME_STYLE_RFC1345,
  ENCA_NAME_STYLE_CSTOCS,
  ENCA_NAME_STYLE_ICONV,
  ENCA_NAME_STYLE_HUMAN,
  ENCA_NAME_STYLE_MIME
} EncaNameStyle;

typedef enum { /*< flags >*/
  ENCA_CHARSET_7BIT      = 1 << 0,
  ENCA_CHARSET_8BIT      = 1 << 1,
  ENCA_CHARSET_16BIT     = 1 << 2,
  ENCA_CHARSET_32BIT     = 1 << 3,
  ENCA_CHARSET_FIXED     = 1 << 4,
  ENCA_CHARSET_VARIABLE  = 1 << 5,
  ENCA_CHARSET_BINARY    = 1 << 6,
  ENCA_CHARSET_REGULAR   = 1 << 7,
  ENCA_CHARSET_MULTIBYTE = 1 << 8
} EncaCharsetFlags;

typedef enum {
  ENCA_EOK = 0,
  ENCA_EINVALUE,
  ENCA_EEMPTY,
  ENCA_EFILTERED,
  ENCA_ENOCS8,
  ENCA_ESIGNIF,
  ENCA_EWINNER,
  ENCA_EGARBAGE
} EncaErrno;

#define ENCA_CS_UNKNOWN (-1)

#define ENCA_NOT_A_CHAR 0xffff

/* Published (opaque) typedefs  */
typedef struct _EncaAnalyserState *EncaAnalyser;

/* Public (transparent) typedefs */
typedef struct _EncaEncoding EncaEncoding;

struct _EncaEncoding { int charset; EncaSurface surface; };

/* Basic interface. */
EncaAnalyser  enca_analyser_alloc (const char *langname);
void          enca_analyser_free  (EncaAnalyser analyser);
EncaEncoding  enca_analyse        (EncaAnalyser analyser,
                                   unsigned char *buffer,
                                   size_t size);
EncaEncoding  enca_analyse_const  (EncaAnalyser analyser,
                                   const unsigned char *buffer,
                                   size_t size);
int           enca_double_utf8_check (EncaAnalyser analyser,
                                      const unsigned char *buffer,
                                      size_t size);
int*          enca_double_utf8_get_candidates (EncaAnalyser analyser);
int           enca_errno          (EncaAnalyser analyser);
const char*   enca_strerror       (EncaAnalyser analyser,
                                   int errnum);

/* Options. */
void          enca_set_multibyte              (EncaAnalyser analyser,
                                               int multibyte);
int           enca_get_multibyte              (EncaAnalyser analyser);
void          enca_set_interpreted_surfaces   (EncaAnalyser analyser,
                                               int interpreted_surfaces);
int           enca_get_interpreted_surfaces   (EncaAnalyser analyser);
void          enca_set_ambiguity              (EncaAnalyser analyser,
                                               int ambiguity);
int           enca_get_ambiguity              (EncaAnalyser analyser);
void          enca_set_filtering              (EncaAnalyser analyser,
                                               int filtering);
int           enca_get_filtering              (EncaAnalyser analyser);
void          enca_set_garbage_test           (EncaAnalyser analyser,
                                               int garabage_test);
int           enca_get_garbage_test           (EncaAnalyser analyser);
void          enca_set_termination_strictness (EncaAnalyser analyser,
                                               int termination_strictness);
int           enca_get_termination_strictness (EncaAnalyser analyser);
int           enca_set_significant            (EncaAnalyser analyser,
                                               size_t significant);
size_t        enca_get_significant            (EncaAnalyser analyser);
int           enca_set_threshold              (EncaAnalyser analyser,
                                               double threshold);
double        enca_get_threshold              (EncaAnalyser analyser);

/* Names and properties. */
const char*       enca_charset_name            (int charset,
                                                EncaNameStyle whatname);
const char**      enca_get_charset_aliases     (int charset,
                                                size_t *n);
char*             enca_get_surface_name        (EncaSurface surface,
                                                EncaNameStyle whatname);
EncaEncoding      enca_parse_encoding_name     (const char *name);
EncaSurface       enca_charset_natural_surface (int charset);
EncaCharsetFlags  enca_charset_properties      (int charset);

#define enca_charset_is_known(cs) \
  ((cs) != ENCA_CS_UNKNOWN)
#define enca_charset_is_7bit(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_7BIT)
#define enca_charset_is_8bit(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_8BIT)
#define enca_charset_is_16bit(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_16BIT)
#define enca_charset_is_32bit(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_32BIT)
#define enca_charset_is_fixed(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_FIXED)
#define enca_charset_is_variable(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_VARIABLE)
#define enca_charset_is_binary(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_BINARY)
#define enca_charset_is_regular(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_REGULAR)
#define enca_charset_is_multibyte(cs) \
  (enca_charset_properties(cs) & ENCA_CHARSET_MULTIBYTE)

/* Auxiliary functions. */
int           enca_charset_has_ucs2_map  (int charset);
int           enca_charset_ucs2_map      (int charset,
                                          unsigned int *buffer);
size_t        enca_number_of_charsets    (void);
const char*   enca_analyser_language     (EncaAnalyser analyser);
const char*   enca_language_english_name (const char *lang);
const char**  enca_get_languages         (size_t *n);
int*          enca_get_language_charsets (const char *langname,
                                          size_t *n);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
