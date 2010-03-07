/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
  A better inplementation of the UNIX ctype(3) library.
  Notes:   my_global.h should be included before ctype.h
*/

#ifndef _m_ctype_h
#define _m_ctype_h

#include <my_attribute.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define MY_CS_NAME_SIZE			32
#define MY_CS_CTYPE_TABLE_SIZE		257
#define MY_CS_TO_LOWER_TABLE_SIZE	256
#define MY_CS_TO_UPPER_TABLE_SIZE	256
#define MY_CS_SORT_ORDER_TABLE_SIZE	256
#define MY_CS_TO_UNI_TABLE_SIZE		256

#define CHARSET_DIR	"charsets/"

#define my_wc_t ulong

typedef struct unicase_info_st
{
  uint16 toupper;
  uint16 tolower;
  uint16 sort;
} MY_UNICASE_INFO;


extern MY_UNICASE_INFO *my_unicase_default[256];
extern MY_UNICASE_INFO *my_unicase_turkish[256];

typedef struct uni_ctype_st
{
  uchar  pctype;
  uchar  *ctype;
} MY_UNI_CTYPE;

extern MY_UNI_CTYPE my_uni_ctype[256];

/* wm_wc and wc_mb return codes */
#define MY_CS_ILSEQ	0     /* Wrong by sequence: wb_wc                   */
#define MY_CS_ILUNI	0     /* Cannot encode Unicode to charset: wc_mb    */
#define MY_CS_TOOSMALL  -101  /* Need at least one byte:    wc_mb and mb_wc */
#define MY_CS_TOOSMALL2 -102  /* Need at least two bytes:   wc_mb and mb_wc */
#define MY_CS_TOOSMALL3 -103  /* Need at least three bytes: wc_mb and mb_wc */
/* These following three are currently not really used */
#define MY_CS_TOOSMALL4 -104  /* Need at least 4 bytes: wc_mb and mb_wc */
#define MY_CS_TOOSMALL5 -105  /* Need at least 5 bytes: wc_mb and mb_wc */
#define MY_CS_TOOSMALL6 -106  /* Need at least 6 bytes: wc_mb and mb_wc */
/* A helper macros for "need at least n bytes" */
#define MY_CS_TOOSMALLN(n)    (-100-(n))

#define MY_SEQ_INTTAIL	1
#define MY_SEQ_SPACES	2

        /* My charsets_list flags */
#define MY_CS_COMPILED  1      /* compiled-in sets               */
#define MY_CS_CONFIG    2      /* sets that have a *.conf file   */
#define MY_CS_INDEX     4      /* sets listed in the Index file  */
#define MY_CS_LOADED    8      /* sets that are currently loaded */
#define MY_CS_BINSORT	16     /* if binary sort order           */
#define MY_CS_PRIMARY	32     /* if primary collation           */
#define MY_CS_STRNXFRM	64     /* if strnxfrm is used for sort   */
#define MY_CS_UNICODE	128    /* is a charset is full unicode   */
#define MY_CS_READY	256    /* if a charset is initialized    */
#define MY_CS_AVAILABLE	512    /* If either compiled-in or loaded*/
#define MY_CS_CSSORT	1024   /* if case sensitive sort order   */	
#define MY_CS_HIDDEN	2048   /* don't display in SHOW          */	
#define MY_CS_PUREASCII 4096   /* if a charset is pure ascii     */
#define MY_CHARSET_UNDEFINED 0

/* Character repertoire flags */
#define MY_REPERTOIRE_ASCII      1 /* Pure ASCII            U+0000..U+007F */
#define MY_REPERTOIRE_EXTENDED   2 /* Extended characters:  U+0080..U+FFFF */
#define MY_REPERTOIRE_UNICODE30  3 /* ASCII | EXTENDED:     U+0000..U+FFFF */


typedef struct my_uni_idx_st
{
  uint16 from;
  uint16 to;
  uchar  *tab;
} MY_UNI_IDX;

typedef struct
{
  uint beg;
  uint end;
  uint mb_len;
} my_match_t;

enum my_lex_states
{
  MY_LEX_START, MY_LEX_CHAR, MY_LEX_IDENT, 
  MY_LEX_IDENT_SEP, MY_LEX_IDENT_START,
  MY_LEX_REAL, MY_LEX_HEX_NUMBER, MY_LEX_BIN_NUMBER,
  MY_LEX_CMP_OP, MY_LEX_LONG_CMP_OP, MY_LEX_STRING, MY_LEX_COMMENT, MY_LEX_END,
  MY_LEX_OPERATOR_OR_IDENT, MY_LEX_NUMBER_IDENT, MY_LEX_INT_OR_REAL,
  MY_LEX_REAL_OR_POINT, MY_LEX_BOOL, MY_LEX_EOL, MY_LEX_ESCAPE, 
  MY_LEX_LONG_COMMENT, MY_LEX_END_LONG_COMMENT, MY_LEX_SEMICOLON, 
  MY_LEX_SET_VAR, MY_LEX_USER_END, MY_LEX_HOSTNAME, MY_LEX_SKIP, 
  MY_LEX_USER_VARIABLE_DELIMITER, MY_LEX_SYSTEM_VAR,
  MY_LEX_IDENT_OR_KEYWORD,
  MY_LEX_IDENT_OR_HEX, MY_LEX_IDENT_OR_BIN, MY_LEX_IDENT_OR_NCHAR,
  MY_LEX_STRING_OR_DELIMITER
};

struct charset_info_st;


/* See strings/CHARSET_INFO.txt for information about this structure  */
typedef struct my_collation_handler_st
{
  my_bool (*init)(struct charset_info_st *, void *(*alloc)(size_t));
  /* Collation routines */
  int     (*strnncoll)(struct charset_info_st *,
		       const uchar *, size_t, const uchar *, size_t, my_bool);
  int     (*strnncollsp)(struct charset_info_st *,
                         const uchar *, size_t, const uchar *, size_t,
                         my_bool diff_if_only_endspace_difference);
  size_t     (*strnxfrm)(struct charset_info_st *,
                         uchar *, size_t, const uchar *, size_t);
  size_t    (*strnxfrmlen)(struct charset_info_st *, size_t); 
  my_bool (*like_range)(struct charset_info_st *,
			const char *s, size_t s_length,
			pchar w_prefix, pchar w_one, pchar w_many, 
			size_t res_length,
			char *min_str, char *max_str,
			size_t *min_len, size_t *max_len);
  int     (*wildcmp)(struct charset_info_st *,
  		     const char *str,const char *str_end,
                     const char *wildstr,const char *wildend,
                     int escape,int w_one, int w_many);

  int  (*strcasecmp)(struct charset_info_st *, const char *, const char *);
  
  uint (*instr)(struct charset_info_st *,
                const char *b, size_t b_length,
                const char *s, size_t s_length,
                my_match_t *match, uint nmatch);
  
  /* Hash calculation */
  void (*hash_sort)(struct charset_info_st *cs, const uchar *key, size_t len,
		    ulong *nr1, ulong *nr2); 
  my_bool (*propagate)(struct charset_info_st *cs, const uchar *str, size_t len);
} MY_COLLATION_HANDLER;

extern MY_COLLATION_HANDLER my_collation_mb_bin_handler;
extern MY_COLLATION_HANDLER my_collation_8bit_bin_handler;
extern MY_COLLATION_HANDLER my_collation_8bit_simple_ci_handler;
extern MY_COLLATION_HANDLER my_collation_ucs2_uca_handler;

/* Some typedef to make it easy for C++ to make function pointers */
typedef int (*my_charset_conv_mb_wc)(struct charset_info_st *, my_wc_t *,
                                     const uchar *, const uchar *);
typedef int (*my_charset_conv_wc_mb)(struct charset_info_st *, my_wc_t,
                                     uchar *, uchar *);
typedef size_t (*my_charset_conv_case)(struct charset_info_st *,
                                       char *, size_t, char *, size_t);


/* See strings/CHARSET_INFO.txt about information on this structure  */
typedef struct my_charset_handler_st
{
  my_bool (*init)(struct charset_info_st *, void *(*alloc)(size_t));
  /* Multibyte routines */
  uint    (*ismbchar)(struct charset_info_st *, const char *, const char *);
  uint    (*mbcharlen)(struct charset_info_st *, uint c);
  size_t  (*numchars)(struct charset_info_st *, const char *b, const char *e);
  size_t  (*charpos)(struct charset_info_st *, const char *b, const char *e,
                     size_t pos);
  size_t  (*well_formed_len)(struct charset_info_st *,
                             const char *b,const char *e,
                             size_t nchars, int *error);
  size_t  (*lengthsp)(struct charset_info_st *, const char *ptr, size_t length);
  size_t  (*numcells)(struct charset_info_st *, const char *b, const char *e);
  
  /* Unicode conversion */
  my_charset_conv_mb_wc mb_wc;
  my_charset_conv_wc_mb wc_mb;

  /* CTYPE scanner */
  int (*ctype)(struct charset_info_st *cs, int *ctype,
               const uchar *s, const uchar *e);
  
  /* Functions for case and sort conversion */
  size_t  (*caseup_str)(struct charset_info_st *, char *);
  size_t  (*casedn_str)(struct charset_info_st *, char *);

  my_charset_conv_case caseup;
  my_charset_conv_case casedn;

  /* Charset dependant snprintf() */
  size_t (*snprintf)(struct charset_info_st *, char *to, size_t n,
                     const char *fmt,
                     ...) ATTRIBUTE_FORMAT_FPTR(printf, 4, 5);
  size_t (*long10_to_str)(struct charset_info_st *, char *to, size_t n,
                          int radix, long int val);
  size_t (*longlong10_to_str)(struct charset_info_st *, char *to, size_t n,
                              int radix, longlong val);
  
  void (*fill)(struct charset_info_st *, char *to, size_t len, int fill);
  
  /* String-to-number conversion routines */
  long        (*strntol)(struct charset_info_st *, const char *s, size_t l,
			 int base, char **e, int *err);
  ulong      (*strntoul)(struct charset_info_st *, const char *s, size_t l,
			 int base, char **e, int *err);
  longlong   (*strntoll)(struct charset_info_st *, const char *s, size_t l,
			 int base, char **e, int *err);
  ulonglong (*strntoull)(struct charset_info_st *, const char *s, size_t l,
			 int base, char **e, int *err);
  double      (*strntod)(struct charset_info_st *, char *s, size_t l, char **e,
			 int *err);
  longlong    (*strtoll10)(struct charset_info_st *cs,
                           const char *nptr, char **endptr, int *error);
  ulonglong   (*strntoull10rnd)(struct charset_info_st *cs,
                                const char *str, size_t length,
                                int unsigned_fl,
                                char **endptr, int *error);
  size_t        (*scan)(struct charset_info_st *, const char *b, const char *e,
                        int sq);
} MY_CHARSET_HANDLER;

extern MY_CHARSET_HANDLER my_charset_8bit_handler;
extern MY_CHARSET_HANDLER my_charset_ucs2_handler;


/* See strings/CHARSET_INFO.txt about information on this structure  */
typedef struct charset_info_st
{
  uint      number;
  uint      primary_number;
  uint      binary_number;
  uint      state;
  const char *csname;
  const char *name;
  const char *comment;
  const char *tailoring;
  uchar    *ctype;
  uchar    *to_lower;
  uchar    *to_upper;
  uchar    *sort_order;
  uint16   *contractions;
  uint16   **sort_order_big;
  uint16      *tab_to_uni;
  MY_UNI_IDX  *tab_from_uni;
  MY_UNICASE_INFO **caseinfo;
  uchar     *state_map;
  uchar     *ident_map;
  uint      strxfrm_multiply;
  uchar     caseup_multiply;
  uchar     casedn_multiply;
  uint      mbminlen;
  uint      mbmaxlen;
  uint16    min_sort_char;
  uint16    max_sort_char; /* For LIKE optimization */
  uchar     pad_char;
  my_bool   escape_with_backslash_is_dangerous;
  
  MY_CHARSET_HANDLER *cset;
  MY_COLLATION_HANDLER *coll;
  
} CHARSET_INFO;
#define ILLEGAL_CHARSET_INFO_NUMBER (~0U)


extern MYSQL_PLUGIN_IMPORT CHARSET_INFO my_charset_bin;
extern CHARSET_INFO my_charset_big5_chinese_ci;
extern CHARSET_INFO my_charset_big5_bin;
extern CHARSET_INFO my_charset_cp932_japanese_ci;
extern CHARSET_INFO my_charset_cp932_bin;
extern CHARSET_INFO my_charset_eucjpms_japanese_ci;
extern CHARSET_INFO my_charset_eucjpms_bin;
extern CHARSET_INFO my_charset_euckr_korean_ci;
extern CHARSET_INFO my_charset_euckr_bin;
extern CHARSET_INFO my_charset_gb2312_chinese_ci;
extern CHARSET_INFO my_charset_gb2312_bin;
extern CHARSET_INFO my_charset_gbk_chinese_ci;
extern CHARSET_INFO my_charset_gbk_bin;
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO my_charset_latin1;
extern CHARSET_INFO my_charset_latin1_german2_ci;
extern CHARSET_INFO my_charset_latin1_bin;
extern CHARSET_INFO my_charset_latin2_czech_ci;
extern CHARSET_INFO my_charset_sjis_japanese_ci;
extern CHARSET_INFO my_charset_sjis_bin;
extern CHARSET_INFO my_charset_tis620_thai_ci;
extern CHARSET_INFO my_charset_tis620_bin;
extern CHARSET_INFO my_charset_ucs2_general_ci;
extern CHARSET_INFO my_charset_ucs2_bin;
extern CHARSET_INFO my_charset_ucs2_unicode_ci;
extern CHARSET_INFO my_charset_ujis_japanese_ci;
extern CHARSET_INFO my_charset_ujis_bin;
extern CHARSET_INFO my_charset_utf8_general_ci;
extern CHARSET_INFO my_charset_utf8_unicode_ci;
extern CHARSET_INFO my_charset_utf8_bin;
extern CHARSET_INFO my_charset_cp1250_czech_ci;
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO my_charset_filename;

/* declarations for simple charsets */
extern size_t my_strnxfrm_simple(CHARSET_INFO *, uchar *, size_t,
                                 const uchar *, size_t); 
size_t  my_strnxfrmlen_simple(CHARSET_INFO *, size_t); 
extern int  my_strnncoll_simple(CHARSET_INFO *, const uchar *, size_t,
				const uchar *, size_t, my_bool);

extern int  my_strnncollsp_simple(CHARSET_INFO *, const uchar *, size_t,
                                  const uchar *, size_t,
                                  my_bool diff_if_only_endspace_difference);

extern void my_hash_sort_simple(CHARSET_INFO *cs,
				const uchar *key, size_t len,
				ulong *nr1, ulong *nr2); 

extern size_t my_lengthsp_8bit(CHARSET_INFO *cs, const char *ptr, size_t length);

extern uint my_instr_simple(struct charset_info_st *,
                            const char *b, size_t b_length,
                            const char *s, size_t s_length,
                            my_match_t *match, uint nmatch);


/* Functions for 8bit */
extern size_t my_caseup_str_8bit(CHARSET_INFO *, char *);
extern size_t my_casedn_str_8bit(CHARSET_INFO *, char *);
extern size_t my_caseup_8bit(CHARSET_INFO *, char *src, size_t srclen,
                             char *dst, size_t dstlen);
extern size_t my_casedn_8bit(CHARSET_INFO *, char *src, size_t srclen,
                             char *dst, size_t dstlen);

extern int my_strcasecmp_8bit(CHARSET_INFO * cs, const char *, const char *);

int my_mb_wc_8bit(CHARSET_INFO *cs,my_wc_t *wc, const uchar *s,const uchar *e);
int my_wc_mb_8bit(CHARSET_INFO *cs,my_wc_t wc, uchar *s, uchar *e);

int my_mb_ctype_8bit(CHARSET_INFO *,int *, const uchar *,const uchar *);
int my_mb_ctype_mb(CHARSET_INFO *,int *, const uchar *,const uchar *);

size_t my_scan_8bit(CHARSET_INFO *cs, const char *b, const char *e, int sq);

size_t my_snprintf_8bit(struct charset_info_st *, char *to, size_t n,
                        const char *fmt, ...)
  ATTRIBUTE_FORMAT(printf, 4, 5);

long       my_strntol_8bit(CHARSET_INFO *, const char *s, size_t l, int base,
                           char **e, int *err);
ulong      my_strntoul_8bit(CHARSET_INFO *, const char *s, size_t l, int base,
			    char **e, int *err);
longlong   my_strntoll_8bit(CHARSET_INFO *, const char *s, size_t l, int base,
			    char **e, int *err);
ulonglong my_strntoull_8bit(CHARSET_INFO *, const char *s, size_t l, int base,
			    char **e, int *err);
double      my_strntod_8bit(CHARSET_INFO *, char *s, size_t l,char **e,
			    int *err);
size_t my_long10_to_str_8bit(CHARSET_INFO *, char *to, size_t l, int radix,
                             long int val);
size_t my_longlong10_to_str_8bit(CHARSET_INFO *, char *to, size_t l, int radix,
                                 longlong val);

longlong my_strtoll10_8bit(CHARSET_INFO *cs,
                           const char *nptr, char **endptr, int *error);
longlong my_strtoll10_ucs2(CHARSET_INFO *cs, 
                           const char *nptr, char **endptr, int *error);

ulonglong my_strntoull10rnd_8bit(CHARSET_INFO *cs,
                                 const char *str, size_t length, int
                                 unsigned_fl, char **endptr, int *error);
ulonglong my_strntoull10rnd_ucs2(CHARSET_INFO *cs, 
                                 const char *str, size_t length,
                                 int unsigned_fl, char **endptr, int *error);

void my_fill_8bit(CHARSET_INFO *cs, char* to, size_t l, int fill);

my_bool  my_like_range_simple(CHARSET_INFO *cs,
			      const char *ptr, size_t ptr_length,
			      pbool escape, pbool w_one, pbool w_many,
			      size_t res_length,
			      char *min_str, char *max_str,
			      size_t *min_length, size_t *max_length);

my_bool  my_like_range_mb(CHARSET_INFO *cs,
			  const char *ptr, size_t ptr_length,
			  pbool escape, pbool w_one, pbool w_many,
			  size_t res_length,
			  char *min_str, char *max_str,
			  size_t *min_length, size_t *max_length);

my_bool  my_like_range_ucs2(CHARSET_INFO *cs,
			    const char *ptr, size_t ptr_length,
			    pbool escape, pbool w_one, pbool w_many,
			    size_t res_length,
			    char *min_str, char *max_str,
			    size_t *min_length, size_t *max_length);


int my_wildcmp_8bit(CHARSET_INFO *,
		    const char *str,const char *str_end,
		    const char *wildstr,const char *wildend,
		    int escape, int w_one, int w_many);

int my_wildcmp_bin(CHARSET_INFO *,
		   const char *str,const char *str_end,
		   const char *wildstr,const char *wildend,
		   int escape, int w_one, int w_many);

size_t my_numchars_8bit(CHARSET_INFO *, const char *b, const char *e);
size_t my_numcells_8bit(CHARSET_INFO *, const char *b, const char *e);
size_t my_charpos_8bit(CHARSET_INFO *, const char *b, const char *e, size_t pos);
size_t my_well_formed_len_8bit(CHARSET_INFO *, const char *b, const char *e,
                             size_t pos, int *error);
uint my_mbcharlen_8bit(CHARSET_INFO *, uint c);


/* Functions for multibyte charsets */
extern size_t my_caseup_str_mb(CHARSET_INFO *, char *);
extern size_t my_casedn_str_mb(CHARSET_INFO *, char *);
extern size_t my_caseup_mb(CHARSET_INFO *, char *src, size_t srclen,
                                         char *dst, size_t dstlen);
extern size_t my_casedn_mb(CHARSET_INFO *, char *src, size_t srclen,
                                         char *dst, size_t dstlen);
extern int my_strcasecmp_mb(CHARSET_INFO * cs,const char *, const char *);

int my_wildcmp_mb(CHARSET_INFO *,
		  const char *str,const char *str_end,
		  const char *wildstr,const char *wildend,
		  int escape, int w_one, int w_many);
size_t my_numchars_mb(CHARSET_INFO *, const char *b, const char *e);
size_t my_numcells_mb(CHARSET_INFO *, const char *b, const char *e);
size_t my_charpos_mb(CHARSET_INFO *, const char *b, const char *e, size_t pos);
size_t my_well_formed_len_mb(CHARSET_INFO *, const char *b, const char *e,
                             size_t pos, int *error);
uint my_instr_mb(struct charset_info_st *,
                 const char *b, size_t b_length,
                 const char *s, size_t s_length,
                 my_match_t *match, uint nmatch);

int my_wildcmp_unicode(CHARSET_INFO *cs,
                       const char *str, const char *str_end,
                       const char *wildstr, const char *wildend,
                       int escape, int w_one, int w_many,
                       MY_UNICASE_INFO **weights);

extern my_bool my_parse_charset_xml(const char *bug, size_t len,
				    int (*add)(CHARSET_INFO *cs));
extern char *my_strchr(CHARSET_INFO *cs, const char *str, const char *end,
                       pchar c);

my_bool my_propagate_simple(CHARSET_INFO *cs, const uchar *str, size_t len);
my_bool my_propagate_complex(CHARSET_INFO *cs, const uchar *str, size_t len);


uint my_string_repertoire(CHARSET_INFO *cs, const char *str, ulong len);
my_bool my_charset_is_ascii_based(CHARSET_INFO *cs);
my_bool my_charset_is_8bit_pure_ascii(CHARSET_INFO *cs);
uint my_charset_repertoire(CHARSET_INFO *cs);


#define	_MY_U	01	/* Upper case */
#define	_MY_L	02	/* Lower case */
#define	_MY_NMR	04	/* Numeral (digit) */
#define	_MY_SPC	010	/* Spacing character */
#define	_MY_PNT	020	/* Punctuation */
#define	_MY_CTR	040	/* Control character */
#define	_MY_B	0100	/* Blank */
#define	_MY_X	0200	/* heXadecimal digit */


#define	my_isascii(c)	(!((c) & ~0177))
#define	my_toascii(c)	((c) & 0177)
#define my_tocntrl(c)	((c) & 31)
#define my_toprint(c)	((c) | 64)
#define my_toupper(s,c)	(char) ((s)->to_upper[(uchar) (c)])
#define my_tolower(s,c)	(char) ((s)->to_lower[(uchar) (c)])
#define	my_isalpha(s, c)  (((s)->ctype+1)[(uchar) (c)] & (_MY_U | _MY_L))
#define	my_isupper(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_U)
#define	my_islower(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_L)
#define	my_isdigit(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_NMR)
#define	my_isxdigit(s, c) (((s)->ctype+1)[(uchar) (c)] & _MY_X)
#define	my_isalnum(s, c)  (((s)->ctype+1)[(uchar) (c)] & (_MY_U | _MY_L | _MY_NMR))
#define	my_isspace(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_SPC)
#define	my_ispunct(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_PNT)
#define	my_isprint(s, c)  (((s)->ctype+1)[(uchar) (c)] & (_MY_PNT | _MY_U | _MY_L | _MY_NMR | _MY_B))
#define	my_isgraph(s, c)  (((s)->ctype+1)[(uchar) (c)] & (_MY_PNT | _MY_U | _MY_L | _MY_NMR))
#define	my_iscntrl(s, c)  (((s)->ctype+1)[(uchar) (c)] & _MY_CTR)

/* Some macros that should be cleaned up a little */
#define my_isvar(s,c)                 (my_isalnum(s,c) || (c) == '_')
#define my_isvar_start(s,c)           (my_isalpha(s,c) || (c) == '_')

#define my_binary_compare(s)	      ((s)->state  & MY_CS_BINSORT)
#define use_strnxfrm(s)               ((s)->state  & MY_CS_STRNXFRM)
#define my_strnxfrm(s, a, b, c, d)    ((s)->coll->strnxfrm((s), (a), (b), (c), (d)))
#define my_strnncoll(s, a, b, c, d) ((s)->coll->strnncoll((s), (a), (b), (c), (d), 0))
#define my_like_range(s, a, b, c, d, e, f, g, h, i, j) \
   ((s)->coll->like_range((s), (a), (b), (c), (d), (e), (f), (g), (h), (i), (j)))
#define my_wildcmp(cs,s,se,w,we,e,o,m) ((cs)->coll->wildcmp((cs),(s),(se),(w),(we),(e),(o),(m)))
#define my_strcasecmp(s, a, b)        ((s)->coll->strcasecmp((s), (a), (b)))
#define my_charpos(cs, b, e, num)     (cs)->cset->charpos((cs), (const char*) (b), (const char *)(e), (num))


#define use_mb(s)                     ((s)->cset->ismbchar != NULL)
#define my_ismbchar(s, a, b)          ((s)->cset->ismbchar((s), (a), (b)))
#ifdef USE_MB
#define my_mbcharlen(s, a)            ((s)->cset->mbcharlen((s),(a)))
#else
#define my_mbcharlen(s, a)            1
#endif

#define my_caseup_str(s, a)           ((s)->cset->caseup_str((s), (a)))
#define my_casedn_str(s, a)           ((s)->cset->casedn_str((s), (a)))
#define my_strntol(s, a, b, c, d, e)  ((s)->cset->strntol((s),(a),(b),(c),(d),(e)))
#define my_strntoul(s, a, b, c, d, e) ((s)->cset->strntoul((s),(a),(b),(c),(d),(e)))
#define my_strntoll(s, a, b, c, d, e) ((s)->cset->strntoll((s),(a),(b),(c),(d),(e)))
#define my_strntoull(s, a, b, c,d, e) ((s)->cset->strntoull((s),(a),(b),(c),(d),(e)))
#define my_strntod(s, a, b, c, d)     ((s)->cset->strntod((s),(a),(b),(c),(d)))


/* XXX: still need to take care of this one */
#ifdef MY_CHARSET_TIS620
#error The TIS620 charset is broken at the moment.  Tell tim to fix it.
#define USE_TIS620
#include "t_ctype.h"
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _m_ctype_h */
