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
 * msc.c - MSC Player by Lubomir Bulej (pallas@kadan.cz)
 */

#include <stdio.h>
#include <cstring>

#include "msc.h"
#include "debug.h"

const unsigned char CmscPlayer::msc_signature [MSC_SIGN_LEN] = {
  'C', 'e', 'r', 'e', 's', ' ', '\x13', ' ',
  'M', 'S', 'C', 'p', 'l', 'a', 'y', ' ' };

/*** public methods *************************************/

CPlayer *CmscPlayer::factory (Copl * newopl)
{
  return new CmscPlayer (newopl);
}

CmscPlayer::CmscPlayer(Copl * newopl) : CPlayer (newopl)
{
  desc = NULL;
  msc_data = NULL;
  raw_data = NULL;
  nr_blocks = 0;
}

CmscPlayer::~CmscPlayer()
{
  if (raw_data != NULL)
    delete [] raw_data;
	
  if (msc_data != NULL) {
    // free compressed blocks
    for (int blk_num = 0; blk_num < nr_blocks; blk_num++) {
      if (msc_data [blk_num].mb_data != NULL)
	delete [] msc_data [blk_num].mb_data;
    }
		
    delete [] msc_data;
  }
	
  if (desc != NULL)
    delete [] desc;
}

bool CmscPlayer::load(const std::string & filename, const CFileProvider & fp)
{
  binistream * 	bf;
  msc_header	hdr;

  // open and validate the file
  bf = fp.open (filename);
  if (! bf)
    return false;
	
  if (! load_header (bf, & hdr)) {
    fp.close (bf);
    return false;
  }
	
  // get stuff from the header
  version = hdr.mh_ver;
  timer_div = hdr.mh_timer;
  nr_blocks = hdr.mh_nr_blocks;
  block_len = hdr.mh_block_len;

  if (! nr_blocks) {
    fp.close (bf);
    return false;
  }

  // load compressed data blocks
  msc_data = new msc_block [nr_blocks];
  raw_data = new u8 [block_len];

  for (int blk_num = 0; blk_num < nr_blocks; blk_num++) {
    msc_block blk;

    blk.mb_length = bf->readInt (2);
    blk.mb_data = new u8 [blk.mb_length];
    for (int oct_num = 0; oct_num < blk.mb_length; oct_num++) {
      blk.mb_data [oct_num] = bf->readInt (1);
    }

    msc_data [blk_num] = blk;
  }

  // clean up & initialize
  fp.close (bf);
  rewind (0);

  return true;
}

bool CmscPlayer::update()
{
  // output data
  while (! delay) {
    u8	cmnd;
    u8	data;

    // decode data
    if (! decode_octet (& cmnd))
      return false;
			
    if (! decode_octet (& data))
      return false;
			
    // check for special commands
    switch (cmnd) {

      // delay
    case 0xff:
      delay = 1 + (u8) (data - 1);
      break;
		
      // play command & data
    default:
      opl->write (cmnd, data);
			
    } // command switch
  } // play pass
	
	
  // count delays
  if (delay)
    delay--;
	
  // advance player position
  play_pos++;
  return true;
}

void CmscPlayer::rewind(int subsong)
{
  // reset state
  dec_prefix = 0;
  block_num = 0;
  block_pos = 0;
  play_pos = 0;
  raw_pos = 0;
  delay = 0;
	
  // init the OPL chip and go to OPL2 mode
  opl->init();
  opl->write(1, 32);
}

float CmscPlayer::getrefresh()
{
  // PC timer oscillator frequency / wait register
  return 1193180 / (float) (timer_div ? timer_div : 0xffff);
}

std::string CmscPlayer::gettype()
{
  char vstr [40];

  sprintf(vstr, "AdLib MSCplay (version %d)", version);
  return std::string (vstr);
}

/*** private methods *************************************/

bool CmscPlayer::load_header(binistream * bf, msc_header * hdr)
{
  // check signature
  bf->readString ((char *) hdr->mh_sign, sizeof (hdr->mh_sign));
  if (memcmp (msc_signature, hdr->mh_sign, MSC_SIGN_LEN) != 0)
    return false;

  // check version
  hdr->mh_ver = bf->readInt (2);
  if (hdr->mh_ver != 0)
    return false;

  bf->readString ((char *) hdr->mh_desc, sizeof (hdr->mh_desc));
  hdr->mh_timer = bf->readInt (2);
  hdr->mh_nr_blocks = bf->readInt (2);
  hdr->mh_block_len = bf->readInt (2);
  return true;
}

bool CmscPlayer::decode_octet(u8 * output)
{
  msc_block blk;			// compressed data block
	
  if (block_num >= nr_blocks)
    return false;
		
  blk = msc_data [block_num];
  while (1) {
    u8 	octet;		// decoded octet
    u8	len_corr;	// length correction
		
    // advance to next block if necessary
    if (block_pos >= blk.mb_length && dec_len == 0) {
      block_num++;
      if (block_num >= nr_blocks)
	return false;
				
      blk = msc_data [block_num];
      block_pos = 0;
      raw_pos = 0;
    }
		
    // decode the compressed music data
    switch (dec_prefix) {
		
      // decode prefix
    case 155:
    case 175:
      octet = blk.mb_data [block_pos++];
      if (octet == 0) {
	// invalid prefix, output original
	octet = dec_prefix;
	dec_prefix = 0;
	break;
      }
			
      // isolate length and distance
      dec_len = (octet & 0x0F);
      len_corr = 2;

      dec_dist = (octet & 0xF0) >> 4;
      if (dec_prefix == 155)
	dec_dist++;
				
      // next decode step for respective prefix type
      dec_prefix++;
      continue;
			

      // check for extended length
    case 156:
      if (dec_len == 15)
	dec_len += blk.mb_data [block_pos++];

      // add length correction and go for copy mode
      dec_len += len_corr;
      dec_prefix = 255;
      continue;
			
			
      // get extended distance
    case 176:
      dec_dist += 17 + 16 * blk.mb_data [block_pos++];
      len_corr = 3;
			
      // check for extended length
      dec_prefix = 156;
      continue;
			
			
      // prefix copy mode
    case 255:
      if((int)raw_pos >= dec_dist)
	octet = raw_data [raw_pos - dec_dist];
      else {
	AdPlug_LogWrite("error! read before raw_data buffer.\n");
	octet = 0;
      }

      dec_len--;
      if (dec_len == 0) {
	// back to normal mode
	dec_prefix = 0;
      }
			
      break;

			
      // normal mode
    default:
      octet = blk.mb_data [block_pos++];
      if (octet == 155 || octet == 175) {
	// it's a prefix, restart
	dec_prefix = octet;
	continue;
      }
    } // prefix switch

			
    // output the octet
    if (output != NULL)
      *output = octet;
			
    raw_data [raw_pos++] = octet;
    break;
  }; // decode pass

  return true;
}	
