/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2004 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * hsc.cpp - HSC Player by Simon Peter <dn.tlp@gmx.net>
 */

#include <string.h>

#include "hsc.h"
#include "debug.h"

/*** public methods **************************************/

CPlayer *ChscPlayer::factory(Copl *newopl)
{
  return new ChscPlayer(newopl);
}

bool ChscPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream	*f = fp.open(filename);
  int		i;

  // file validation section
  if(!f || !fp.extension(filename, ".hsc") || fp.filesize(f) > 59187) {
    AdPlug_LogWrite("ChscPlayer::load(\"%s\"): Not a HSC file!\n", filename.c_str());
    fp.close(f);
    return false;
  }

  // load section
  for(i=0;i<128*12;i++)		// load instruments
    *((unsigned char *)instr + i) = f->readInt(1);
  for (i=0;i<128;i++) {			// correct instruments
    instr[i][2] ^= (instr[i][2] & 0x40) << 1;
    instr[i][3] ^= (instr[i][3] & 0x40) << 1;
    instr[i][11] >>= 4;			// slide
  }
  for(i=0;i<51;i++) song[i] = f->readInt(1);	// load tracklist
  for(i=0;i<50*64*9;i++)			// load patterns
    *((char *)patterns + i) = f->readInt(1);

  fp.close(f);
  rewind(0);					// rewind module
  return true;
}

bool ChscPlayer::update()
{
  // general vars
  unsigned char		chan,pattnr,note,effect,eff_op,inst,vol,Okt,db;
  unsigned short	Fnr;
  unsigned long		pattoff;

  del--;                      // player speed handling
  if(del)
    return !songend;		// nothing done

  if(fadein)					// fade-in handling
    fadein--;

  pattnr = song[songpos];
  if(pattnr == 0xff) {			// arrangement handling
    songend = 1;				// set end-flag
    songpos = 0;
    pattnr = song[songpos];
  } else
    if ((pattnr & 128) && (pattnr <= 0xb1)) { // goto pattern "nr"
      songpos = song[songpos] & 127;
      pattpos = 0;
      pattnr = song[songpos];
      songend = 1;
    }

  pattoff = pattpos*9;
  for (chan=0;chan<9;chan++) {			// handle all channels
    note = patterns[pattnr][pattoff].note;
    effect = patterns[pattnr][pattoff].effect;
    pattoff++;

    if(note & 128) {                    // set instrument
      setinstr(chan,effect);
      continue;
    }
    eff_op = effect & 0x0f;
    inst = channel[chan].inst;
    if(note)
      channel[chan].slide = 0;

    switch (effect & 0xf0) {			// effect handling
    case 0:								// global effect
      /* The following fx are unimplemented on purpose:
       * 02 - Slide Mainvolume up
       * 03 - Slide Mainvolume down (here: fade in)
       * 04 - Set Mainvolume to 0
       *
       * This is because i've never seen any HSC modules using the fx this way.
       * All modules use the fx the way, i've implemented it.
       */
      switch(eff_op) {
      case 1: pattbreak++; break;	// jump to next pattern
      case 3: fadein = 31; break;	// fade in (divided by 2)
      case 5: mode6 = 1; break;	// 6 voice mode on
      case 6: mode6 = 0; break;	// 6 voice mode off
      }
      break;
    case 0x20:
    case 0x10:		                    // manual slides
      if (effect & 0x10) {
	channel[chan].freq += eff_op;
	channel[chan].slide += eff_op;
      } else {
	channel[chan].freq -= eff_op;
	channel[chan].slide -= eff_op;
      }
      if(!note)
	setfreq(chan,channel[chan].freq);
      break;
    case 0x50:							// set percussion instrument (unimplemented)
      break;
    case 0x60:							// set feedback
      opl->write(0xc0 + chan, (instr[channel[chan].inst][8] & 1) + (eff_op << 1));
      break;
    case 0xa0:		                    // set carrier volume
      vol = eff_op << 2;
      opl->write(0x43 + op_table[chan], vol | (instr[channel[chan].inst][2] & ~63));
      break;
    case 0xb0:		                    // set modulator volume
      vol = eff_op << 2;
      if (instr[inst][8] & 1)
	opl->write(0x40 + op_table[chan], vol | (instr[channel[chan].inst][3] & ~63));
      else
	opl->write(0x40 + op_table[chan],vol | (instr[inst][3] & ~63));
      break;
    case 0xc0:		                    // set instrument volume
      db = eff_op << 2;
      opl->write(0x43 + op_table[chan], db | (instr[channel[chan].inst][2] & ~63));
      if (instr[inst][8] & 1)
	opl->write(0x40 + op_table[chan], db | (instr[channel[chan].inst][3] & ~63));
      break;
    case 0xd0: pattbreak++; songpos = eff_op; songend = 1; break;	// position jump
    case 0xf0:							// set speed
      speed = eff_op;
      del = ++speed;
      break;
    }

    if(fadein)						// fade-in volume setting
      setvolume(chan,fadein*2,fadein*2);

    if(!note)						// note handling
      continue;
    note--;

    if ((note == 0x7f-1) || ((note/12) & ~7)) {    // pause (7fh)
      adl_freq[chan] &= ~32;
      opl->write(0xb0 + chan,adl_freq[chan]);
      continue;
    }

    // play the note
    if(mtkmode)		// imitate MPU-401 Trakker bug
      note--;
    Okt = ((note/12) & 7) << 2;
    Fnr = note_table[(note % 12)] + instr[inst][11] + channel[chan].slide;
    channel[chan].freq = Fnr;
    if(!mode6 || chan < 6)
      adl_freq[chan] = Okt | 32;
    else
      adl_freq[chan] = Okt;		// never set key for drums
    opl->write(0xb0 + chan, 0);
    setfreq(chan,Fnr);
    if(mode6) {
      switch(chan) {		// play drums
      case 6: opl->write(0xbd,bd & ~16); bd |= 48; break;	// bass drum
      case 7: opl->write(0xbd,bd & ~1); bd |= 33; break;	// hihat
      case 8: opl->write(0xbd,bd & ~2); bd |= 34; break;	// cymbal
      }
      opl->write(0xbd,bd);
    }
  }

  del = speed;		// player speed-timing
  if(pattbreak) {		// do post-effect handling
    pattpos=0;			// pattern break!
    pattbreak=0;
    songpos++;
    songpos %= 50;
    if(!songpos)
      songend = 1;
  } else {
    pattpos++;
    pattpos &= 63;		// advance in pattern data
    if (!pattpos) {
      songpos++;
      songpos %= 50;
      if(!songpos)
	songend = 1;
    }
  }
  return !songend;		// still playing
}

void ChscPlayer::rewind(int subsong)
{
  int i;								// counter

  // rewind HSC player
  pattpos = 0; songpos = 0; pattbreak = 0; speed = 2;
  del = 1; songend = 0; mode6 = 0; bd = 0; fadein = 0;

  opl->init();						// reset OPL chip
  opl->write(1,32); opl->write(8,128); opl->write(0xbd,0);

  for(i=0;i<9;i++)
    setinstr((char) i,(char) i);	// init channels
}

unsigned int ChscPlayer::getpatterns()
{
  unsigned char	poscnt,pattcnt=0;

  // count patterns
  for(poscnt=0;poscnt<51 && song[poscnt] != 0xff;poscnt++)
    if(song[poscnt] > pattcnt)
      pattcnt = song[poscnt];

  return (pattcnt+1);
}

unsigned int ChscPlayer::getorders()
{
  unsigned char poscnt;

  // count positions
  for(poscnt=0;poscnt<51;poscnt++)
    if(song[poscnt] == 0xff)
      break;

  return poscnt;
}

unsigned int ChscPlayer::getinstruments()
{
  unsigned char	instcnt,instnum=0,i;
  bool		isinst;

  // count instruments
  for(instcnt=0;instcnt<128;instcnt++) {
    isinst = false;
    for(i=0;i<12;i++)
      if(instr[instcnt][i])
	isinst = true;
    if(isinst)
      instnum++;
  }

  return instnum;
}

/*** private methods *************************************/

void ChscPlayer::setfreq(unsigned char chan, unsigned short freq)
{
  adl_freq[chan] = (adl_freq[chan] & ~3) | (freq >> 8);

  opl->write(0xa0 + chan, freq & 0xff);
  opl->write(0xb0 + chan, adl_freq[chan]);
}

void ChscPlayer::setvolume(unsigned char chan, int volc, int volm)
{
  unsigned char	*ins = instr[channel[chan].inst];
  char		op = op_table[chan];

  opl->write(0x43 + op,volc | (ins[2] & ~63));
  if (ins[8] & 1)							// carrier
    opl->write(0x40 + op,volm | (ins[3] & ~63));
  else
    opl->write(0x40 + op, ins[3]);		// modulator
}

void ChscPlayer::setinstr(unsigned char chan, unsigned char insnr)
{
  unsigned char	*ins = instr[insnr];
  char		op = op_table[chan];

  channel[chan].inst = insnr;		// set internal instrument
  opl->write(0xb0 + chan,0);			// stop old note

  // set instrument
  opl->write(0xc0 + chan, ins[8]);
  opl->write(0x23 + op, ins[0]);        // carrier
  opl->write(0x20 + op, ins[1]);        // modulator
  opl->write(0x63 + op, ins[4]);        // bits 0..3 = decay; 4..7 = attack
  opl->write(0x60 + op, ins[5]);
  opl->write(0x83 + op, ins[6]);        // 0..3 = release; 4..7 = sustain
  opl->write(0x80 + op, ins[7]);
  opl->write(0xe3 + op, ins[9]);        // bits 0..1 = Wellenform
  opl->write(0xe0 + op, ins[10]);
  setvolume(chan, ins[2] & 63, ins[3] & 63);
}
