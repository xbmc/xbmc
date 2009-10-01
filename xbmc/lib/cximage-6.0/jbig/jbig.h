/*
 *  Header file for the portable free JBIG compression library
 *
 *  Markus Kuhn -- http://www.cl.cam.ac.uk/~mgk25/
 *
 *  $Id: jbig.h,v 1.17 2004-06-11 15:18:21+01 mgk25 Exp $
 */

#ifndef JBG_H
#define JBG_H

#include <stddef.h>

/*
 * JBIG-KIT version number
 */

#define JBG_VERSION    "1.6"

/*
 * Buffer block for SDEs which are temporarily stored by encoder
 */

#define JBG_BUFSIZE 4000

struct jbg_buf {
  unsigned char d[JBG_BUFSIZE];              /* one block of a buffer list */
  int len;                             /* length of the data in this block */
  struct jbg_buf *next;                           /* pointer to next block */
  struct jbg_buf *previous;                   /* pointer to previous block *
					       * (unused in freelist)      */
  struct jbg_buf *last;     /* only used in list head: final block of list */
  struct jbg_buf **free_list;   /* pointer to pointer to head of free list */
};

/*
 * Maximum number of ATMOVEs per stripe that decoder can handle
 */

#define JBG_ATMOVES_MAX  64

/*
 * Option and order flags
 */

#define JBG_HITOLO     0x08
#define JBG_SEQ        0x04
#define JBG_ILEAVE     0x02
#define JBG_SMID       0x01

#define JBG_LRLTWO     0x40
#define JBG_VLENGTH    0x20
#define JBG_TPDON      0x10
#define JBG_TPBON      0x08
#define JBG_DPON       0x04
#define JBG_DPPRIV     0x02
#define JBG_DPLAST     0x01

#define JBG_DELAY_AT   0x100  /* delay ATMOVE until the first line of the next
			       * stripe. Option available for compatibility
			       * with conformance test example in clause 7.2.*/


/*
 * Possible error code return values
 */

#define JBG_EOK        0
#define JBG_EOK_INTR   1
#define JBG_EAGAIN     2
#define JBG_ENOMEM     3
#define JBG_EABORT     4
#define JBG_EMARKER    5
#define JBG_ENOCONT    6
#define JBG_EINVAL     7
#define JBG_EIMPL      8

/*
 * Language code for error message strings (based on ISO 639 2-letter
 * standard language name abbreviations).
 */ 

#define JBG_EN         0        /* English */
#define JBG_DE_8859_1  1        /* German in ISO Latin 1 character set */
#define JBG_DE_UTF_8   2        /* German in Unicode UTF-8 encoding */

/*
 * Status description of an arithmetic encoder
 */

struct jbg_arenc_state {
  unsigned char st[4096];    /* probability status for contexts, MSB = MPS */
  unsigned long c;                /* C register, base of coding intervall, *
                                   * layout as in Table 23                 */
  unsigned long a;      /* A register, normalized size of coding intervall */
  long sc;        /* counter for buffered 0xff values which might overflow */
  int ct;  /* bit shift counter, determines when next byte will be written */
  int buffer;                /* buffer for most recent output byte != 0xff */
  void (*byte_out)(int, void *); /* function which receives all PSCD bytes */
  void *file;                              /* parameter passed to byte_out */
};


/*
 * Status description of an arithmetic decoder
 */

enum jbg_ardec_result {
  JBG_OK,                          /* symbol has been successfully decoded */
  JBG_READY,               /* no more bytes of this PSCD required, marker  *
			    * encountered, probably more symbols available */
  JBG_MORE,            /* more PSCD data bytes required to decode a symbol */
  JBG_MARKER     /* more PSCD data bytes required, ignored final 0xff byte */
};

struct jbg_ardec_state {
  unsigned char st[4096];    /* probability status for contexts, MSB = MPS */
  unsigned long c;                /* C register, base of coding intervall, *
                                   * layout as in Table 25                 */
  unsigned long a;      /* A register, normalized size of coding intervall */
  int ct;     /* bit shift counter, determines when next byte will be read */
  unsigned char *pscd_ptr;               /* pointer to next PSCD data byte */
  unsigned char *pscd_end;                   /* pointer to byte after PSCD */
  enum jbg_ardec_result result;          /* result of previous decode call */
  int startup;                            /* controls initial fill of s->c */
};

#ifdef TEST_CODEC
void arith_encode_init(struct jbg_arenc_state *s, int reuse_st);
void arith_encode_flush(struct jbg_arenc_state *s);
void arith_encode(struct jbg_arenc_state *s, int cx, int pix);
void arith_decode_init(struct jbg_ardec_state *s, int reuse_st);
int arith_decode(struct jbg_ardec_state *s, int cx);
#endif

 
/*
 * Status of a JBIG encoder
 */

struct jbg_enc_state {
  int d;                            /* resolution layer of the input image */
  unsigned long xd, yd;    /* size of the input image (resolution layer d) */
  unsigned long yd1;    /* BIH announced height of image, use yd1 != yd to
                        emulate T.85-style NEWLEN height updates for tests */
  int planes;                         /* number of different bitmap planes */
  int dl;                       /* lowest resolution layer in the next BIE */
  int dh;                      /* highest resolution layer in the next BIE */
  unsigned long l0;                /* number of lines per stripe at lowest *
                                    * resolution layer 0                   */
  unsigned long stripes;    /* number of stripes required  (determ. by l0) */
  unsigned char **lhp[2];    /* pointers to lower/higher resolution images */
  int *highres;                 /* index [plane] of highres image in lhp[] */
  int order;                                    /* SDE ordering parameters */
  int options;                                      /* encoding parameters */
  unsigned mx, my;                           /* maximum ATMOVE window size */
  int *tx;       /* array [plane] with x-offset of adaptive template pixel */
  char *dppriv;         /* optional private deterministic prediction table */
  char *res_tab;           /* table for the resolution reduction algorithm */
  struct jbg_buf ****sde;      /* array [stripe][layer][plane] pointers to *
				* buffers for stored SDEs                  */
  struct jbg_arenc_state *s;  /* array [planes] for arithm. encoder status */
  struct jbg_buf *free_list; /* list of currently unused SDE block buffers */
  void (*data_out)(unsigned char *start, size_t len, void *file);
                                                    /* data write callback */
  void *file;                            /* parameter passed to data_out() */
  char *tp;    /* buffer for temp. values used by diff. typical prediction */
};


/*
 * Status of a JBIG decoder
 */

struct jbg_dec_state {
  /* data from BIH */
  int d;                             /* resolution layer of the full image */
  int dl;                            /* first resolution layer in this BIE */
  unsigned long xd, yd;     /* size of the full image (resolution layer d) */
  int planes;                         /* number of different bitmap planes */
  unsigned long l0;                /* number of lines per stripe at lowest *
				    * resolution layer 0                   */
  unsigned long stripes;    /* number of stripes required  (determ. by l0) */
  int order;                                    /* SDE ordering parameters */
  int options;                                      /* encoding parameters */
  int mx, my;                                /* maximum ATMOVE window size */
  char *dppriv;         /* optional private deterministic prediction table */

  /* loop variables */
  unsigned long ii[3];  /* current stripe, layer, plane (outer loop first) */

  /*
   * Pointers to array [planes] of lower/higher resolution images.
   * lhp[d & 1] contains image of layer d. 
   */
  unsigned char **lhp[2];

  /* status information */
  int **tx, **ty;   /* array [plane][layer-dl] with x,y-offset of AT pixel */
  struct jbg_ardec_state **s;    /* array [plane][layer-dl] for arithmetic *
				  * decoder status */
  int **reset;     /* array [plane][layer-dl] remembers if previous stripe *
		    * in that plane/resolution ended with SDRST.           */
  unsigned long bie_len;                    /* number of bytes read so far */
  unsigned char buffer[20]; /* used to store BIH or marker segments fragm. */
  int buf_len;                                /* number of bytes in buffer */
  unsigned long comment_skip;      /* remaining bytes of a COMMENT segment */
  unsigned long x;              /* x position of next pixel in current SDE */
  unsigned long i; /* line in current SDE (first line of each stripe is 0) */ 
  int at_moves;                /* number of AT moves in the current stripe */
  unsigned long at_line[JBG_ATMOVES_MAX];           /* lines at which an   *
					             * AT move will happen */
  int at_tx[JBG_ATMOVES_MAX], at_ty[JBG_ATMOVES_MAX]; /* ATMOVE offsets in *
						       * current stripe    */
  unsigned long line_h1, line_h2, line_h3;     /* variables of decode_pscd */
  unsigned long line_l1, line_l2, line_l3;
  int pseudo;         /* flag for TPBON/TPDON:  next pixel is pseudo pixel */
  int **lntp;        /* flag [plane][layer-dl] for TP: line is not typical */

  unsigned long xmax, ymax;         /* if possible abort before image gets *
				     * larger than this size */
  int dmax;                                      /* abort after this layer */
};


/* some macros (too trivial for a function) */

#define jbg_dec_getplanes(s)     ((s)->planes)


/* function prototypes */

void jbg_enc_init(struct jbg_enc_state *s, unsigned long x, unsigned long y,
		  int planes, unsigned char **p,
		  void (*data_out)(unsigned char *start, size_t len,
				   void *file),
		  void *file);
int jbg_enc_lrlmax(struct jbg_enc_state *s, unsigned long mwidth,
		   unsigned long mheight);
void jbg_enc_layers(struct jbg_enc_state *s, int d);
int  jbg_enc_lrange(struct jbg_enc_state *s, int dl, int dh);
void jbg_enc_options(struct jbg_enc_state *s, int order, int options,
		     unsigned long l0, int mx, int my);
void jbg_enc_out(struct jbg_enc_state *s);
void jbg_enc_free(struct jbg_enc_state *s);

void jbg_dec_init(struct jbg_dec_state *s);
void jbg_dec_maxsize(struct jbg_dec_state *s, unsigned long xmax,
		     unsigned long ymax);
int  jbg_dec_in(struct jbg_dec_state *s, unsigned char *data, size_t len,
		size_t *cnt);
long jbg_dec_getwidth(const struct jbg_dec_state *s);
long jbg_dec_getheight(const struct jbg_dec_state *s);
unsigned char *jbg_dec_getimage(const struct jbg_dec_state *s, int plane);
long jbg_dec_getsize(const struct jbg_dec_state *s);
void jbg_dec_merge_planes(const struct jbg_dec_state *s, int use_graycode,
			  void (*data_out)(unsigned char *start, size_t len,
					   void *file), void *file);
long jbg_dec_getsize_merged(const struct jbg_dec_state *s);
void jbg_dec_free(struct jbg_dec_state *s);

const char *jbg_strerror(int errnum, int language);
void jbg_int2dppriv(unsigned char *dptable, const char *internal);
void jbg_dppriv2int(char *internal, const unsigned char *dptable);
unsigned long jbg_ceil_half(unsigned long x, int n);
void jbg_split_planes(unsigned long x, unsigned long y, int has_planes,
		      int encode_planes,
		      const unsigned char *src, unsigned char **dest,
		      int use_graycode);
int jbg_newlen(unsigned char *bie, size_t len);

#endif /* JBG_H */
