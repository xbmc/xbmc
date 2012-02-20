/* C code produced by gperf version 3.0.1 */
/* Command-line: gperf -tCcTonD -K id -N id3_compat_lookup -s -3 -k 1,2,3,4 compat.gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "compat.gperf"

/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: compat.gperf,v 1.11 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>
# include <string.h>

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# include "id3tag.h"
# include "compat.h"
# include "frame.h"
# include "field.h"
# include "parse.h"
# include "ucs4.h"

# define EQ(id)    #id, 0
# define OBSOLETE    0, 0
# define TX(id)    #id, translate_##id

static id3_compat_func_t translate_TCON;
static id3_compat_func_t translate_APIC;

#define TOTAL_KEYWORDS 75
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 4
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 107
/* maximum key range = 107, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108,   5, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108,  74,
       69,  62,  25, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108,  10,   5,   3,   1,   6,
       24,  59, 108,   0,  63,  25,  52,   0,  54,   1,
       10,  14,  13,  30,   5,  18,   5,  30,  15,  62,
       47, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
      108, 108, 108, 108, 108, 108, 108
    };
  register int hval = 0;

  switch (len)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]+1];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct id3_compat *
id3_compat_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct id3_compat wordlist[] =
    {
#line 73 "compat.gperf"
      {"MCI",  EQ(MCDI)  /* Music CD identifier */},
#line 85 "compat.gperf"
      {"TCM",  EQ(TCOM)  /* Composer */},
#line 86 "compat.gperf"
      {"TCO",  TX(TCON)  /* Content type */},
#line 62 "compat.gperf"
      {"CNT",  EQ(PCNT)  /* Play counter */},
#line 117 "compat.gperf"
      {"TSI",  OBSOLETE  /* Size [obsolete] */},
#line 99 "compat.gperf"
      {"TLE",  EQ(TLEN)  /* Length */},
#line 63 "compat.gperf"
      {"COM",  EQ(COMM)  /* Comments */},
#line 98 "compat.gperf"
      {"TLA",  EQ(TLAN)  /* Language(s) */},
#line 88 "compat.gperf"
      {"TCP",  EQ(TCMP)  /* Stupid iTunes making up their own tags (compilation) */},
#line 84 "compat.gperf"
      {"TBP",  EQ(TBPM)  /* BPM (beats per minute) */},
#line 89 "compat.gperf"
      {"TCR",  EQ(TCOP)  /* Copyright message */},
#line 106 "compat.gperf"
      {"TOT",  EQ(TOAL)  /* Original album/movie/show title */},
#line 90 "compat.gperf"
      {"TDA",  OBSOLETE  /* Date [obsolete] */},
#line 128 "compat.gperf"
      {"ULT",  EQ(USLT)  /* Unsynchronised lyric/text transcription */},
#line 112 "compat.gperf"
      {"TPB",  EQ(TPUB)  /* Publisher */},
#line 101 "compat.gperf"
      {"TOA",  EQ(TOPE)  /* Original artist(s)/performer(s) */},
#line 91 "compat.gperf"
      {"TDAT", OBSOLETE  /* Date [obsolete] */},
#line 68 "compat.gperf"
      {"ETC",  EQ(ETCO)  /* Event timing codes */},
#line 104 "compat.gperf"
      {"TOR",  EQ(TDOR)  /* Original release year [obsolete] */},
#line 111 "compat.gperf"
      {"TPA",  EQ(TPOS)  /* Part of a set */},
#line 76 "compat.gperf"
      {"POP",  EQ(POPM)  /* Popularimeter */},
#line 132 "compat.gperf"
      {"WCM",  EQ(WCOM)  /* Commercial information */},
#line 65 "compat.gperf"
      {"CRM",  OBSOLETE  /* Encrypted meta frame [obsolete] */},
#line 61 "compat.gperf"
      {"BUF",  EQ(RBUF)  /* Recommended buffer size */},
#line 81 "compat.gperf"
      {"SLT",  EQ(SYLT)  /* Synchronised lyric/text */},
#line 114 "compat.gperf"
      {"TRD",  OBSOLETE  /* Recording dates [obsolete] */},
#line 66 "compat.gperf"
      {"EQU",  OBSOLETE  /* Equalization [obsolete] */},
#line 113 "compat.gperf"
      {"TRC",  EQ(TSRC)  /* ISRC (international standard recording code) */},
#line 102 "compat.gperf"
      {"TOF",  EQ(TOFN)  /* Original filename */},
#line 119 "compat.gperf"
      {"TSS",  EQ(TSSE)  /* Software/hardware and settings used for encoding */},
#line 133 "compat.gperf"
      {"WCP",  EQ(WCOP)  /* Copyright/legal information */},
#line 78 "compat.gperf"
      {"REV",  EQ(RVRB)  /* Reverb */},
#line 64 "compat.gperf"
      {"CRA",  EQ(AENC)  /* Audio encryption */},
#line 110 "compat.gperf"
      {"TP4",  EQ(TPE4)  /* Interpreted, remixed, or otherwise modified by */},
#line 77 "compat.gperf"
      {"PRI ", EQ(PRIV)  /* Private field (added by WMP) */},
#line 115 "compat.gperf"
      {"TRDA", OBSOLETE  /* Recording dates [obsolete] */},
#line 67 "compat.gperf"
      {"EQUA", OBSOLETE  /* Equalization [obsolete] */},
#line 130 "compat.gperf"
      {"WAR",  EQ(WOAR)  /* Official artist/performer webpage */},
#line 134 "compat.gperf"
      {"WPB",  EQ(WPUB)  /* Publishers official webpage */},
#line 82 "compat.gperf"
      {"STC",  EQ(SYTC)  /* Synchronised tempo codes */},
#line 74 "compat.gperf"
      {"MLL",  EQ(MLLT)  /* MPEG location lookup table */},
#line 79 "compat.gperf"
      {"RVA",  OBSOLETE  /* Relative volume adjustment [obsolete] */},
#line 80 "compat.gperf"
      {"RVAD", OBSOLETE  /* Relative volume adjustment [obsolete] */},
#line 118 "compat.gperf"
      {"TSIZ", OBSOLETE  /* Size [obsolete] */},
#line 125 "compat.gperf"
      {"TYE",  OBSOLETE  /* Year [obsolete] */},
#line 129 "compat.gperf"
      {"WAF",  EQ(WOAF)  /* Official audio file webpage */},
#line 116 "compat.gperf"
      {"TRK",  EQ(TRCK)  /* Track number/position in set */},
#line 87 "compat.gperf"
      {"TCON", TX(TCON)  /* Content type */},
#line 83 "compat.gperf"
      {"TAL",  EQ(TALB)  /* Album/movie/show title */},
#line 97 "compat.gperf"
      {"TKE",  EQ(TKEY)  /* Initial key */},
#line 100 "compat.gperf"
      {"TMT",  EQ(TMED)  /* Media type */},
#line 131 "compat.gperf"
      {"WAS",  EQ(WOAS)  /* Official audio source webpage */},
#line 70 "compat.gperf"
      {"IPL",  EQ(TIPL)  /* Involved people list */},
#line 103 "compat.gperf"
      {"TOL",  EQ(TOLY)  /* Original lyricist(s)/text writer(s) */},
#line 95 "compat.gperf"
      {"TIM",  OBSOLETE  /* Time [obsolete] */},
#line 94 "compat.gperf"
      {"TFT",  EQ(TFLT)  /* File type */},
#line 126 "compat.gperf"
      {"TYER", OBSOLETE  /* Year [obsolete] */},
#line 123 "compat.gperf"
      {"TXT",  EQ(TEXT)  /* Lyricist/text writer */},
#line 92 "compat.gperf"
      {"TDY",  EQ(TDLY)  /* Playlist delay */},
#line 96 "compat.gperf"
      {"TIME", OBSOLETE  /* Time [obsolete] */},
#line 75 "compat.gperf"
      {"PIC",  TX(APIC)  /* Attached picture */},
#line 127 "compat.gperf"
      {"UFI",  EQ(UFID)  /* Unique file identifier */},
#line 72 "compat.gperf"
      {"LNK",  EQ(LINK)  /* Linked information */},
#line 109 "compat.gperf"
      {"TP3",  EQ(TPE3)  /* Conductor/performer refinement */},
#line 124 "compat.gperf"
      {"TXX",  EQ(TXXX)  /* User defined text information frame */},
#line 93 "compat.gperf"
      {"TEN",  EQ(TENC)  /* Encoded by */},
#line 69 "compat.gperf"
      {"GEO",  EQ(GEOB)  /* General encapsulated object */},
#line 122 "compat.gperf"
      {"TT3",  EQ(TIT3)  /* Subtitle/description refinement */},
#line 108 "compat.gperf"
      {"TP2",  EQ(TPE2)  /* Band/orchestra/accompaniment */},
#line 105 "compat.gperf"
      {"TORY", EQ(TDOR)  /* Original release year [obsolete] */},
#line 121 "compat.gperf"
      {"TT2",  EQ(TIT2)  /* Title/songname/content description */},
#line 107 "compat.gperf"
      {"TP1",  EQ(TPE1)  /* Lead performer(s)/soloist(s) */},
#line 71 "compat.gperf"
      {"IPLS", EQ(TIPL)  /* Involved people list */},
#line 120 "compat.gperf"
      {"TT1",  EQ(TIT1)  /* Content group description */},
#line 135 "compat.gperf"
      {"WXX",  EQ(WXXX)  /* User defined URL link frame */}
    };

  static const short lookup[] =
    {
      -1,  0, -1, -1, -1, -1,  1,  2, -1,  3,  4,  5, -1,  6,
      -1,  7,  8, -1,  9, 10, 11, 12, -1, 13, 14, 15, 16, 17,
      18, 19, 20, 21, -1, 22, 23, 24, 25, 26, 27, 28, 29, 30,
      31, 32, 33, 34, 35, 36, 37, 38, -1, 39, 40, 41, 42, -1,
      -1, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
      -1, 56, 57, 58, 59, -1, 60, 61, 62, -1, -1, 63, 64, 65,
      66, 67, -1, -1, 68, -1, 69, -1, 70, 71, -1, -1, 72, 73,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 74
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].id;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
#line 136 "compat.gperf"


static
int translate_APIC(struct id3_frame *frame, char const *oldid,
		   id3_byte_t const *data, id3_length_t length)
{
  id3_byte_t const *end;
  char type[4];
  enum id3_field_textencoding encoding;
  int result = 0;
  
  encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

  end = data + length;

  /* Text encoding */
  if (id3_field_parse(&frame->fields[0], &data, end - data, &encoding) == -1)
    goto fail;

  /* Image format */
  id3_parse_immediate(&data, 3, type);
  if (type[0] == 'P' && type[1] == 'N' && type[2] == 'G') {
    if (id3_field_setlatin1(&frame->fields[1], "image/png") == -1)
      goto fail;
  }
  else if (type[0] == 'J' && type[1] == 'P' && type[2] == 'G') {
    if (id3_field_setlatin1(&frame->fields[1], "image/jpeg") == -1)
      goto fail;
  }
  else
    goto fail;

  /* Picture type */
  if (id3_field_parse(&frame->fields[2], &data, end - data, &encoding) == -1)
    goto fail;

  /* Description */
  if (id3_field_parse(&frame->fields[3], &data, end - data, &encoding) == -1)
    goto fail;

  /* Picture data */ 
  if (id3_field_parse(&frame->fields[4], &data, end - data, &encoding) == -1)
    goto fail;
  
  if (0) {
  fail:
    result = -1;
  }

  return result;
}

static
int translate_TCON(struct id3_frame *frame, char const *oldid,
		   id3_byte_t const *data, id3_length_t length)
{
  id3_byte_t const *end;
  enum id3_field_textencoding encoding;
  id3_ucs4_t *string = 0, *ptr, *endptr;
  int result = 0;

  /* translate old TCON syntax into multiple strings */

  assert(frame->nfields == 2);

  encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

  end = data + length;

  if (id3_field_parse(&frame->fields[0], &data, end - data, &encoding) == -1)
    goto fail;

  string = id3_parse_string(&data, end - data, encoding, 0);
  if (string == 0)
    goto fail;

  ptr = string;
  while (*ptr == '(') {
    if (*++ptr == '(')
      break;

    endptr = ptr;
    while (*endptr && *endptr != ')')
      ++endptr;

    if (*endptr)
      *endptr++ = 0;

    if (id3_field_addstring(&frame->fields[1], ptr) == -1)
      goto fail;

    ptr = endptr;
  }

  if (*ptr && id3_field_addstring(&frame->fields[1], ptr) == -1)
    goto fail;

  if (0) {
  fail:
    result = -1;
  }

  if (string)
    free(string);

  return result;
}

/*
 * NAME:	compat->fixup()
 * DESCRIPTION:	finish compatibility translations
 */
int id3_compat_fixup(struct id3_tag *tag)
{
  struct id3_frame *frame;
  unsigned int index;
  id3_ucs4_t timestamp[17] = { 0 };
  int result = 0;

  /* create a TDRC frame from obsolete TYER/TDAT/TIME frames */

  /*
   * TYE/TYER: YYYY
   * TDA/TDAT: DDMM
   * TIM/TIME: HHMM
   *
   * TDRC: yyyy-MM-ddTHH:mm
   */

  index = 0;
  while ((frame = id3_tag_findframe(tag, ID3_FRAME_OBSOLETE, index++))) {
    char const *id;
    id3_byte_t const *data, *end;
    id3_length_t length;
    enum id3_field_textencoding encoding;
    id3_ucs4_t *string;

    id = id3_field_getframeid(&frame->fields[0]);
    assert(id);

    if (strcmp(id, "TYER") != 0 && strcmp(id, "YTYE") != 0 &&
	strcmp(id, "TDAT") != 0 && strcmp(id, "YTDA") != 0 &&
	strcmp(id, "TIME") != 0 && strcmp(id, "YTIM") != 0)
      continue;

    data = id3_field_getbinarydata(&frame->fields[1], &length);
    assert(data);

    if (length < 1)
      continue;

    end = data + length;

    encoding = id3_parse_uint(&data, 1);
    string   = id3_parse_string(&data, end - data, encoding, 0);

    if (id3_ucs4_length(string) < 4) {
      free(string);
      continue;
    }

    if (strcmp(id, "TYER") == 0 ||
	strcmp(id, "YTYE") == 0) {
      timestamp[0] = string[0];
      timestamp[1] = string[1];
      timestamp[2] = string[2];
      timestamp[3] = string[3];
    }
    else if (strcmp(id, "TDAT") == 0 ||
	     strcmp(id, "YTDA") == 0) {
      timestamp[4] = '-';
      timestamp[5] = string[2];
      timestamp[6] = string[3];
      timestamp[7] = '-';
      timestamp[8] = string[0];
      timestamp[9] = string[1];
    }
    else {  /* TIME or YTIM */
      timestamp[10] = 'T';
      timestamp[11] = string[0];
      timestamp[12] = string[1];
      timestamp[13] = ':';
      timestamp[14] = string[2];
      timestamp[15] = string[3];
    }

    free(string);
  }

  if (timestamp[0]) {
    id3_ucs4_t *strings;

    frame = id3_frame_new("TDRC");
    if (frame == 0)
      goto fail;

    strings = timestamp;

    if (id3_field_settextencoding(&frame->fields[0],
				  ID3_FIELD_TEXTENCODING_ISO_8859_1) == -1 ||
	id3_field_setstrings(&frame->fields[1], 1, &strings) == -1 ||
	id3_tag_attachframe(tag, frame) == -1) {
      id3_frame_delete(frame);
      goto fail;
    }
  }

  if (0) {
  fail:
    result = -1;
  }

  return result;
}
