/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * msc.h - MSC Player by Lubomir Bulej (pallas@kadan.cz)
 */

#include "player.h"

#define MSC_SIGN_LEN	16
#define MSC_DESC_LEN	64

class CmscPlayer: public CPlayer
{
 public:
  static CPlayer * factory(Copl * newopl);

  CmscPlayer(Copl * newopl);
  ~CmscPlayer();
	
  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);
  float getrefresh();

  std::string gettype ();

 protected:
  typedef unsigned char		u8;
  typedef unsigned short	u16;

  struct msc_header {
    u8 	mh_sign [MSC_SIGN_LEN];
    u16	mh_ver;
    u8	mh_desc [MSC_DESC_LEN];
    u16	mh_timer;
    u16	mh_nr_blocks;
    u16	mh_block_len;
  };

  struct msc_block {
    u16		mb_length;
    u8 *	mb_data;
  };

  // file data
  char *		desc;		// song desctiption
  unsigned short	version;	// file version
  unsigned short	nr_blocks;	// number of music blocks
  unsigned short	block_len;	// maximal block length
  unsigned short	timer_div;	// timer divisor
  msc_block *		msc_data;	// compressed music data

  // decoder state
  unsigned long	block_num;	// active block
  unsigned long	block_pos;	// position in block
  unsigned long	raw_pos;	// position in data buffer
  u8 *		raw_data;	// decompression buffer

  u8 		dec_prefix;	// prefix / state
  int		dec_dist;	// prefix distance
  unsigned int	dec_len;	// prefix length
	
  // player state
  unsigned char	delay;		// active delay
  unsigned long	play_pos;	// player position

 private:
  static const u8 msc_signature [MSC_SIGN_LEN];

  bool load_header (binistream * bf, msc_header * hdr);
  bool decode_octet (u8 * output);
};
