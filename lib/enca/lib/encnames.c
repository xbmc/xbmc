/*
  @(#) $Id: encnames.c,v 1.11 2005/12/01 10:08:53 yeti Exp $
  convert charset and surface names to internal representation and back

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
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "enca.h"
#include "internal.h"
#include "tools/encodings.h"

#define NCHARSETS (ELEMENTS(CHARSET_INFO))
#define NALIASES (ELEMENTS(ALIAS_LIST))
#define NSURFACES (ELEMENTS(SURFACE_INFO))

#define ENCODING_UNKNOWN { ENCA_CS_UNKNOWN, 0 }

/* tolower() and toupper() which never fail. */
#define enca_tolower(c) (enca_isupper(c) ? (c) + ('a' - 'A') : (c))
#define enca_toupper(c) (enca_islower(c) ? (c) - ('a' - 'A') : (c))

static const char *UNKNOWN_CHARSET_NAME = "unknown";
static const char *UNKNOWN_CHARSET_HUMAN = "Unrecognized encoding";
static const char *UNKNOWN_CHARSET_SYM = "???";

/* Surface separator (sometimes we need a character, sometimes a string). */
#define SURF_SEPARATOR '/'
#define SURF_SEPARATOR_STR "/"

/**
 * EncaSurfaceInfo:
 * @enca: Canonical identifier (#NULL when not applicable).
 * @human: Human readable name.
 * @bit: Appropriate ENCA_SURFACE_<something>.
 *
 * Surface information.
 **/
struct _EncaSurfaceInfo {
  const char *enca;
  const char *human;
  EncaSurface bit;
};

typedef struct _EncaSurfaceInfo EncaSurfaceInfo;

/* Local prototypes. */
static int squeeze_compare(const char *x,
                           const char *y);
static int alias_search(const char *const *alist,
                        int n,
                        const char *s);
static int check_surface_consistency(EncaSurface surface);
static int count_bits(unsigned long int x);
static int check_encoding_name(const char *name);

/* Surface information. */
static const EncaSurfaceInfo SURFACE_INFO[] = {
  {
    "CR",
    "CR line terminators",
    ENCA_SURFACE_EOL_CR
  },
  {
    "LF",
    "LF line terminators",
    ENCA_SURFACE_EOL_LF
  },
  {
    "CRLF",
    "CRLF line terminators",
    ENCA_SURFACE_EOL_CRLF
  },
  {
    NULL,
    "Mixed line terminators",
    ENCA_SURFACE_EOL_MIX
  },
  {
    NULL,
    "Surrounded by/intermixed with non-text data",
    ENCA_SURFACE_EOL_BIN
  },
  {
    "21",
    "Byte order reversed in pairs (1,2 -> 2,1)",
    ENCA_SURFACE_PERM_21
  },
  {
    "4321",
    "Byte order reversed in quadruples (1,2,3,4 -> 4,3,2,1)",
    ENCA_SURFACE_PERM_4321
  },
  {
    NULL,
    "Both little and big endian chunks, concatenated",
    ENCA_SURFACE_PERM_MIX
  },
  {
    "qp",
    "Quoted-printable encoded",
    ENCA_SURFACE_QP
  },
  {
    "",
    "",
    ENCA_SURFACE_REMOVE
  },
};

/**
 * enca_charset_name:
 * @charset: A charset id.
 * @whatname: Teh type of name you request.
 * 
 * Translates numeric charset id @charset to some kind of name.
 * 
 * Returns: The requested charset name; #NULL for invalid @whatname or
 * @charset, or when @whatname name doesn't exist for charset @charset
 * (#ENCA_CS_UNKNOWN is OK).
 **/
const char*
enca_charset_name(int charset,
                  EncaNameStyle whatname)
{
  const EncaCharsetInfo *cs;

  if (charset == ENCA_CS_UNKNOWN) {
    switch (whatname) {
      case ENCA_NAME_STYLE_RFC1345:
      case ENCA_NAME_STYLE_ENCA:
      case ENCA_NAME_STYLE_MIME:
      return UNKNOWN_CHARSET_NAME;

      case ENCA_NAME_STYLE_HUMAN:
      return UNKNOWN_CHARSET_HUMAN;

      case ENCA_NAME_STYLE_CSTOCS:
      case ENCA_NAME_STYLE_ICONV:
      return UNKNOWN_CHARSET_SYM;

      default:
      return NULL;
    }
  }
  if ((size_t)charset >= NCHARSETS)
    return NULL;

  cs = CHARSET_INFO + charset;
  switch (whatname) {
    case ENCA_NAME_STYLE_RFC1345:
    return ALIAS_LIST[cs->rfc1345];

    case ENCA_NAME_STYLE_HUMAN:
    return cs->human;

    case ENCA_NAME_STYLE_ENCA:
    return ALIAS_LIST[cs->enca];

    case ENCA_NAME_STYLE_CSTOCS:
    return cs->cstocs < 0 ? NULL : ALIAS_LIST[cs->cstocs];

    case ENCA_NAME_STYLE_ICONV:
    return cs->iconv < 0 ? NULL : ALIAS_LIST[cs->iconv];

    case ENCA_NAME_STYLE_MIME:
    return cs->mime < 0 ? NULL : ALIAS_LIST[cs->mime];

    default:
    return NULL;
  }

  /* just to placate gcc */
  return NULL;
}

/**
 * enca_get_charset_aliases:
 * @charset: A charset id.
 * @n: The number of aliases will be stored here.
 *
 * Returns list of accepted aliases for charset @charset.
 *
 * The list of aliases has to be freed by caller; the strings themselves
 * must be considered constant and must NOT be freed.
 *
 * Returns: The list of aliases, storing their number into *@n; #NULL for
 * invalid @charset (*@n is zero then).
 **/
const char**
enca_get_charset_aliases(int charset,
                         size_t *n)
{
  const char **aliases;
  size_t i, j;

  /* Compute total length.
   * FIXME: The list is known at compile time. */
  for (i = *n = 0; i < NALIASES; i++)
    if (INDEX_LIST[i] == charset) (*n)++;

  /* Create the list. */
  aliases = NEW(const char*, *n);
  for (i = j = 0; i < NALIASES; i++)
    if (INDEX_LIST[i] == charset)
      aliases[j++] = ALIAS_LIST[i];

  return aliases;
}

/**
 * enca_get_surface_name:
 * @surface: A surface.
 * @whatname: The type of name you request.
 *
 * Constructs surface name from surface flags @surface.
 *
 * Returns: The requested surface name; #NULL for invalid @whatname; empty
 * string for naming style not supporting surfaces.  In all cases, the
 * returned string must be freed by caller when no longer used.
 **/
char*
enca_get_surface_name(EncaSurface surface,
                      EncaNameStyle whatname)
{
  char *s;
  size_t i;

  switch (whatname) {
    /* these don't know/define surfaces so forget it */
    case ENCA_NAME_STYLE_CSTOCS:
    case ENCA_NAME_STYLE_RFC1345:
    case ENCA_NAME_STYLE_ICONV:
    case ENCA_NAME_STYLE_MIME:
    s = enca_strdup("");
    break;

    /* human readable name (each on separate line) */
    case ENCA_NAME_STYLE_HUMAN:
    s = enca_strdup("");
    for (i = 0; i < NSURFACES; i++) {
      if (SURFACE_INFO[i].bit & surface) {
        s = enca_strappend(s, SURFACE_INFO[i].human, "\n", NULL);
      }
    }
    break;

    /* canonical name (/recode style) */
    case ENCA_NAME_STYLE_ENCA:
    s = enca_strdup("");
    for (i = 0; i < NSURFACES; i++) {
      if ((SURFACE_INFO[i].bit & surface) && SURFACE_INFO[i].enca != NULL) {
        s = enca_strappend(s, SURF_SEPARATOR_STR, SURFACE_INFO[i].enca, NULL);
      }
    }
    break;

    default:
    s = NULL;
    break;
  }

  return s;
}

/**
 * enca_charset_properties:
 * @charset: A charset.
 *
 * Returns charset properties.
 *
 * Returns: The requested charset properties; zero for invalid @charset.
 **/
EncaCharsetFlags
enca_charset_properties(int charset)
{
  if ((size_t)charset >= NCHARSETS)
    return 0;
  return CHARSET_INFO[charset].flags;
}

/**
 * enca_charset_natural_surface:
 * @charset: A charset.
 *
 * Returns natural surface of a charset.
 *
 * Returns: The requested charset natural surface (called `implied' in recode),
 *          zero for invalid @charset or for charsets with no natural surface.
 *
 *          Natrual surface is the surface one expects for a given charset --
 *          e.g. CRLF EOLs for IBM/Microsoft charsets, CR EOLs for Macintosh
 *          charsets and LF EOLs for ISO/Unix charsets.
 **/
EncaSurface
enca_charset_natural_surface(int charset)
{
  if ((size_t)charset >= NCHARSETS)
    return 0;
  else
    return CHARSET_INFO[charset].nsurface;
}

/**
 * enca_number_of_charsets:
 *
 * Returns number of known charsets.
 *
 * Charsets idetifiers are assigned successively starting from zero, so last
 * charset has identifier enca_number_of_charsets() - 1.
 *
 * Returns: The number of charsets.
 **/
size_t
enca_number_of_charsets(void)
{
  return NCHARSETS;
}

/**
 * enca_parse_encoding_name:
 * @name: An encoding specification.
 *
 * Transofrms encoding specification charset/surface into numeric #EncaEncoding.
 *
 * When the charset name is not recognized, surfaces are not parsed at all and
 * #ENCA_CS_UNKNOWN is returned as charset.  However, unrecognized surfaces are
 * considered only a minor problem causing %ENCA_SURFACE_UNKNOWN flag to be
 * set in the result, beside recognized surface flags.
 *
 * Returns: The charset/surface pair.
 **/
EncaEncoding
enca_parse_encoding_name(const char *name)
{
  EncaEncoding enc = ENCODING_UNKNOWN;
  char *p, *q;

  if (name == NULL)
    return enc;

  p = enca_strdup(name);
  /* separate pure charset name into p */
  q = strchr(p, SURF_SEPARATOR);
  if (q != NULL)
    *q++ = '\0';
  enc.charset = enca_name_to_charset(p);
  /* surfaces, ony by one */
  while (q != NULL && enc.charset != ENCA_CS_UNKNOWN) {
    unsigned int surface;
    char *r = strchr(p, SURF_SEPARATOR);

    if (r != NULL)
      *r++ = '\0';
    enc.surface |= surface = enca_name_to_surface(q);
    q = r;
  }
  if (!check_surface_consistency(enc.surface))
    enc.surface |= ENCA_SURFACE_UNKNOWN;
  free(p);

  return enc;
}

/**
 * squeeze_compare:
 * @x: A string.
 * @y: Another string.
 *
 * Compares two strings taking into account only alphanumeric characters.
 *
 * Returns: Less than zero, more than zero, or zero, when the first string is
 *          squeeze-alphabeticaly before, after, or equal to second string.
 **/
static int
squeeze_compare(const char *x,
                const char *y)
{
  if (x == NULL || y == NULL) {
    if (x == NULL && y == NULL)
      return 0;

    if (x == NULL)
      return -1;
    else
      return 1;
  }

  while (*x != '\0' || *y != '\0') {
    while (*x != '\0' && !enca_isalnum(*x))
      x++;
    while (*y != '\0' && !enca_isalnum(*y))
      y++;

    if (enca_tolower(*x) != enca_tolower(*y))
      return (int)enca_tolower(*x) - (int)enca_tolower(*y);

    if (*x != '\0')
      x++;
    if (*y != '\0')
      y++;
  }
  return 0;
}

#if 0
/**
 * stable_compare:
 * @x: A string.
 * @y: Another string.
 *
 * Compares two strings taking into account only alphanumeric characters first.
 *
 * When the strings are equal, compares them normally, too.  Zero is thus
 * returned for really identical strings only.
 *
 * Returns: Less than zero, more than zero, or zero, when the first string is
 *          squeeze-alphabeticaly before, after, or equal to second string.
 **/
static int
stable_compare(const char *x,
               const char *y)
{
  int i;

  i = squeeze_compare(x, y);
  /* to stabilize the sort */
  if (i == 0)
    return strcmp(x, y);

  return i;
}
#endif

/**
 * alias_search:
 * @alist: Sorted array of strings.
 * @n: Size of @alist.
 * @s: String to find.
 *
 * Finds string @s in stable-sorted array of strings.
 *
 * Returns: Index of @s in @alist; -1 if not found.
 **/
static int
alias_search(const char *const *alist,
             int n,
             const char *s)
{
  int i1 = 0;
  int i2 = n-1;
  int i;

  i = squeeze_compare(s, alist[i1]);
  if (i < 0)
    return -1;
  if (i == 0)
    return i1;

  i = squeeze_compare(s, alist[i2]);
  if (i > 0)
    return -1;
  if (i == 0)
    return i2;

  while (i1+1 < i2) {
    int im = (i1 + i2)/2;

    i = squeeze_compare(s, alist[im]);
    if (i == 0)
      return im;

    if (i > 0)
      i1 = im;
    else
      i2 = im;
  }
  if (squeeze_compare(s, alist[i1+1]) == 0)
    return i1+1;

  return -1;
}

/**
 * enca_name_to_charset:
 * @csname: The charset name.
 *
 * Transforms charset name to numeric charset id.
 *
 * Returns: The charset id; #ENCA_CS_UNKNOWN when the name is not recognized.
 **/
int
enca_name_to_charset(const char *csname)
{
  int i;

  if (check_encoding_name(csname) <= 0)
    return ENCA_CS_UNKNOWN;

  i = alias_search(ALIAS_LIST, NALIASES, csname);
  return i < 0 ? ENCA_CS_UNKNOWN : INDEX_LIST[i];
}

/**
 * enca_name_to_surface:
 * @sname: The surface name.
 *
 * Transforms surface name to numeric surface id.
 *
 * Returns: The surface id; %ENCA_SURFACE_UNKNOWN when the name is not
 * recognized.
 **/
EncaSurface
enca_name_to_surface(const char *sname)
{
  size_t i;

  if (sname == NULL)
    return ENCA_SURFACE_UNKNOWN;

  for (i = 0; i < NSURFACES; i++) {
    if (SURFACE_INFO[i].enca == NULL || *(SURFACE_INFO[i].enca) == '\0')
      continue;
    if (squeeze_compare(SURFACE_INFO[i].enca, sname))
      return SURFACE_INFO[i].bit;
  }
  return ENCA_SURFACE_UNKNOWN;
}

/**
 * check_surface_consistency:
 * @surface: The surface.
 *
 * Checks whether the specified surface makes sense.  Unlike recode we don't
 * consider /cr/cr/crlf/cr/lf/lf/crlf a reasonable surface.
 *
 * Returns: Nonzero when the surface is OK, zero othewise.
 **/
static int
check_surface_consistency(EncaSurface surface)
{
  return count_bits((unsigned long int)surface & ENCA_SURFACE_MASK_EOL) <= 1
         && count_bits((unsigned long int)surface & ENCA_SURFACE_MASK_PERM) <= 1
         && (surface & ENCA_SURFACE_REMOVE) == 0
         && (surface & ENCA_SURFACE_UNKNOWN) == 0;
}

/**
 * count_bits:
 * @x: A flag field.
 *
 * Returns: The number of bits set in @x.
 **/
static int
count_bits(unsigned long int x)
{
  int i = 0;

  while (x != 0) {
    if (x & 1UL)
      i++;

    x >>= 1;
  }

  return i;
}

/**
 * check_encoding_name:
 * @name: A charset/surface/encoding name.
 *
 * Checks whether @name contains only allowed characters and counts the
 * number of alphanumeric characters in @name.
 *
 * Returns: The number of alphanumeric characters in @name; -1 when @name
 * is invalid.
 **/
static int
check_encoding_name(const char *name)
{
  size_t i, n;

  if (name == NULL)
    return -1;

  for (i = n = 0; name[i] != '\0'; i++) {
    if (!enca_isname(name[i]))
      return -1;

    if (enca_isalnum(name[i]))
      n++;
  }

  return n;
}

/***** Documentation *********************************************************/

/**
 * EncaSurface:
 * @ENCA_SURFACE_EOL_CR: End-of-lines are represented with CR's.
 * @ENCA_SURFACE_EOL_LF: End-of-lines are represented with LF's.
 * @ENCA_SURFACE_EOL_CRLF: End-of-lines are represented with CRLF's.
 * @ENCA_SURFACE_EOL_MIX: Several end-of-line types, mixed.
 * @ENCA_SURFACE_EOL_BIN: End-of-line concept not applicable (binary data).
 * @ENCA_SURFACE_MASK_EOL: Mask for end-of-line surfaces.
 * @ENCA_SURFACE_PERM_21: Odd and even bytes swapped.
 * @ENCA_SURFACE_PERM_4321: Reversed byte sequence in 4byte words.
 * @ENCA_SURFACE_PERM_MIX: Chunks with both endianess, concatenated.
 * @ENCA_SURFACE_MASK_PERM: Mask for permutation surfaces.
 * @ENCA_SURFACE_QP: Quoted printables.
 * @ENCA_SURFACE_HZ: HZ encoded.
 * @ENCA_SURFACE_REMOVE: Recode `remove' surface.
 * @ENCA_SURFACE_UNKNOWN: Unknown surface.
 * @ENCA_SURFACE_MASK_ALL: Mask for all bits, withnout #ENCA_SURFACE_UNKNOWN.
 *
 * Surface flags.
 **/

/**
 * EncaNameStyle:
 * @ENCA_NAME_STYLE_ENCA: Default, implicit charset name in Enca.
 * @ENCA_NAME_STYLE_RFC1345: RFC 1345 or otherwise canonical charset name.
 * @ENCA_NAME_STYLE_CSTOCS: Cstocs charset name (may not exist).
 * @ENCA_NAME_STYLE_ICONV: Iconv charset name (may not exist).
 * @ENCA_NAME_STYLE_HUMAN: Human comprehensible description.
 * @ENCA_NAME_STYLE_MIME: Preferred MIME name (may not exist).
 *
 * Charset naming styles and conventions.
 **/

/**
 * EncaCharsetFlags:
 * @ENCA_CHARSET_7BIT: Characters are represented with 7bit characters.
 * @ENCA_CHARSET_8BIT: Characters are represented with bytes.
 * @ENCA_CHARSET_16BIT: Characters are represented with 2byte words.
 * @ENCA_CHARSET_32BIT: Characters are represented with 4byte words.
 * @ENCA_CHARSET_FIXED: One characters consists of one fundamental piece.
 * @ENCA_CHARSET_VARIABLE: One character consists of variable number of
 * fundamental pieces.
 * @ENCA_CHARSET_BINARY: Charset is binary from ASCII viewpoint.
 * @ENCA_CHARSET_REGULAR: Language dependent (8bit) charset.
 * @ENCA_CHARSET_MULTIBYTE: Multibyte charset.
 *
 * Charset properties.
 *
 * Flags %ENCA_CHARSET_7BIT, %ENCA_CHARSET_8BIT, %ENCA_CHARSET_16BIT,
 * %ENCA_CHARSET_32BIT tell how many bits a `fundamental piece' consists of.
 * This is different from bits per character; r.g. UTF-8 consists of 8bit
 * pieces (bytes), but character can be composed from 1 to 6 of them.
 **/

/**
 * ENCA_CS_UNKNOWN:
 *
 * Unknown character set id.
 *
 * Use enca_charset_is_known() to check for unknown charset instead of direct
 * comparsion.
 **/

/**
 * EncaEncoding:
 * @charset: Numeric charset identifier.
 * @surface: Surface flags.
 *
 * Encoding, i.e. charset and surface.
 *
 * This is what enca_analyse() and enca_analyse_const() return.
 *
 * The @charset field is an opaque numerical charset identifier, which has no
 * meaning outside Enca library.
 * You will probably want to use it only as enca_charset_name() argument.
 * It is only guaranteed not to change meaning
 * during program execution time; change of its interpretation (e.g. due to
 * addition of new charsets) is not considered API change.
 *
 * The @surface field is a combination of #EncaSurface flags.  You may want
 * to ignore it completely; you should use enca_set_interpreted_surfaces()
 * to disable weird surfaces then.
 **/

/**
 * enca_charset_is_known:
 * @cs: Charset id.
 *
 * Expands to nonzero when the charset is known (i.e. it's not
 * ENCA_CS_UNKNOWN).
 **/

/**
 * enca_charset_is_7bit:
 * @cs: Charset id.
 *
 * Expands to nonzero when characters are represented with 7bit characters.
 **/

/**
 * enca_charset_is_8bit:
 * @cs: Charset id.
 *
 * Expands to nonzero when characters are represented with bytes.
 **/

/**
 * enca_charset_is_16bit:
 * @cs: Charset id.
 *
 * Expands to nonzero when characters are represented with 2byte words.
 **/

/**
 * enca_charset_is_32bit:
 * @cs: Charset id.
 *
 * Expands to nonzero when characters are represented with 4byte words.
 **/

/**
 * enca_charset_is_fixed:
 * @cs: Charset id.
 *
 * Expands to nonzero when one characters consists of one fundamental piece.
 **/

/**
 * enca_charset_is_variable:
 * @cs: Charset id.
 *
 * Expands to nonzero when one character consists of variable number of
 * fundamental pieces.
 **/

/**
 * enca_charset_is_binary:
 * @cs: Charset id.
 *
 * Expands to nonzero when charset is binary from ASCII viewpoint.
 **/

/**
 * enca_charset_is_regular:
 * @cs: Charset id.
 *
 * Expands to nonzero when charset is language dependent (8bit) charset.
 **/

/**
 * enca_charset_is_multibyte:
 * @cs: Charset id.
 *
 * Expands to nonzero when charset is multibyte.
 **/

/* vim: ts=2 sw=2 et
 */
