/*
    $Id: nrg.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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

#if defined(_XBOX) || defined(WIN32)
#pragma pack(1)
#else
PRAGMA_BEGIN_PACKED
#endif
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
  uint8_t  type       GNUC_PACKED; /* has track copy bit and whether audiofile
				      or datafile. Is often 0x41 == 'A' */
  uint8_t  track      GNUC_PACKED; /* binary or BCD?? */
  uint8_t  addr_ctrl  GNUC_PACKED; /* addresstype: MSF or LBA in lower 4 bits
				      control in upper 4 bits. 
				      makes 0->1 transitions */
  uint8_t  res        GNUC_PACKED; /* ?? */
  uint32_t lsn        GNUC_PACKED; 
} _cuex_array_t;

typedef struct {
  uint32_t _unknown1  GNUC_PACKED;
  char      psz_mcn[CDIO_MCN_SIZE]  GNUC_PACKED;
  uint8_t  _unknown[64-CDIO_MCN_SIZE-sizeof(uint32_t)]  GNUC_PACKED;
} _daox_array_t;

typedef struct {
  uint32_t _unknown1  GNUC_PACKED;
  char      psz_mcn[CDIO_MCN_SIZE]  GNUC_PACKED;
  uint8_t  _unknown[64-CDIO_MCN_SIZE-sizeof(uint32_t)]  GNUC_PACKED;
} _daoi_array_t;

typedef struct {
  uint32_t id                    GNUC_PACKED;
  uint32_t len                   GNUC_PACKED;
  char data[EMPTY_ARRAY_SIZE]    GNUC_PACKED;
} _chunk_t;

#if defined(_XBOX) || defined(WIN32)
#pragma pack()
#else
  PRAGMA_END_PACKED
#endif

/* Nero images are Big Endian. */
#define CDTX_ID  0x43445458  /* CD TEXT */
#define CUEX_ID  0x43554558  /* Nero version 5.5.x-6.x */
#define CUES_ID  0x43554553  /* Nero pre version 5.5.x-6.x */
#define DAOX_ID  0x44414f58  /* Nero version 5.5.x-6.x */
#define DAOI_ID  0x44414f49
#define END1_ID  0x454e4421
#define ETN2_ID  0x45544e32
#define ETNF_ID  0x45544e46
#define NER5_ID  0x4e455235  /* Nero version 5.5.x */
#define NERO_ID  0x4e45524f  /* Nero pre 5.5.x */
#define SINF_ID  0x53494e46  /* Session information */
#define MTYP_ID  0x4d545950  /* Disc Media type? */

#define MTYP_AUDIO_CD 1 /* This isn't correct. But I don't know the
			   the right thing is and it sometimes works (and
			   sometimes is wrong). */

/* Disk track type Values gleaned from DAOX */
#define DTYP_MODE1     0 
#define DTYP_MODE2_XA  2 
#define DTYP_INVALID 255
