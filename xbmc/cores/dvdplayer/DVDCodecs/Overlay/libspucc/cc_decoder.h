/*
 * Copyright (C) 2000-2008 the xine project
 *
 * Copyright (C) Christian Vogler
 *               cvogler@gradient.cis.upenn.edu - December 2001
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * stuff needed to provide closed captioning decoding and display
 *
 * Some small bits and pieces of the EIA-608 captioning decoder were
 * adapted from CCDecoder 0.9.1 by Mike Baker. The latest version is
 * available at http://sourceforge.net/projects/ccdecoder/.
 */

#ifdef _LINUX
#undef EMULATE_INTTYPES
#else
#define EMULATE_INTTYPES
#endif /* _LINUX */

#ifndef EMULATE_INTTYPES
#   include <stdint.h>
#else
    typedef signed char  int8_t;
    typedef signed short int16_t;
    typedef signed int   int32_t;
    typedef unsigned char  uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int   uint32_t;

#   ifdef CONFIG_WIN32
        typedef signed __int64   int64_t;
        typedef unsigned __int64 uint64_t;
#   elif __WORDSIZE == 64
        typedef long int                int64_t;
        typedef unsigned long int       uint64_t;
#   else /* other OS */
        typedef signed long long   int64_t;
        typedef unsigned long long uint64_t;
#   endif /* other OS */
#endif /* EMULATE_INTTYPES */

#define CC_ROWS 15
#define CC_COLUMNS 32
#define CC_CHANNELS 2

typedef struct cc_attribute_s {
  uint8_t italic;
  uint8_t underline;
  uint8_t foreground;
  uint8_t background;
} cc_attribute_t;

/* CC character cell */
typedef struct cc_char_cell_s {
  uint8_t c;                   /* character code, not the same as ASCII */
  cc_attribute_t attributes;   /* attributes of this character, if changed */
			       /* here */
  int midrow_attr;             /* true if this cell changes an attribute */
} cc_char_cell_t;

/* a single row in the closed captioning memory */
typedef struct cc_row_s {
  cc_char_cell_t cells[CC_COLUMNS];
  int pos;                   /* position of the cursor */
  int num_chars;             /* how many characters in the row are data */
  int attr_chg;              /* true if midrow attr. change at cursor pos */
  int pac_attr_chg;          /* true if attribute has changed via PAC */
  cc_attribute_t pac_attr;   /* PAC attr. that hasn't been applied yet */
} cc_row_t;

/* closed captioning memory for a single channel */
typedef struct cc_buffer_s {
  cc_row_t rows[CC_ROWS];
  int rowpos;              /* row cursor position */
} cc_buffer_t;

/* captioning memory for all channels */
typedef struct cc_memory_s {
  cc_buffer_t channel[CC_CHANNELS];
  int channel_no;          /* currently active channel */
} cc_memory_t;

/* The closed captioning decoder data structure */
struct cc_decoder_s {
  /* CC decoder buffer  - one onscreen, one offscreen */
  cc_memory_t buffer[2];
  /* onscreen, offscreen buffer ptrs */
  cc_memory_t *on_buf;
  cc_memory_t *off_buf;
  /* which buffer is active for receiving data */
  cc_memory_t **active;

  /* for logging and debugging purposes, captions are assigned increasing */
  /*   unique ids. */
  uint32_t capid;

  /* the last captioning code seen (control codes are often sent twice
     in a row, but should be processed only once) */
  uint32_t lastcode;
};

typedef struct cc_decoder_s cc_decoder_t;

#define NUM_CC_PALETTES 2
extern char *cc_schemes[NUM_CC_PALETTES + 1];

#define CC_FONT_MAX 256

typedef struct cc_config_s {
  int cc_enabled;             /* true if closed captions are enabled */
  char font[CC_FONT_MAX];     /* standard captioning font & size */
  int font_size;
  char italic_font[CC_FONT_MAX];   /* italic captioning font & size */
  int center;                 /* true if captions should be centered */
                              /* according to text width */
  int cc_scheme;              /* which captioning scheme to use */
  
  int config_version;         /* the decoder should be updated when this is increased */
} cc_config_t;

typedef struct cc_state_s {
  cc_config_t *cc_cfg;
  /* the following variables are not controlled by configuration files; they */
  /* are intrinsic to the properties of the configuration options and the */
  /* currently played video */
  int            can_cc;      /* true if captions can be displayed */
                              /* (e.g., font fits on screen) */
} cc_state_t;

cc_decoder_t *cc_decoder_open();
void cc_decoder_close(cc_decoder_t *this_obj);
void cc_decoder_init(void);

void decode_cc(cc_decoder_t *dec, uint8_t *buffer, uint32_t buf_len);
