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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "cc_decoder.h"

#define LOG_DEBUG 3

/* number of text colors specified by EIA-608 standard */
#define NUM_FG_COL 7

#ifndef TARGET_WINDOWS
/* colors specified by the EIA 608 standard */
enum { WHITE, GREEN, BLUE, CYAN, RED, YELLOW, MAGENTA, BLACK, TRANSPARENT };
#else
/* colors specified by the EIA 608 standard */
enum { WHITE, GREEN, BLUE, CYAN, RED, YELLOW, MAGENTA, BLACK };
#endif

/* --------------------- misc. EIA 608 definitions -------------------*/

#define TRANSP_SPACE 0x19   /* code for transparent space, essentially 
			       arbitrary */

#define MAX(a, b) ((a) > (b)? (a) : (b))

/* mapping from PAC row code to actual CC row */
static int  rowdata[] = {10, -1, 0, 1, 2, 3, 11, 12, 13, 14, 4, 5, 6,
			 7, 8, 9};
/* FIXME: do real TM */
/* must be mapped as a music note in the captioning font */ 

static char specialchar[] = {0xAE,0xB0,0xBD,0xBF,0x54,0xA2,0xA3,0xB6,0xA0,
        TRANSP_SPACE,0xA8,0xA2,0xAA,0xAE,0xB4,0xBB}; 

/* character translation table - EIA 608 codes are not all the same as ASCII */
static char chartbl[128];

/* CC codes use odd parity for error detection, since they originally were */
/* transmitted via noisy video signals */
static int parity_table[256];

/*---------------- general utility functions ---------------------*/

static int parity(uint8_t byte)
{
  int i;
  int ones = 0;

  for (i = 0; i < 7; i++) {
    if (byte & (1 << i))
      ones++;
  }

  return ones & 1;
}


static void build_parity_table(void)
{
  uint8_t byte;
  for (byte = 0; byte <= 127; byte++) {
    int parity_v = parity(byte);
    /* CC uses odd parity (i.e., # of 1's in byte is odd.) */
    parity_table[byte] = parity_v;
    parity_table[byte | 0x80] = !parity_v;
  }
}


static int good_parity(uint16_t data)
{
  int ret = parity_table[data & 0xff] && parity_table[(data & 0xff00) >> 8];
  if (! ret)
    printf("Bad parity in EIA-608 data (%x)\n", data);
  return ret;
}


static void build_char_table(void)
{
  int i;
  /* first the normal ASCII codes */
  for (i = 0; i < 128; i++)
   chartbl[i] = (char) i;
 /* now the special codes */
 chartbl[0x2a] = 0xA1;
 chartbl[0x5c] = 0xA9;
 chartbl[0x5e] = 0xAD;
 chartbl[0x5f] = 0xB3;
 chartbl[0x60] = 0xAA;
 chartbl[0x7b] = 0xA7;
 chartbl[0x7c] = 0xb7;
 chartbl[0x7d] = 0x91;
 chartbl[0x7e] = 0xB1;
 chartbl[0x7f] = 0xA4;    /* FIXME: this should be a solid block */
}
/*
static int ccbuf_has_displayable(cc_buffer_t *buf)
{
  int i;
  int found = 0;
  for (i = 0; !found && i < CC_ROWS; i++) {
    if (buf->rows[i].num_chars > 0)
      found = 1;
  }
  return found;
}
*/

static void ccbuf_add_char(cc_buffer_t *buf, uint8_t c)
{
  cc_row_t *rowbuf = &buf->rows[buf->rowpos];
  int pos = rowbuf->pos;
  int left_displayable = (pos > 0) && (pos <= rowbuf->num_chars);

#if LOG_DEBUG > 2
  printf("cc_decoder: ccbuf_add_char: %c @ %d/%d\n", c, buf->rowpos, pos);
#endif

  if (pos >= CC_COLUMNS) {
    printf("cc_decoder: ccbuf_add_char: row buffer overflow\n");
    return;
  }

  if (pos > rowbuf->num_chars) {
    /* fill up to indented position with transparent spaces, if necessary */
    /* ccrow_fill_transp(rowbuf); */
  }

  /* midrow PAC attributes are applied only if there is no displayable */
  /* character to the immediate left. This makes the implementation rather */
  /* complicated, but this is what the EIA-608 standard specifies. :-( */
  if (rowbuf->pac_attr_chg && !rowbuf->attr_chg && !left_displayable) {
    rowbuf->attr_chg = 1;
    rowbuf->cells[pos].attributes = rowbuf->pac_attr;
#ifdef LOG_DEBUG
    printf("cc_decoder: ccbuf_add_char: Applying midrow PAC.\n");
#endif
  }

  rowbuf->cells[pos].c = c;
  rowbuf->cells[pos].midrow_attr = rowbuf->attr_chg;
  rowbuf->pos++;

  if (rowbuf->num_chars < rowbuf->pos)
    rowbuf->num_chars = rowbuf->pos;

  rowbuf->attr_chg = 0;
  rowbuf->pac_attr_chg = 0;
}


static void ccbuf_set_cursor(cc_buffer_t *buf, int row, int column, 
			     int underline, int italics, int color)
{
  cc_row_t *rowbuf = &buf->rows[row];
  cc_attribute_t attr;

  attr.italic = italics;
  attr.underline = underline;
  attr.foreground = color;
  attr.background = BLACK;

  rowbuf->pac_attr = attr;
  rowbuf->pac_attr_chg = 1;

  buf->rowpos = row; 
  rowbuf->pos = column;
  rowbuf->attr_chg = 0;
}


static void ccbuf_apply_attribute(cc_buffer_t *buf, cc_attribute_t *attr)
{
  cc_row_t *rowbuf = &buf->rows[buf->rowpos];
  int pos = rowbuf->pos;
  
  rowbuf->attr_chg = 1;
  rowbuf->cells[pos].attributes = *attr;
  /* A midrow attribute always counts as a space */
  ccbuf_add_char(buf, chartbl[(unsigned int) ' ']);
}


static void ccbuf_tab(cc_buffer_t *buf, int tabsize)
{
  cc_row_t *rowbuf = &buf->rows[buf->rowpos];
  rowbuf->pos += tabsize;
  if (rowbuf->pos > CC_COLUMNS) {
#ifdef LOG_DEBUG
    printf("cc_decoder: ccbuf_tab: row buffer overflow\n");
#endif
    rowbuf->pos = CC_COLUMNS;
    return;
  }
  /* tabs have no effect on pending PAC attribute changes */
}

/*----------------- cc_memory_t methods --------------------------------*/

static void ccmem_clear(cc_memory_t *buf)
{
#ifdef LOG_DEBUG
  printf("cc_decoder.c: ccmem_clear: Clearing CC memory\n");
#endif
  memset(buf, 0, sizeof (cc_memory_t));
}


static void ccmem_init(cc_memory_t *buf)
{
  ccmem_clear(buf);
}


static void ccmem_exit(cc_memory_t *buf)
{
/*FIXME: anything to deallocate?*/
}

/*----------------- cc_decoder_t methods --------------------------------*/

static void cc_set_channel(cc_decoder_t *dec, int channel)
{
  (*dec->active)->channel_no = channel;
#ifdef LOG_DEBUG
  printf("cc_decoder: cc_set_channel: selecting channel %d\n", channel);
#endif
}


static cc_buffer_t *active_ccbuffer(cc_decoder_t *dec)
{
  cc_memory_t *mem = *dec->active;
  return &mem->channel[mem->channel_no];
}

static void cc_swap_buffers(cc_decoder_t *dec)
{
  cc_memory_t *temp;

  /* hide caption in displayed memory */
  /* cc_hide_displayed(dec); */

#ifdef LOG_DEBUG
  printf("cc_decoder: cc_swap_buffers: swapping caption memory\n");
#endif
  temp = dec->on_buf;
  dec->on_buf = dec->off_buf;
  dec->off_buf = temp;

  /* show new displayed memory */
  /* cc_show_displayed(dec); */
}

static void cc_decode_standard_char(cc_decoder_t *dec, uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf = active_ccbuffer(dec);
  /* c1 always is a valid character */
  ccbuf_add_char(buf, chartbl[c1]);
  /* c2 might not be a printable character, even if c1 was */
  if (c2 & 0x60)
    ccbuf_add_char(buf, chartbl[c2]);
}


static void cc_decode_PAC(cc_decoder_t *dec, int channel,
			  uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf;
  int row, column = 0;
  int underline, italics = 0, color;

  /* There is one invalid PAC code combination. Ignore it. */
  if (c1 == 0x10 && c2 > 0x5f)
    return;

  cc_set_channel(dec, channel);
  buf = active_ccbuffer(dec);

  row = rowdata[((c1 & 0x07) << 1) | ((c2 & 0x20) >> 5)];
  if (c2 & 0x10) {
    column = ((c2 & 0x0e) >> 1) * 4;   /* preamble indentation */
    color = WHITE;                     /* indented lines have white color */
  }
  else if ((c2 & 0x0e) == 0x0e) {
    italics = 1;                       /* italics, they are always white */
    color = WHITE;
  }
  else
    color = (c2 & 0x0e) >> 1;
  underline = c2 & 0x01;

#ifdef LOG_DEBUG
  printf("cc_decoder: cc_decode_PAC: row %d, col %d, ul %d, it %d, clr %d\n",
	 row, column, underline, italics, color);
#endif

  ccbuf_set_cursor(buf, row, column, underline, italics, color);
}


static void cc_decode_ext_attribute(cc_decoder_t *dec, int channel,
				    uint8_t c1, uint8_t c2)
{
  cc_set_channel(dec, channel);
}


static void cc_decode_special_char(cc_decoder_t *dec, int channel,
				   uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf;

  cc_set_channel(dec, channel);
  buf = active_ccbuffer(dec);
#ifdef LOG_DEBUG
  printf("cc_decoder: cc_decode_special_char: Mapping %x to %x\n", c2, specialchar[c2 & 0xf]);
#endif
  ccbuf_add_char(buf, specialchar[c2 & 0xf]);
}


static void cc_decode_midrow_attr(cc_decoder_t *dec, int channel,
				  uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf;
  cc_attribute_t attr;

  cc_set_channel(dec, channel);
  buf = active_ccbuffer(dec);
  if (c2 < 0x2e) {
    attr.italic = 0;
    attr.foreground = (c2 & 0xe) >> 1;
  }
  else {
    attr.italic = 1;
    attr.foreground = WHITE;
  }
  attr.underline = c2 & 0x1;
  attr.background = BLACK;
#ifdef LOG_DEBUG
  printf("cc_decoder: cc_decode_midrow_attr: attribute %x\n", c2);
  printf("cc_decoder: cc_decode_midrow_attr: ul %d, it %d, clr %d\n",
	 attr.underline, attr.italic, attr.foreground);
#endif

  ccbuf_apply_attribute(buf, &attr);
}


static void cc_decode_misc_control_code(cc_decoder_t *dec, int channel,
					uint8_t c1, uint8_t c2)
{
#ifdef LOG_DEBUG
  printf("cc_decoder: decode_misc: decoding %x %x\n", c1, c2);
#endif

  cc_set_channel(dec, channel);

  switch (c2) {          /* 0x20 <= c2 <= 0x2f */

  case 0x20:             /* RCL */
    break;

  case 0x21:             /* backspace */
#ifdef LOG_DEBUG
    printf("cc_decoder: backspace\n");
#endif
    break;

  case 0x24:             /* DER */
    break;

  case 0x25:             /* RU2 */
    break;

  case 0x26:             /* RU3 */
    break;

  case 0x27:             /* RU4 */
    break;

  case 0x28:             /* FON */
    break;

  case 0x29:             /* RDC */
    break;

  case 0x2a:             /* TR */
    break;

  case 0x2b:             /* RTD */
    break;

  case 0x2c:             /* EDM - erase displayed memory */
    /* cc_hide_displayed(dec); */
    ccmem_clear(dec->on_buf);
    break;

  case 0x2d:             /* carriage return */
    break;

  case 0x2e:             /* ENM - erase non-displayed memory */
    ccmem_clear(dec->off_buf);
    break;

  case 0x2f:             /* EOC - swap displayed and non displayed memory */
    cc_swap_buffers(dec);
    break;
  }
}


static void cc_decode_tab(cc_decoder_t *dec, int channel,
			  uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf;

  cc_set_channel(dec, channel);
  buf = active_ccbuffer(dec);
  ccbuf_tab(buf, c2 & 0x3);
}


static void cc_decode_EIA608(cc_decoder_t *dec, uint16_t data)
{
  uint8_t c1 = data & 0x7f;
  uint8_t c2 = (data >> 8) & 0x7f;

#if LOG_DEBUG >= 3
  printf("decoding %x %x\n", c1, c2);
#endif

  if (c1 & 0x60) {             /* normal character, 0x20 <= c1 <= 0x7f */
    cc_decode_standard_char(dec, c1, c2);
  }
  else if (c1 & 0x10) {        /* control code or special character */
                               /* 0x10 <= c1 <= 0x1f */
    int channel = (c1 & 0x08) >> 3;
    c1 &= ~0x08;

    /* control sequences are often repeated. In this case, we should */
    /* evaluate it only once. */
    if (data != dec->lastcode) {

      if (c2 & 0x40) {         /* preamble address code: 0x40 <= c2 <= 0x7f */
	cc_decode_PAC(dec, channel, c1, c2);
      }
      else {
	switch (c1) {
	  
	case 0x10:             /* extended background attribute code */
	  cc_decode_ext_attribute(dec, channel, c1, c2);
	  break;

	case 0x11:             /* attribute or special character */
	  if ((c2 & 0x30) == 0x30) { /* special char: 0x30 <= c2 <= 0x3f  */
	    cc_decode_special_char(dec, channel, c1, c2);
	  }
	  else if (c2 & 0x20) {     /* midrow attribute: 0x20 <= c2 <= 0x2f */
	    cc_decode_midrow_attr(dec, channel, c1, c2);
	  }
	  break;

	case 0x14:             /* possibly miscellaneous control code */
	  cc_decode_misc_control_code(dec, channel, c1, c2);
	  break;

	case 0x17:            /* possibly misc. control code TAB offset */
	                      /* 0x21 <= c2 <= 0x23 */
	  if (c2 >= 0x21 && c2 <= 0x23) {
	    cc_decode_tab(dec, channel, c1, c2);
	  }
	  break;
	}
      }
    }
  }
  
  dec->lastcode = data;
}


void decode_cc(cc_decoder_t *dec, uint8_t *buffer, uint32_t buf_len)
{
  /* The first number may denote a channel number. I don't have the
   * EIA-708 standard, so it is hard to say.
   * From what I could figure out so far, the general format seems to be:
   *
   * repeat
   *
   *   0xfe starts 2 byte sequence of unknown purpose. It might denote
   *        field #2 in line 21 of the VBI. We'll ignore it for the
   *        time being.
   *
   *   0xff starts 2 byte EIA-608 sequence, field #1 in line 21 of the VBI.
   *        Followed by a 3-code triplet that starts either with 0xff or
   *        0xfe. In either case, the following triplet needs to be ignored
   *        for line 21, field 1.
   *
   *   0x00 is padding, followed by 2 more 0x00.
   *
   *   0x01 always seems to appear at the beginning, always seems to
   *        be followed by 0xf8, 8-bit number. 
   *        The lower 7 bits of this 8-bit number seem to denote the
   *        number of code triplets that follow.
   *        The most significant bit denotes whether the Line 21 field 1 
   *        captioning information is at odd or even triplet offsets from this
   *        beginning triplet. 1 denotes odd offsets, 0 denotes even offsets.
   *      
   *        Most captions are encoded with odd offsets, so this is what we
   *        will assume.
   *
   * until end of packet
   */
  uint8_t *current = buffer;
  uint32_t curbytes = 0;
  int odd_offset = 1;

  while (curbytes < buf_len) {
    int skip = 2;

    uint8_t cc_code = *current++;
    curbytes++;
    
    if (buf_len - curbytes < 2) {
#ifdef LOG_DEBUG
      fprintf(stderr, "Not enough data for 2-byte CC encoding\n");
#endif
      break;
    }
    
    uint8_t data1 = *current;
    uint8_t data2 = *(current + 1);
    
    switch (cc_code) {
    case 0xfe:
      /* expect 2 byte encoding (perhaps CC3, CC4?) */
      /* ignore for time being */
      skip = 2;
      break;
      
    case 0xff:
      /* expect EIA-608 CC1/CC2 encoding */
      if (good_parity(data1 | (data2 << 8))) {
	cc_decode_EIA608(dec, data1 | (data2 << 8));

      }
      skip = 5;
      break;
      
    case 0x00:
      /* This seems to be just padding */
      skip = 2;
      break;
      
    case 0x01:
      odd_offset = data2 & 0x80;
      if (odd_offset)
	skip = 2;
      else
	skip = 5;
      break;
      
    default:
#ifdef LOG_DEBUG
      fprintf(stderr, "Unknown CC encoding: %x\n", cc_code);
#endif
      skip = 2;
      break;
    }
    current += skip;
    curbytes += skip;
  }
}



cc_decoder_t *cc_decoder_open()
{
  cc_decoder_t *dec = (cc_decoder_t *) calloc(1, sizeof (cc_decoder_t));

  ccmem_init(&dec->buffer[0]);
  ccmem_init(&dec->buffer[1]);
  dec->on_buf = &dec->buffer[0];
  dec->off_buf = &dec->buffer[1];
  dec->active = &dec->off_buf;

  dec->lastcode = 0;
  dec->capid = 0;

#ifdef LOG_DEBUG
  printf("spucc: cc_decoder_open\n");
#endif
  return dec;
}


void cc_decoder_close(cc_decoder_t *dec)
{
  ccmem_exit(&dec->buffer[0]);
  ccmem_exit(&dec->buffer[1]);

  free(dec);

#ifdef LOG_DEBUG
  printf("spucc: cc_decoder_close\n");
#endif
}


/*--------------- initialization methods --------------------------*/

void cc_decoder_init(void)
{
  build_parity_table();
  build_char_table();
}

#ifdef __cplusplus
}
#endif

