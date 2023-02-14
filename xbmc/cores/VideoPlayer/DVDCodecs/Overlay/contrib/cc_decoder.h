/*
 *  Copyright (C) 2000-2008 the xine project
 *
 *  Copyright (C) Christian Vogler
 *                cvogler@gradient.cis.upenn.edu - December 2001
 *
 *  This file is part of xine, a free video player.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  stuff needed to provide closed captioning decoding and display
 *
 *  Some small bits and pieces of the EIA-608 captioning decoder were
 *  adapted from CCDecoder 0.9.1 by Mike Baker. The latest version is
 *  available at http://sourceforge.net/projects/ccdecoder/.
 */

#pragma once

#include <stdint.h>

#define CC_ROWS 15
#define CC_COLUMNS 32
#define CC_CHANNELS 2

/* colors specified by the EIA 608 standard */
enum cc_colors
{
  WHITE,
  GREEN,
  BLUE,
  CYAN,
  RED,
  YELLOW,
  MAGENTA,
  BLACK
};

typedef struct cc_attribute_s {
  uint8_t italic;
  uint8_t underline;
  uint8_t foreground;
  uint8_t background;
} cc_attribute_t;

/* CC character cell */
typedef struct cc_char_cell_s
{
  uint8_t c;                   /* character code, not the same as ASCII */
  cc_attribute_t attributes;   /* attributes of this character, if changed */
			       /* here */
  uint8_t charset; /* charset type */
  int midrow_attr;             /* true if this cell changes an attribute */
} cc_char_cell_t;

/* a single row in the closed captioning memory */
typedef struct cc_row_s
{
  cc_char_cell_t cells[CC_COLUMNS];
  int pos;                   /* position of the cursor */
  int num_chars;             /* how many characters in the row are data */
  int attr_chg;              /* true if midrow attr. change at cursor pos */
  int pac_attr_chg;          /* true if attribute has changed via PAC */
  cc_attribute_t pac_attr;   /* PAC attr. that hasn't been applied yet */
} cc_row_t;

/* closed captioning memory for a single channel */
typedef struct cc_buffer_s
{
  cc_row_t rows[CC_ROWS];
  int rowpos;              /* row cursor position */
} cc_buffer_t;

/* captioning memory for all channels */
typedef struct cc_memory_s
{
  cc_buffer_t channel[CC_CHANNELS];
  int channel_no;          /* currently active channel */
} cc_memory_t;

enum cc_style
{
  CC_NOTSET = 0,
  CC_ROLLUP,
  CC_PAINTON,
  CC_POPON
};

/* The closed captioning decoder data structure */
struct cc_decoder_s
{
  /* CC decoder buffer  - one onscreen, one offscreen */
  cc_memory_t buffer[2];
  /* onscreen, offscreen buffer ptrs */
  cc_memory_t *on_buf;
  cc_memory_t *off_buf;
  /* which buffer is active for receiving data */
  cc_memory_t **active;

  /* the last captioning code seen (control codes are often sent twice
     in a row, but should be processed only once) */
  uint32_t lastcode;

  uint16_t rollup_rows;
  enum cc_style style;

  void *userdata;
  void(*callback)(int service, void *userdata);
  char text[CC_ROWS*CC_COLUMNS + 1];
  int textlen;
  cc_attribute_t textattr;
};

typedef struct cc_decoder_s cc_decoder_t;

cc_decoder_t *cc_decoder_open();
void cc_decoder_close(cc_decoder_t *this_obj);
void cc_decoder_init(void);

void decode_cc(cc_decoder_t *dec, const uint8_t *buffer, uint32_t buf_len);
