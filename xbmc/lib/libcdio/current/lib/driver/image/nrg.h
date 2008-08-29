/*
    $Id: nrg.h,v 1.3 2006/01/14 09:45:44 rocky Exp $

    Copyright (C) 2004, 2006 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 2001, 2003 Herbert Valerio Riedel <hvr@gnu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* NERO (NRG) file format structures. */

/* this ugly image format is typical for lazy win32 programmers... at
   least structure were set big endian, so at reverse
   engineering wasn't such a big headache... */

PRAGMA_BEGIN_PACKED
typedef union {
  struct {
    uint32_t __x          GNUC_PACKED;
    uint32_t ID           GNUC_PACKED;
    uint32_t footer_ofs   GNUC_PACKED;
  } v50;
  struct {
    uint32_t ID           GNUC_PACKED;
    uint64_t footer_ofs   GNUC_PACKED;
  } v55;
} _footer_t;

typedef struct {
  uint32_t start      GNUC_PACKED;
  uint32_t length     GNUC_PACKED;
  uint32_t type       GNUC_PACKED; /* 0x0 -> MODE1,  0x2 -> MODE2 form1,
				      0x3 -> MIXED_MODE2 2336 blocksize 
				   */
  uint32_t start_lsn  GNUC_PACKED; /* does not include any pre-gaps! */
  uint32_t _unknown   GNUC_PACKED; /* wtf is this for? -- always zero... */
} _etnf_array_t;

/* Finally they realized that 32-bit offsets are a bit outdated for
   IA64 *eg* */
typedef struct {
  uint64_t start      GNUC_PACKED;
  uint64_t length     GNUC_PACKED;
  uint32_t type       GNUC_PACKED; /* 0x0 -> MODE1,  0x2 -> MODE2 form1,
				      0x3 -> MIXED_MODE2 2336 blocksize 
				   */
  uint32_t start_lsn  GNUC_PACKED;
  uint64_t _unknown   GNUC_PACKED; /* wtf is this for? -- always zero... */
} _etn2_array_t;

typedef struct {
  uint8_t  type;                   /* has track copy bit and whether audiofile
				      or datafile. Is often 0x41 == 'A' */
  uint8_t  track;                  /* binary or BCD?? */
  uint8_t  addr_ctrl;              /* addresstype: MSF or LBA in lower 4 bits
				      control in upper 4 bits. 
				      makes 0->1 transitions */
  uint8_t  res;                    /* ?? */
  uint32_t lsn        GNUC_PACKED; 
} _cuex_array_t;

typedef struct {
  uint32_t _unknown1  GNUC_PACKED;
  char      psz_mcn[CDIO_MCN_SIZE];             
  uint8_t  _unknown[64-CDIO_MCN_SIZE-sizeof(uint32_t)];
} _daox_array_t;

typedef struct {
  uint32_t _unknown1  GNUC_PACKED;
  char      psz_mcn[CDIO_MCN_SIZE];
  uint8_t  _unknown[64-CDIO_MCN_SIZE-sizeof(uint32_t)];
} _daoi_array_t;

typedef struct {
  uint32_t id                    GNUC_PACKED;
  uint32_t len                   GNUC_PACKED;
  char data[EMPTY_ARRAY_SIZE];
} _chunk_t;

PRAGMA_END_PACKED

/* Nero images are Big Endian. */
typedef enum {
  CDTX_ID  = 0x43445458,   /* CD TEXT */
  CUEX_ID  = 0x43554558,  /* Nero version 5.5.x-6.x */
  CUES_ID  = 0x43554553,  /* Nero pre version 5.5.x-6.x */
  DAOX_ID  = 0x44414f58,  /* Nero version 5.5.x-6.x */
  DAOI_ID  = 0x44414f49,
  END1_ID  = 0x454e4421,
  ETN2_ID  = 0x45544e32,
  ETNF_ID  = 0x45544e46,
  NER5_ID  = 0x4e455235,  /* Nero version 5.5.x */
  NERO_ID  = 0x4e45524f,  /* Nero pre 5.5.x */
  SINF_ID  = 0x53494e46,  /* Session information */
  MTYP_ID  = 0x4d545950,  /* Disc Media type? */
} nero_id_t;

#define MTYP_AUDIO_CD 1 /* This isn't correct. But I don't know the
			   the right thing is and it sometimes works (and
			   sometimes is wrong). */

/* Disk track type Values gleaned from DAOX */
typedef enum {
  DTYP_MODE1    =   0,
  DTYP_MODE2_XA =   2,
  DTYP_INVALID  = 255
} nero_dtype_t;

/** The below variables are trickery to force the above enum symbol
    values to be recorded in debug symbol tables. They are used to
    allow one to refer to the enumeration value names in the typedefs
    above in a debugger and debugger expressions.
*/
extern nero_id_t    nero_id;
extern nero_dtype_t nero_dtype;
  
