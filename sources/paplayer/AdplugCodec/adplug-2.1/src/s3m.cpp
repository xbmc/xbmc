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
 * s3m.c - S3M Player by Simon Peter <dn.tlp@gmx.net>
 *
 * BUGS:
 * Extra Fine Slides (EEx, FEx) & Fine Vibrato (Uxy) are inaccurate
 */

#include "s3m.h"

const char Cs3mPlayer::chnresolv[] =	// S3M -> adlib channel conversion
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,-1,-1,-1,-1,-1,-1,-1};

const unsigned short Cs3mPlayer::notetable[12] =		// S3M adlib note table
  {340,363,385,408,432,458,485,514,544,577,611,647};

const unsigned char Cs3mPlayer::vibratotab[32] =		// vibrato rate table
  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

/*** public methods *************************************/

CPlayer *Cs3mPlayer::factory(Copl *newopl)
{
  return new Cs3mPlayer(newopl);
}

Cs3mPlayer::Cs3mPlayer(Copl *newopl): CPlayer(newopl)
{
  int i,j,k;

  memset(pattern,255,sizeof(pattern));
  memset(orders,255,sizeof(orders));

  for(i=0;i<99;i++)		// setup pattern
    for(j=0;j<64;j++)
      for(k=0;k<32;k++) {
	pattern[i][j][k].instrument = 0;
	pattern[i][j][k].info = 0;
      }
}

bool Cs3mPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream		*f = fp.open(filename); if(!f) return false;
  unsigned short	insptr[99],pattptr[99];
  int			i,row;
  unsigned char		bufval,bufval2;
  unsigned short	ppatlen;
  s3mheader		*checkhead;
  bool			adlibins=false;

  // file validation section
  checkhead = new s3mheader;
  load_header(f, checkhead);
  if(checkhead->kennung != 0x1a || checkhead->typ != 16
     || checkhead->insnum > 99) {
    delete checkhead; fp.close(f); return false;
  } else
    if(strncmp(checkhead->scrm,"SCRM",4)) {
      delete checkhead; fp.close(f); return false;
    } else {	// is an adlib module?
      f->seek(checkhead->ordnum, binio::Add);
      for(i = 0; i < checkhead->insnum; i++)
	insptr[i] = f->readInt(2);
      for(i=0;i<checkhead->insnum;i++) {
	f->seek(insptr[i]*16);
	if(f->readInt(1) >= 2) {
	  adlibins = true;
	  break;
	}
      }
      delete checkhead;
      if(!adlibins) { fp.close(f); return false; }
    }

  // load section
  f->seek(0);			// rewind for load
  load_header(f, &header);	// read header

  // security check
  if(header.ordnum > 256 || header.insnum > 99 || header.patnum > 99) {
    fp.close(f);
    return false;
  }

  for(i = 0; i < header.ordnum; i++) orders[i] = f->readInt(1);	// read orders
  for(i = 0; i < header.insnum; i++) insptr[i] = f->readInt(2);	// instrument parapointers
  for(i = 0; i < header.patnum; i++) pattptr[i] = f->readInt(2); // pattern parapointers

  for(i=0;i<header.insnum;i++) {	// load instruments
    f->seek(insptr[i]*16);
    inst[i].type = f->readInt(1);
    f->readString(inst[i].filename, 15);
    inst[i].d00 = f->readInt(1); inst[i].d01 = f->readInt(1);
    inst[i].d02 = f->readInt(1); inst[i].d03 = f->readInt(1);
    inst[i].d04 = f->readInt(1); inst[i].d05 = f->readInt(1);
    inst[i].d06 = f->readInt(1); inst[i].d07 = f->readInt(1);
    inst[i].d08 = f->readInt(1); inst[i].d09 = f->readInt(1);
    inst[i].d0a = f->readInt(1); inst[i].d0b = f->readInt(1);
    inst[i].volume = f->readInt(1); inst[i].dsk = f->readInt(1);
    f->ignore(2);
    inst[i].c2spd = f->readInt(4);
    f->ignore(12);
    f->readString(inst[i].name, 28);
    f->readString(inst[i].scri, 4);
  }

  for(i=0;i<header.patnum;i++) {	// depack patterns
    f->seek(pattptr[i]*16);
    ppatlen = f->readInt(2);
    unsigned long pattpos = f->pos();
    for(row=0;(row<64) && (pattpos-pattptr[i]*16<=ppatlen);row++)
      do {
	bufval = f->readInt(1);
	if(bufval & 32) {
	  bufval2 = f->readInt(1);
	  pattern[i][row][bufval & 31].note = bufval2 & 15;
	  pattern[i][row][bufval & 31].oct = (bufval2 & 240) >> 4;
	  pattern[i][row][bufval & 31].instrument = f->readInt(1);
	}
	if(bufval & 64)
	  pattern[i][row][bufval & 31].volume = f->readInt(1);
	if(bufval & 128) {
	  pattern[i][row][bufval & 31].command = f->readInt(1);
	  pattern[i][row][bufval & 31].info = f->readInt(1);
	}
      } while(bufval);
  }

  fp.close(f);
  rewind(0);
  return true;		// done
}

bool Cs3mPlayer::update()
{
  unsigned char	pattbreak=0,donote;		// remember vars
  unsigned char	pattnr,chan,row,info;	// cache vars
  signed char		realchan;

  // effect handling (timer dependant)
  for(realchan=0; realchan<9; realchan++) {
    info = channel[realchan].info;	// fill infobyte cache
    switch(channel[realchan].fx) {
    case 11:
    case 12: if(channel[realchan].fx == 11)	// dual command: H00 and Dxy
      vibrato(realchan,channel[realchan].dualinfo);
    else					// dual command: G00 and Dxy
      tone_portamento(realchan,channel[realchan].dualinfo);
    case 4: if(info <= 0x0f)			// volume slide down
      if(channel[realchan].vol - info >= 0)
	channel[realchan].vol -= info;
      else
	channel[realchan].vol = 0;
      if((info & 0x0f) == 0)			// volume slide up
	if(channel[realchan].vol + (info >> 4) <= 63)
	  channel[realchan].vol += info >> 4;
	else
	  channel[realchan].vol = 63;
      setvolume(realchan);
      break;
    case 5: if(info == 0xf0 || info <= 0xe0) {	// slide down
      slide_down(realchan,info);
      setfreq(realchan);
    }
      break;
    case 6: if(info == 0xf0 || info <= 0xe0) {	// slide up
      slide_up(realchan,info);
      setfreq(realchan);
    }
      break;
    case 7: tone_portamento(realchan,channel[realchan].dualinfo); break;	// tone portamento
    case 8: vibrato(realchan,channel[realchan].dualinfo); break;	// vibrato
    case 10: channel[realchan].nextfreq = channel[realchan].freq;	// arpeggio
      channel[realchan].nextoct = channel[realchan].oct;
      switch(channel[realchan].trigger) {
      case 0: channel[realchan].freq = notetable[channel[realchan].note]; break;
      case 1: if(channel[realchan].note + ((info & 0xf0) >> 4) < 12)
	channel[realchan].freq = notetable[channel[realchan].note + ((info & 0xf0) >> 4)];
      else {
	channel[realchan].freq = notetable[channel[realchan].note + ((info & 0xf0) >> 4) - 12];
	channel[realchan].oct++;
      }
	break;
      case 2: if(channel[realchan].note + (info & 0x0f) < 12)
	channel[realchan].freq = notetable[channel[realchan].note + (info & 0x0f)];
      else {
	channel[realchan].freq = notetable[channel[realchan].note + (info & 0x0f) - 12];
	channel[realchan].oct++;
      }
	break;
      }
      if(channel[realchan].trigger < 2)
	channel[realchan].trigger++;
      else
	channel[realchan].trigger = 0;
      setfreq(realchan);
      channel[realchan].freq = channel[realchan].nextfreq;
      channel[realchan].oct = channel[realchan].nextoct;
      break;
    case 21: vibrato(realchan,(unsigned char) (info / 4)); break;	// fine vibrato
    }
  }

  if(del) {		// speed compensation
    del--;
    return !songend;
  }

  // arrangement handling
  pattnr = orders[ord];
  if(pattnr == 0xff || ord > header.ordnum) {	// "--" end of song
    songend = 1;				// set end-flag
    ord = 0;
    pattnr = orders[ord];
    if(pattnr == 0xff)
      return !songend;
  }
  if(pattnr == 0xfe) {		// "++" skip marker
    ord++; pattnr = orders[ord];
  }

  // play row
  row = crow;	// fill row cache
  for(chan=0;chan<32;chan++) {
    if(!(header.chanset[chan] & 128))		// resolve S3M -> AdLib channels
      realchan = chnresolv[header.chanset[chan] & 127];
    else
      realchan = -1;		// channel disabled
    if(realchan != -1) {	// channel playable?
      // set channel values
      donote = 0;
      if(pattern[pattnr][row][chan].note < 14)
	// tone portamento
	if(pattern[pattnr][row][chan].command == 7 || pattern[pattnr][row][chan].command == 12) {
	  channel[realchan].nextfreq = notetable[pattern[pattnr][row][chan].note];
	  channel[realchan].nextoct = pattern[pattnr][row][chan].oct;
	} else {											// normal note
	  channel[realchan].note = pattern[pattnr][row][chan].note;
	  channel[realchan].freq = notetable[pattern[pattnr][row][chan].note];
	  channel[realchan].oct = pattern[pattnr][row][chan].oct;
	  channel[realchan].key = 1;
	  donote = 1;
	}
      if(pattern[pattnr][row][chan].note == 14) {	// key off (is 14 here, cause note is only first 4 bits)
	channel[realchan].key = 0;
	setfreq(realchan);
      }
      if((channel[realchan].fx != 8 && channel[realchan].fx != 11) &&	// vibrato begins
	 (pattern[pattnr][row][chan].command == 8 || pattern[pattnr][row][chan].command == 11)) {
	channel[realchan].nextfreq = channel[realchan].freq;
	channel[realchan].nextoct = channel[realchan].oct;
      }
      if(pattern[pattnr][row][chan].note >= 14)
	if((channel[realchan].fx == 8 || channel[realchan].fx == 11) &&	// vibrato ends
	   (pattern[pattnr][row][chan].command != 8 && pattern[pattnr][row][chan].command != 11)) {
	  channel[realchan].freq = channel[realchan].nextfreq;
	  channel[realchan].oct = channel[realchan].nextoct;
	  setfreq(realchan);
	}
      if(pattern[pattnr][row][chan].instrument) {	// set instrument
	channel[realchan].inst = pattern[pattnr][row][chan].instrument - 1;
	if(inst[channel[realchan].inst].volume < 64)
	  channel[realchan].vol = inst[channel[realchan].inst].volume;
	else
	  channel[realchan].vol = 63;
	if(pattern[pattnr][row][chan].command != 7)
	  donote = 1;
      }
      if(pattern[pattnr][row][chan].volume != 255)
	if(pattern[pattnr][row][chan].volume < 64)	// set volume
	  channel[realchan].vol = pattern[pattnr][row][chan].volume;
	else
	  channel[realchan].vol = 63;
      channel[realchan].fx = pattern[pattnr][row][chan].command;	// set command
      if(pattern[pattnr][row][chan].info)			// set infobyte
	channel[realchan].info = pattern[pattnr][row][chan].info;

      // some commands reset the infobyte memory
      switch(channel[realchan].fx) {
      case 1:
      case 2:
      case 3:
      case 20:
	channel[realchan].info = pattern[pattnr][row][chan].info;
	break;
      }

      // play note
      if(donote)
	playnote(realchan);
      if(pattern[pattnr][row][chan].volume != 255)	// set volume
	setvolume(realchan);

      // command handling (row dependant)
      info = channel[realchan].info;	// fill infobyte cache
      switch(channel[realchan].fx) {
      case 1: speed = info; break;	// set speed
      case 2: if(info <= ord) songend = 1; ord = info; crow = 0; pattbreak = 1; break;	// jump to order
      case 3: if(!pattbreak) { crow = info; ord++; pattbreak = 1; } break;	// pattern break
      case 4: if(info > 0xf0)		// fine volume down
	if(channel[realchan].vol - (info & 0x0f) >= 0)
	  channel[realchan].vol -= info & 0x0f;
	else
	  channel[realchan].vol = 0;
	if((info & 0x0f) == 0x0f && info >= 0x1f)	// fine volume up
	  if(channel[realchan].vol + ((info & 0xf0) >> 4) <= 63)
	    channel[realchan].vol += (info & 0xf0) >> 4;
	  else
	    channel[realchan].vol = 63;
	setvolume(realchan);
	break;
      case 5: if(info > 0xf0)	{			// fine slide down
	slide_down(realchan,(unsigned char) (info & 0x0f));
	setfreq(realchan);
      }
	if(info > 0xe0 && info < 0xf0) {		// extra fine slide down
	  slide_down(realchan,(unsigned char) ((info & 0x0f) / 4));
	  setfreq(realchan);
	}
	break;
      case 6: if(info > 0xf0) {				// fine slide up
	slide_up(realchan,(unsigned char) (info & 0x0f));
	setfreq(realchan);
      }
	if(info > 0xe0 && info < 0xf0) {		// extra fine slide up
	  slide_up(realchan,(unsigned char) ((info & 0x0f) / 4));
	  setfreq(realchan);
	}
	break;
      case 7:														// tone portamento
      case 8:	if((channel[realchan].fx == 7 ||	// vibrato (remember info for dual commands)
		    channel[realchan].fx == 8) && pattern[pattnr][row][chan].info)
	channel[realchan].dualinfo = info;
	break;
      case 10: channel[realchan].trigger = 0; break;	// arpeggio (set trigger)
      case 19: if(info == 0xb0)				// set loop start
	loopstart = row;
	if(info > 0xb0 && info <= 0xbf)			// pattern loop
	  if(!loopcnt) {
	    loopcnt = info & 0x0f;
	    crow = loopstart;
	    pattbreak = 1;
	  } else
	    if(--loopcnt > 0) {
	      crow = loopstart;
	      pattbreak = 1;
	    }
	if((info & 0xf0) == 0xe0)			// patterndelay
	  del = speed * (info & 0x0f) - 1;
	break;
      case 20: tempo = info; break;			// set tempo
      }
    }
  }

  if(!del)
    del = speed - 1;		// speed compensation
  if(!pattbreak) {		// next row (only if no manual advance)
    crow++;
    if(crow > 63) {
      crow = 0;
      ord++;
      loopstart = 0;
    }
  }

  return !songend;		// still playing
}

void Cs3mPlayer::rewind(int subsong)
{
  // set basic variables
  songend = 0; ord = 0; crow = 0; tempo = header.it;
  speed = header.is; del = 0; loopstart = 0; loopcnt = 0;

  memset(channel,0,sizeof(channel));

  opl->init();				// reset OPL chip
  opl->write(1,32);			// Go to ym3812 mode
}

std::string Cs3mPlayer::gettype()
{
  char filever[5];

  switch(header.cwtv) {		// determine version number
  case 0x1300: strcpy(filever,"3.00"); break;
  case 0x1301: strcpy(filever,"3.01"); break;
  case 0x1303: strcpy(filever,"3.03"); break;
  case 0x1320: strcpy(filever,"3.20"); break;
  default: strcpy(filever,"3.??");
  }

  return (std::string("Scream Tracker ") + filever);
}

float Cs3mPlayer::getrefresh()
{
  return (float) (tempo / 2.5);
}

/*** private methods *************************************/

void Cs3mPlayer::load_header(binistream *f, s3mheader *h)
{
  int i;

  f->readString(h->name, 28);
  h->kennung = f->readInt(1); h->typ = f->readInt(1);
  f->ignore(2);
  h->ordnum = f->readInt(2); h->insnum = f->readInt(2);
  h->patnum = f->readInt(2); h->flags = f->readInt(2);
  h->cwtv = f->readInt(2); h->ffi = f->readInt(2);
  f->readString(h->scrm, 4);
  h->gv = f->readInt(1); h->is = f->readInt(1); h->it = f->readInt(1);
  h->mv = f->readInt(1); h->uc = f->readInt(1); h->dp = f->readInt(1);
  f->ignore(8);
  h->special = f->readInt(2);
  for(i = 0; i < 32; i++) h->chanset[i] = f->readInt(1);
}

void Cs3mPlayer::setvolume(unsigned char chan)
{
  unsigned char op = op_table[chan], insnr = channel[chan].inst;

  opl->write(0x43 + op,(int)(63-((63-(inst[insnr].d03 & 63))/63.0)*channel[chan].vol) + (inst[insnr].d03 & 192));
  if(inst[insnr].d0a & 1)
    opl->write(0x40 + op,(int)(63-((63-(inst[insnr].d02 & 63))/63.0)*channel[chan].vol) + (inst[insnr].d02 & 192));
}

void Cs3mPlayer::setfreq(unsigned char chan)
{
  opl->write(0xa0 + chan, channel[chan].freq & 255);
  if(channel[chan].key)
    opl->write(0xb0 + chan, ((channel[chan].freq & 768) >> 8) + (channel[chan].oct << 2) | 32);
  else
    opl->write(0xb0 + chan, ((channel[chan].freq & 768) >> 8) + (channel[chan].oct << 2));
}

void Cs3mPlayer::playnote(unsigned char chan)
{
  unsigned char op = op_table[chan], insnr = channel[chan].inst;

  opl->write(0xb0 + chan, 0);	// stop old note

  // set instrument data
  opl->write(0x20 + op, inst[insnr].d00);
  opl->write(0x23 + op, inst[insnr].d01);
  opl->write(0x40 + op, inst[insnr].d02);
  opl->write(0x43 + op, inst[insnr].d03);
  opl->write(0x60 + op, inst[insnr].d04);
  opl->write(0x63 + op, inst[insnr].d05);
  opl->write(0x80 + op, inst[insnr].d06);
  opl->write(0x83 + op, inst[insnr].d07);
  opl->write(0xe0 + op, inst[insnr].d08);
  opl->write(0xe3 + op, inst[insnr].d09);
  opl->write(0xc0 + chan, inst[insnr].d0a);

  // set frequency & play
  channel[chan].key = 1;
  setfreq(chan);
}

void Cs3mPlayer::slide_down(unsigned char chan, unsigned char amount)
{
  if(channel[chan].freq - amount > 340)
    channel[chan].freq -= amount;
  else
    if(channel[chan].oct > 0) {
      channel[chan].oct--;
      channel[chan].freq = 684;
    } else
      channel[chan].freq = 340;
}

void Cs3mPlayer::slide_up(unsigned char chan, unsigned char amount)
{
  if(channel[chan].freq + amount < 686)
    channel[chan].freq += amount;
  else
    if(channel[chan].oct < 7) {
      channel[chan].oct++;
      channel[chan].freq = 341;
    } else
      channel[chan].freq = 686;
}

void Cs3mPlayer::vibrato(unsigned char chan, unsigned char info)
{
  unsigned char i,speed,depth;

  speed = info >> 4;
  depth = (info & 0x0f) / 2;

  for(i=0;i<speed;i++) {
    channel[chan].trigger++;
    while(channel[chan].trigger >= 64)
      channel[chan].trigger -= 64;
    if(channel[chan].trigger >= 16 && channel[chan].trigger < 48)
      slide_down(chan,(unsigned char) (vibratotab[channel[chan].trigger - 16] / (16-depth)));
    if(channel[chan].trigger < 16)
      slide_up(chan,(unsigned char) (vibratotab[channel[chan].trigger + 16] / (16-depth)));
    if(channel[chan].trigger >= 48)
      slide_up(chan,(unsigned char) (vibratotab[channel[chan].trigger - 48] / (16-depth)));
  }
  setfreq(chan);
}

void Cs3mPlayer::tone_portamento(unsigned char chan, unsigned char info)
{
  if(channel[chan].freq + (channel[chan].oct << 10) < channel[chan].nextfreq +
     (channel[chan].nextoct << 10))
    slide_up(chan,info);
  if(channel[chan].freq + (channel[chan].oct << 10) > channel[chan].nextfreq +
     (channel[chan].nextoct << 10))
    slide_down(chan,info);
  setfreq(chan);
}
