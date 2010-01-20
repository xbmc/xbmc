/* @(#) $Id: internal.h,v 1.25 2005/12/01 10:08:53 yeti Exp $ */
#ifndef LIBENCA_H
#define LIBENCA_H
/***************************************************************************
 *
 *  Do not use anything from this file in applications.
 *  Or else don't be surprised when they mysteriously crash.
 *  Changes in internal interfaces DON'T count as interface
 *  changes and DON'T cause library API version changes.
 *
 ***************************************************************************/

#include <assert.h>

#include "enca.h"

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

#ifdef DEBUG
#  include <stdio.h>
#endif /* DEBUG */

/* Simple macro statement wrappers. Use do / while (0) since the other cases
 * tend to produce an incredible amount of gcc warnings with -pedantic. */
#define ENCA_STMT_START do
#define ENCA_STMT_END while (0)

/* Flags for character type table.
 * 0-10 are standard ones, 11-13 Enca-specific. */
enum {
  ENCA_CTYPE_ALNUM  = 1 << 0,
  ENCA_CTYPE_ALPHA  = 1 << 1,
  ENCA_CTYPE_CNTRL  = 1 << 2,
  ENCA_CTYPE_DIGIT  = 1 << 3,
  ENCA_CTYPE_GRAPH  = 1 << 4,
  ENCA_CTYPE_LOWER  = 1 << 5,
  ENCA_CTYPE_PRINT  = 1 << 6,
  ENCA_CTYPE_PUNCT  = 1 << 7,
  ENCA_CTYPE_SPACE  = 1 << 8,
  ENCA_CTYPE_UPPER  = 1 << 9,
  ENCA_CTYPE_XDIGIT = 1 << 10,
  ENCA_CTYPE_NAME   = 1 << 11,
  ENCA_CTYPE_BINARY = 1 << 12,
  ENCA_CTYPE_TEXT   = 1 << 13
};

/* Forward delcarations of structured Enca types */
typedef struct _EncaAnalyserOptions EncaAnalyserOptions;
typedef struct _EncaAnalyserState EncaAnalyserState;
typedef struct _EncaCharsetInfo EncaCharsetInfo;
typedef struct _EncaLanguageInfo EncaLanguageInfo;
typedef struct _EncaLanguageHookData1CS EncaLanguageHookData1CS;
typedef struct _EncaLanguageHookDataEOL EncaLanguageHookDataEOL;
typedef struct _EncaUTFCheckData EncaUTFCheckData;

/**
 * EncaCharsetInfo:
 * @enca: Default, implicit name in enca.
 * @rfc1345: RFC1345 charset name.
 *          (For charsets not in RFC1345, some canonical name is invented.)
 * @cstocs: Cstocs charset name or -1.
 * @iconv: Iconv charset name or -1.
 * @mime: Preferred MIME charset name or -1.
 * @human: Human comprehensible description.
 * @flags: Charset properties (7bit, 8bit, multibyte, ...).
 * @nsurface: Natural surface (`implied' in recode).
 *
 * General charset informnations.
 *
 * All the #int fields are indices in #ALIAS_LIST[].
 **/
struct _EncaCharsetInfo {
  int enca;
  int rfc1345;
  int cstocs;
  int iconv;
  int mime;
  const char *human;
  unsigned int flags;
  unsigned int nsurface;
};

/**
 * EncaHookFunc:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Language hook function type.
 *
 * Launches language specific hooks for a particular language.
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
typedef int (* EncaHookFunc)(EncaAnalyserState *analyser);

/**
 * EncaGuessFunc:
 * @analyser: Analyser state whose buffer should be checked.
 *
 * Special (multibyte) encoding check function type.
 *
 * Returns: Nonzero if analyser->result has been set, zero otherwise.
 **/
typedef int (* EncaGuessFunc)(EncaAnalyserState *analyser);

/**
 * EncaLanguageInfo:
 * @name: Language name, or more precisely, locale name.
 * @humanname: Normal human-readable [English] language name.
 * @ncharsets: Number of charsets in this language.
 * @csnames: Charset names [@ncharsets].
 * @weights: Character weights for charsets [@ncharsets][0x100].
 * @significant: Character significancy data [0x100].
 * @letters: Characters considered letters (255's have no entry in @pairs,
 *           zeroes are non-letters aka FILL_NONLETTERs)
 * @pairs: Frequent pair table [max number in @letters].
 * @weight_sum: Sum of all @weights (is the same for all charsets).
 * @hook: Hook function (deciding hard cases).
 * @eolhook: EOL hook function (deciding ambiguous cases based on EOL type).
 * @ratinghook: Helper to calculate ratings for weightingless languages.
 *
 * Language specific data.
 **/
struct _EncaLanguageInfo {
  const char *name;
  const char *humanname;
  size_t ncharsets;
  const char *const *csnames;
  const unsigned short int *const *weights;
  const unsigned short int *significant;
  const unsigned char *const *letters;
  const unsigned char **const *pairs;
  long int weight_sum;
  EncaHookFunc hook;
  EncaHookFunc eolhook;
  EncaHookFunc lcuchook;
  EncaHookFunc ratinghook;
};

/**
 * EncaAnalyserOptions:
 * @const_buffer: Treat buffer as const?  Otherwise its content can be,
 *                and probably will be, modified.
 * @min_chars: Minimal number significant characters.
 * @threshold: Minimal ratio between winner and the second.
 * @multibyte_enabled: Check for multibyte encodings?
 * @interpreted_surfaces: Allow surfaces causing fundamental reinterpretation?
 * @ambiguous_mode: Ambiguous mode?
 * @filtering: Allow binary and box-drawing filters?
 * @test_garbageness: Do test garbageness?
 * @termination_strictness: Disallow broken multibyte sequences at buffer end?
 *
 * Analyser options, a part of analyser state.
 **/
struct _EncaAnalyserOptions {
  int const_buffer;
  size_t min_chars;
  double threshold;
  int multibyte_enabled;
  int interpreted_surfaces;
  int ambiguous_mode;
  int filtering;
  int test_garbageness;
  int termination_strictness;
};

/**
 * EncaAnalyserState:
 * @lang: Language informations.
 * @ncharsets: Number of 8bit charsets in this language.
 *             (Equal to @lang->ncharsets.)
 * @charsets: 8bit charset id's [@ncharsets].
 * @gerrno: Guessing gerrno.
 * @size: Size of buffer.
 * @buffer: Buffer whose encoding is to be detected [@size].
 *         (Owned by outer world.)
 * @result: Result returned to caller.
 * @counts: Character counts [0x100].
 * @bin: Number of `binary' characters.
 * @up: Number of 8bit characters.
 * @ratings: 8bit charset ratings [@ncharsets].
 * @order: Charset indices (not id's) sorted by ratings in descending order
 *         [ncharsets].
 * @size2: Size of buffer2.
 * @buffer2: A temporary secondary buffer [@size2].
 * @utfch: Double-UTF-8 test data [@ncharsets].
 * @utfbuf: Double-UTF-8 buffer for various UCS-2 character counting [0x10000].
 *          (Magic: see mark_scratch_buffer() for description.)
 * @pair2bits: Character pair map to charsets [0x100000] (indexed
 *             0x100*first + second).  Each bit corresponds to one charset,
 *             when set, the pair is `good' for the given charset.  The
 *             type is char, so it breaks for @ncharsets > 8, but it should
 *             not be accessed from outer world, so it can be easily enlarged
 *             to more bits.
 * @bitcounts: Counts for each possible bit combinations in @pair2bits
 *             [0x1 << ncharsets].
 * @pairratings: Counts of `good' pairs per charset [@ncharsets].
 * @lcbits: If a character is lowercase in some charset, correspinding bit
 *          is set [0x100].
 * @ucbits: If a character is uppercase in some charset, correspinding bit
 *          is set [0x100].
 * @options: Analyser options.
 *
 * The internal analyser state.
 *
 * Passed as an opaque object (`this') to analyser calls.
 **/
struct _EncaAnalyserState {
  /* Language data. */
  const EncaLanguageInfo *lang;
  size_t ncharsets;
  int *charsets;
  /* Analyser state. */
  EncaErrno gerrno;
  size_t size;
  unsigned char *buffer;
  EncaEncoding result;
  size_t *counts;
  size_t bin;
  size_t up;
  double *ratings;
  size_t *order;
  size_t size2;
  unsigned char *buffer2;
  /* Double-UTF-8 data. */
  EncaUTFCheckData *utfch;
  int *utfbuf;
  /* Pair frequency data */
  unsigned char *pair2bits;
  size_t *bitcounts;
  size_t *pairratings;
  /* LCUC data XXX: unused (yet) */
  size_t *lcbits;
  size_t *ucbits;
  /* Options. */
  EncaAnalyserOptions options;
};

/**
 * EncaLanguageHookData1CS:
 * @name: Charset name.
 * @size: Number of characters in @list.
 * @list: Extra-important character list for the charset.
 * @cs: Charset number.  This is an index in @analyser arrays (like @charsets),
 *      NOT a charset id.
 *
 * Cointainer for data needed by enca_language_hook_ncs().
 **/
struct _EncaLanguageHookData1CS {
  const char *name;
  size_t size;
  const unsigned char *list;
  size_t cs;
};

/**
 * EncaLanguageHookDataEOL:
 * @name: Charset name.
 * @eol: The corresponding #EncaSurface bit.
 * @cs: Charset number.  This is an index in @analyser arrays (like @charsets),
 *      NOT a charset id.
 *
 * Cointainer for data needed by enca_language_hook_eol().
 **/
struct _EncaLanguageHookDataEOL {
  const char *name;
  EncaSurface eol;
  size_t cs;
};

/**
 * EncaUTFCheckData:
 * @rating: Total rating for this charset.
 * @size: Number of UCS-2 characters.
 * @result: Nonzero when the sample is probably Doubly-UTF-8 encoded from
 *          this charset.
 * @ucs2: List of significant UCS-2 characters, in order [@size].
 * @weights: Weights for double-UTF-8 check [@size].  Positive means normal
 *           UTF-8, negative doubly-encoded.
 *
 * Data needed by double-UTF-8 check, per language charset.
 **/
struct _EncaUTFCheckData {
  double rating;
  size_t size;
  int result;
  int *ucs2;
  int *weights;
};

/**
 * FILL_NONLETTER:
 *
 * Replacement character for non-letters in pair frequencies.
 **/
#define FILL_NONLETTER '.'

/**
 * EPSILON:
 *
 * `Zero' for float comparsion (and to prevent division by zero, etc.).
 **/
#define EPSILON 0.000001

/**
 * LF:
 *
 * Line feed character (End-of-line on Unix).
 **/
#define LF ((unsigned char)'\n')

/**
 * CR:
 *
 * Carriage return character (End-of-line on Macintosh).
 **/
#define CR ((unsigned char)'\r')

/* Character type macros.
 *
 * The `text' and `binary' flags mark characters that can cause switch to
 * binary/text mode in filter_binary().  The view of what is text and what
 * is binary is quite simplistic, as we don't know the charset...
 *
 * The `name' flag marks characters acceptable in charset identifiers.
 **/
#define enca_ctype_test(c, t) ((enca_ctype_data[(unsigned char)c] & t) != 0)

#define enca_isalnum(c)  enca_ctype_test((c), ENCA_CTYPE_ALNUM)
#define enca_isalpha(c)  enca_ctype_test((c), ENCA_CTYPE_ALPHA)
#define enca_iscntrl(c)  enca_ctype_test((c), ENCA_CTYPE_CNTRL)
#define enca_isdigit(c)  enca_ctype_test((c), ENCA_CTYPE_DIGIT)
#define enca_isgraph(c)  enca_ctype_test((c), ENCA_CTYPE_GRAPH)
#define enca_islower(c)  enca_ctype_test((c), ENCA_CTYPE_LOWER)
#define enca_isprint(c)  enca_ctype_test((c), ENCA_CTYPE_PRINT)
#define enca_ispunct(c)  enca_ctype_test((c), ENCA_CTYPE_PUNCT)
#define enca_isspace(c)  enca_ctype_test((c), ENCA_CTYPE_SPACE)
#define enca_isupper(c)  enca_ctype_test((c), ENCA_CTYPE_UPPER)
#define enca_isxdigit(c) enca_ctype_test((c), ENCA_CTYPE_XDIGIT)
#define enca_isname(c)   enca_ctype_test((c), ENCA_CTYPE_NAME)
#define enca_isbinary(c) enca_ctype_test((c), ENCA_CTYPE_BINARY)
#define enca_istext(c)   enca_ctype_test((c), ENCA_CTYPE_TEXT)

/**
 * ELEMENTS:
 * @array: An array whose size is to be computed.
 *
 * Compute the number of elements of a static array.
 *
 * Returns: the number of elements.
 **/
#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

void*  enca_malloc  (size_t size);
void*  enca_realloc (void *ptr,
                     size_t size);

/**
 * enca_free:
 * @ptr: Pointer to memory to free.
 *
 * Frees memory pointed by @ptr with free() hack and assigns it a safe value,
 * thus may be called more than once.
 *
 * @ptr MUST be l-value.
 **/
#define enca_free(ptr) \
  ENCA_STMT_START{ if (ptr) free(ptr); ptr=NULL; }ENCA_STMT_END

/**
 * NEW:
 * @type: Data type to allocate.
 * @n: Number of elements to allocate.
 *
 * An enca_malloc() wrapper.
 *
 * Returns: Pointer to the newly allocated memory.
 **/
#define NEW(type,n) ((type*)enca_malloc((n)*sizeof(type)))

/**
 * RENEW:
 * @ptr: Pointer to already allocate memory or #NULL.
 * @type: Data type to allocate.
 * @n: Number of elements to resize the memory to.
 *
 * An enca_realloc() wrapper.
 *
 * Returns: Pointer to the reallocated memory (or pointer safe to call free()
 * on when @n is zero).
 **/
#define RENEW(ptr,type,n) ((type*)enca_realloc((ptr),(n)*sizeof(type)))

/**
 * MAKE_HOOK_LINE:
 * @name: A charset name in C-style identifier suitable form.
 *
 * Ugly code `beautifier' macro for language hooks.
 **/
#define MAKE_HOOK_LINE(name) \
  { #name, ELEMENTS(list_##name), list_##name, (size_t)-1 }

/* Always use our, since we rely on enca_strdup(NULL) -> NULL */
char* enca_strdup(const char *s);

#ifndef HAVE_STRSTR
const char* enca_strstr(const char *haystack,
                        const char* needle);
#else/* not HAVE_STRSTR */
# define enca_strstr strstr
#endif /* not HAVE_STRSTR */

#ifndef HAVE_STPCPY
char* enca_stpcpy(char *dest,
                  const char *src);
#else /* not HAVE_STPCPY */
# define enca_stpcpy stpcpy
#endif /* not HAVE_STPCPY */

/**
 * enca_csname:
 * @cs: A charset id.
 *
 * A shorthand for printing names with #ENCA_NAME_STYLE_ENCA.
 **/
#define enca_csname(cs) enca_charset_name((cs), ENCA_NAME_STYLE_ENCA)

/* common.c */
char* enca_strconcat (const char *str,
                      ...);
char* enca_strappend (char *str,
                      ...);

/* encnames.c */
int         enca_name_to_charset  (const char *csname);
EncaSurface enca_name_to_surface  (const char *sname);

/* enca.c */
int         enca_language_init    (EncaAnalyserState *analyser,
                                   const char *langname);
void        enca_language_destroy (EncaAnalyserState *analyser);
double*     enca_get_charset_similarity_matrix(const EncaLanguageInfo *lang);

/* unicodemap.c */
int         enca_charsets_subset_identical (int charset1,
                                            int charset2,
                                            const size_t *counts);

/* filters.c */
size_t      enca_filter_boxdraw    (EncaAnalyserState *analyser,
                                    unsigned char fill_char);
int         enca_language_hook_ncs (EncaAnalyserState *analyser,
                                    size_t ncs,
                                    EncaLanguageHookData1CS *hookdata);
int         enca_language_hook_eol (EncaAnalyserState *analyser,
                                    size_t ncs,
                                    EncaLanguageHookDataEOL *hookdata);

/* guess.c */
void        enca_guess_init    (EncaAnalyserState *analyser);
void        enca_guess_destroy (EncaAnalyserState *analyser);
EncaSurface enca_eol_surface   (const unsigned char *buffer,
                                size_t size,
                                const size_t *counts);
void        enca_find_max_sec  (EncaAnalyserState *analyser);

/* utf8_double.c */
void        enca_double_utf8_init    (EncaAnalyserState *analyser);
void        enca_double_utf8_destroy (EncaAnalyserState *analyser);

/* pair.c */
void        enca_pair_init    (EncaAnalyserState *analyser);
void        enca_pair_destroy (EncaAnalyserState *analyser);
int         enca_pair_analyse (EncaAnalyserState *analyser);

/* Languages. */
extern const EncaLanguageInfo ENCA_LANGUAGE_BE;
extern const EncaLanguageInfo ENCA_LANGUAGE_BG;
extern const EncaLanguageInfo ENCA_LANGUAGE_CS;
extern const EncaLanguageInfo ENCA_LANGUAGE_ET;
extern const EncaLanguageInfo ENCA_LANGUAGE_HR;
extern const EncaLanguageInfo ENCA_LANGUAGE_HU;
extern const EncaLanguageInfo ENCA_LANGUAGE_LT;
extern const EncaLanguageInfo ENCA_LANGUAGE_LV;
extern const EncaLanguageInfo ENCA_LANGUAGE_PL;
extern const EncaLanguageInfo ENCA_LANGUAGE_RU;
extern const EncaLanguageInfo ENCA_LANGUAGE_SK;
extern const EncaLanguageInfo ENCA_LANGUAGE_SL;
extern const EncaLanguageInfo ENCA_LANGUAGE_UK;
extern const EncaLanguageInfo ENCA_LANGUAGE_ZH;

/* Multibyte test lists.
 * These arrays must be NULL-terminated. */
extern EncaGuessFunc ENCA_MULTIBYTE_TESTS_ASCII[];
extern EncaGuessFunc ENCA_MULTIBYTE_TESTS_8BIT[];
extern EncaGuessFunc ENCA_MULTIBYTE_TESTS_BINARY[];
extern EncaGuessFunc ENCA_MULTIBYTE_TESTS_8BIT_TOLERANT[];

/* Locale-independent character type table. */
extern const short int enca_ctype_data[0x100];

#endif /* not LIBENCA_H */
