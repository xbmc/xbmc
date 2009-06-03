/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * protrack.cpp - Generic Protracker Player
 *
 * NOTES:
 * This is a generic Protracker-based formats player. It offers all Protracker
 * features, plus a good set of extensions to be compatible to other Protracker
 * derivatives. It is derived from the former SA2 player. If you got a
 * Protracker-like format, this is most certainly the player you want to use.
 */

#include "protrack.h"
#include "debug.h"

#define SPECIALARPLEN	256	// Standard length of special arpeggio lists
#define JUMPMARKER	0x80	// Orderlist jump marker

// SA2 compatible adlib note table
const unsigned short CmodPlayer::sa2_notetable[12] =
  {340,363,385,408,432,458,485,514,544,577,611,647};

// SA2 compatible vibrato rate table
const unsigned char CmodPlayer::vibratotab[32] =
  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

/*** public methods *************************************/

CmodPlayer::CmodPlayer(Copl *newopl)
  : CPlayer(newopl), inst(0), order(0), arplist(0), arpcmd(0), initspeed(6),
    nop(0), activechan(0xffffffff), flags(Standard), curchip(opl->getchip()),
    nrows(0), npats(0), nchans(0)
{
  realloc_order(128);
  realloc_patterns(64, 64, 9);
  realloc_instruments(250);
  init_notetable(sa2_notetable);
}

CmodPlayer::~CmodPlayer()
{
  dealloc();
}

bool CmodPlayer::update()
{
  unsigned char		pattbreak=0, donote, pattnr, chan, oplchan, info1,
    info2, info, pattern_delay;
  unsigned short	track;
  unsigned long		row;

  if(!speed)		// song full stop
    return !songend;

  // effect handling (timer dependant)
  for(chan = 0; chan < nchans; chan++) {
    oplchan = set_opl_chip(chan);

    if(arplist && arpcmd && inst[channel[chan].inst].arpstart)	// special arpeggio
      if(channel[chan].arpspdcnt)
	channel[chan].arpspdcnt--;
      else
	if(arpcmd[channel[chan].arppos] != 255) {
	  switch(arpcmd[channel[chan].arppos]) {
	  case 252: channel[chan].vol1 = arplist[channel[chan].arppos];	// set volume
	    if(channel[chan].vol1 > 63)	// ?????
	      channel[chan].vol1 = 63;
	    channel[chan].vol2 = channel[chan].vol1;
	    setvolume(chan);
	    break;
	  case 253: channel[chan].key = 0; setfreq(chan); break;	// release sustaining note
	  case 254: channel[chan].arppos = arplist[channel[chan].arppos]; break; // arpeggio loop
	  default: if(arpcmd[channel[chan].arppos]) {
	    if(arpcmd[channel[chan].arppos] / 10)
	      opl->write(0xe3 + op_table[oplchan], arpcmd[channel[chan].arppos] / 10 - 1);
	    if(arpcmd[channel[chan].arppos] % 10)
	      opl->write(0xe0 + op_table[oplchan], (arpcmd[channel[chan].arppos] % 10) - 1);
	    if(arpcmd[channel[chan].arppos] < 10)	// ?????
	      opl->write(0xe0 + op_table[oplchan], arpcmd[channel[chan].arppos] - 1);
	  }
	  }
	  if(arpcmd[channel[chan].arppos] != 252) {
	    if(arplist[channel[chan].arppos] <= 96)
	      setnote(chan,channel[chan].note + arplist[channel[chan].arppos]);
	    if(arplist[channel[chan].arppos] >= 100)
	      setnote(chan,arplist[channel[chan].arppos] - 100);
	  } else
	    setnote(chan,channel[chan].note);
	  setfreq(chan);
	  if(arpcmd[channel[chan].arppos] != 255)
	    channel[chan].arppos++;
	  channel[chan].arpspdcnt = inst[channel[chan].inst].arpspeed - 1;
	}

    info1 = channel[chan].info1;
    info2 = channel[chan].info2;
    if(flags & Decimal)
      info = channel[chan].info1 * 10 + channel[chan].info2;
    else
      info = (channel[chan].info1 << 4) + channel[chan].info2;
    switch(channel[chan].fx) {
    case 0:	if(info) {	// arpeggio
      if(channel[chan].trigger < 2)
	channel[chan].trigger++;
      else
	channel[chan].trigger = 0;
      switch(channel[chan].trigger) {
      case 0: setnote(chan,channel[chan].note); break;
      case 1: setnote(chan,channel[chan].note + info1); break;
      case 2: setnote(chan,channel[chan].note + info2);
      }
      setfreq(chan);
    }
      break;
    case 1: slide_up(chan,info); setfreq(chan); break;	// slide up
    case 2: slide_down(chan,info); setfreq(chan); break;	// slide down
    case 3: tone_portamento(chan,channel[chan].portainfo); break;	// tone portamento
    case 4: vibrato(chan,channel[chan].vibinfo1,channel[chan].vibinfo2); break;	// vibrato
    case 5:	// tone portamento & volume slide
    case 6: if(channel[chan].fx == 5)		// vibrato & volume slide
      tone_portamento(chan,channel[chan].portainfo);
    else
      vibrato(chan,channel[chan].vibinfo1,channel[chan].vibinfo2);
    case 10: if(del % 4)	// SA2 volume slide
      break;
      if(info1)
	vol_up(chan,info1);
      else
	vol_down(chan,info2);
      setvolume(chan);
      break;
    case 14: if(info1 == 3)	// retrig note
      if(!(del % (info2+1)))
	playnote(chan);
      break;
    case 16: if(del % 4)	// AMD volume slide
      break;
      if(info1)
	vol_up_alt(chan,info1);
      else
	vol_down_alt(chan,info2);
      setvolume(chan);
      break;
    case 20:			// RAD volume slide
      if(info < 50)
	vol_down_alt(chan,info);
      else
	vol_up_alt(chan,info - 50);
      setvolume(chan);
      break;
    case 26: 			// volume slide
      if(info1)
	vol_up(chan,info1);
      else
	vol_down(chan,info2);
      setvolume(chan);
      break;
    case 28:
      if (info1) {
	slide_up(chan,1); channel[chan].info1--;
      }
      if (info2) {
	slide_down(chan,1); channel[chan].info2--;
      }
      setfreq(chan);
      break;
    }
  }

  if(del) {		// speed compensation
    del--;
    return !songend;
  }

  // arrangement handling
  if(!resolve_order()) return !songend;
  pattnr = order[ord];

  if(!rw) AdPlug_LogWrite("\nCmodPlayer::update(): Pattern: %d, Order: %d\n", pattnr, ord);
  AdPlug_LogWrite("CmodPlayer::update():%3d|", rw);

  // play row
  pattern_delay = 0;
  row = rw;
  for(chan = 0; chan < nchans; chan++) {
    oplchan = set_opl_chip(chan);

    if(!(activechan >> (31 - chan)) & 1) {	// channel active?
      AdPlug_LogWrite("N/A|");
      continue;
    }
    if(!(track = trackord[pattnr][chan])) {	// resolve track
      AdPlug_LogWrite("------------|");
      continue;
    } else
      track--;

    AdPlug_LogWrite("%3d%3d%2X%2X%2X|", tracks[track][row].note,
		    tracks[track][row].inst, tracks[track][row].command,
		    tracks[track][row].param1, tracks[track][row].param2);

    donote = 0;
    if(tracks[track][row].inst) {
      channel[chan].inst = tracks[track][row].inst - 1;
      if (!(flags & Faust)) {
	channel[chan].vol1 = 63 - (inst[channel[chan].inst].data[10] & 63);
	channel[chan].vol2 = 63 - (inst[channel[chan].inst].data[9] & 63);
	setvolume(chan);
      }
    }

    if(tracks[track][row].note && tracks[track][row].command != 3) {	// no tone portamento
      channel[chan].note = tracks[track][row].note;
      setnote(chan,tracks[track][row].note);
      channel[chan].nextfreq = channel[chan].freq;
      channel[chan].nextoct = channel[chan].oct;
      channel[chan].arppos = inst[channel[chan].inst].arpstart;
      channel[chan].arpspdcnt = 0;
      if(tracks[track][row].note != 127)	// handle key off
	donote = 1;
    }
    channel[chan].fx = tracks[track][row].command;
    channel[chan].info1 = tracks[track][row].param1;
    channel[chan].info2 = tracks[track][row].param2;

    if(donote)
      playnote(chan);

    // command handling (row dependant)
    info1 = channel[chan].info1;
    info2 = channel[chan].info2;
    if(flags & Decimal)
      info = channel[chan].info1 * 10 + channel[chan].info2;
    else
      info = (channel[chan].info1 << 4) + channel[chan].info2;
    switch(channel[chan].fx) {
    case 3: // tone portamento
      if(tracks[track][row].note) {
	if(tracks[track][row].note < 13)
	  channel[chan].nextfreq = notetable[tracks[track][row].note - 1];
	else
	  if(tracks[track][row].note % 12 > 0)
	    channel[chan].nextfreq = notetable[(tracks[track][row].note % 12) - 1];
	  else
	    channel[chan].nextfreq = notetable[11];
	channel[chan].nextoct = (tracks[track][row].note - 1) / 12;
	if(tracks[track][row].note == 127) {	// handle key off
	  channel[chan].nextfreq = channel[chan].freq;
	  channel[chan].nextoct = channel[chan].oct;
	}
      }
      if(info)	// remember vars
	channel[chan].portainfo = info;
      break;

    case 4: // vibrato (remember vars)
      if(info) {
	channel[chan].vibinfo1 = info1;
	channel[chan].vibinfo2 = info2;
      }
      break;

    case 7: tempo = info; break;	// set tempo

    case 8: channel[chan].key = 0; setfreq(chan); break;	// release sustaining note

    case 9: // set carrier/modulator volume
      if(info1)
	channel[chan].vol1 = info1 * 7;
      else
	channel[chan].vol2 = info2 * 7;
      setvolume(chan);
      break;

    case 11: // position jump
      pattbreak = 1; rw = 0; if(info < ord) songend = 1; ord = info; break;

    case 12: // set volume
      channel[chan].vol1 = info;
      channel[chan].vol2 = info;
      if(channel[chan].vol1 > 63)
	channel[chan].vol1 = 63;
      if(channel[chan].vol2 > 63)
	channel[chan].vol2 = 63;
      setvolume(chan);
      break;

    case 13: // pattern break
      if(!pattbreak) { pattbreak = 1; rw = info; ord++; } break;

    case 14: // extended command
      switch(info1) {
      case 0: // define cell-tremolo
	if(info2)
	  regbd |= 128;
	else
	  regbd &= 127;
	opl->write(0xbd,regbd);
	break;

      case 1: // define cell-vibrato
	if(info2)
	  regbd |= 64;
	else
	  regbd &= 191;
	opl->write(0xbd,regbd);
	break;

      case 4: // increase volume fine
	vol_up_alt(chan,info2);
	setvolume(chan);
	break;

      case 5: // decrease volume fine
	vol_down_alt(chan,info2);
	setvolume(chan);
	break;

      case 6: // manual slide up
	slide_up(chan,info2);
	setfreq(chan);
	break;

      case 7: // manual slide down
	slide_down(chan,info2);
	setfreq(chan);
	break;

      case 8: // pattern delay (rows)
	pattern_delay = info2 * speed;
	break;
      }
      break;

    case 15: // SA2 set speed
      if(info <= 0x1f)
	speed = info;
      if(info >= 0x32)
	tempo = info;
      if(!info)
	songend = 1;
      break;

    case 17: // alternate set volume
      channel[chan].vol1 = info;
      if(channel[chan].vol1 > 63)
	channel[chan].vol1 = 63;
      if(inst[channel[chan].inst].data[0] & 1) {
	channel[chan].vol2 = info;
	if(channel[chan].vol2 > 63)
	  channel[chan].vol2 = 63;
      }

      setvolume(chan);
      break;

    case 18: // AMD set speed
      if(info <= 31 && info > 0)
	speed = info;
      if(info > 31 || !info)
	tempo = info;
      break;

    case 19: // RAD/A2M set speed
      speed = (info ? info : info + 1);
      break;

    case 21: // set modulator volume
      if(info <= 63)
	channel[chan].vol2 = info;
      else
	channel[chan].vol2 = 63;
      setvolume(chan);
      break;

    case 22: // set carrier volume
      if(info <= 63)
	channel[chan].vol1 = info;
      else
	channel[chan].vol1 = 63;
      setvolume(chan);
      break;

    case 23: // fine frequency slide up
      slide_up(chan,info);
      setfreq(chan);
      break;

    case 24: // fine frequency slide down
      slide_down(chan,info);
      setfreq(chan);
      break;

    case 25: // set carrier/modulator waveform
      if(info1 != 0x0f)
	opl->write(0xe3 + op_table[oplchan],info1);
      if(info2 != 0x0f)
	opl->write(0xe0 + op_table[oplchan],info2);
      break;

    case 27: // set chip tremolo/vibrato
      if(info1)
	regbd |= 128;
      else
	regbd &= 127;
      if(info2)
	regbd |= 64;
      else
	regbd &= 191;
      opl->write(0xbd,regbd);
      break;

    case 29: // pattern delay (frames)
      pattern_delay = info;
      break;
    }
  }

  // speed compensation
  del = speed - 1 + pattern_delay;

  if(!pattbreak) {	// next row (only if no manual advance)
    rw++;
    if(rw >= nrows) {
      rw = 0;
      ord++;
    }
  }

  resolve_order();	// so we can report songend right away
  AdPlug_LogWrite("\n");
  return !songend;
}

unsigned char CmodPlayer::set_opl_chip(unsigned char chan)
  /*
   * Sets OPL chip according to channel number. Channels 0-8 are on first chip,
   * channels 9-17 are on second chip. Returns corresponding OPL channel
   * number.
   */
{
  int newchip = chan < 9 ? 0 : 1;

  if(newchip != curchip) {
    opl->setchip(newchip);
    curchip = newchip;
  }

  return chan % 9;
}

bool CmodPlayer::resolve_order()
  /*
   * Resolves current orderlist entry, checking for jumps and loops.
   *
   * Returns true on correct processing, false if immediate recursive loop
   * has been detected.
   */
{
  if(ord < length) {
    while(order[ord] >= JUMPMARKER) {	// jump to order
      unsigned long neword = order[ord] - JUMPMARKER;

      if(neword <= ord) songend = 1;
      if(neword == ord) return false;
      ord = neword;
    }
  } else {
    songend = 1;
    ord = restartpos;
  }

  return true;
}

void CmodPlayer::rewind(int subsong)
{
  unsigned long i;

  // Reset playing variables
  songend = del = ord = rw = regbd = 0;
  tempo = bpm; speed = initspeed;

  // Reset channel data
  memset(channel,0,sizeof(Channel)*nchans);

  // Compute number of patterns, if needed
  if(!nop)
    for(i=0;i<length;i++)
      nop = (order[i] > nop ? order[i] : nop);

  opl->init();		// Reset OPL chip
  opl->write(1, 32);	// Go to ym3812 mode

  // Enable OPL3 extensions if flagged
  if(flags & Opl3) {
    opl->setchip(1);
    opl->write(1, 32);
    opl->write(5, 1);
    opl->setchip(0);
  }

  // Enable tremolo/vibrato depth if flagged
  if(flags & Tremolo) regbd |= 128;
  if(flags & Vibrato) regbd |= 64;
  if(regbd) opl->write(0xbd, regbd);
}

float CmodPlayer::getrefresh()
{
  return (float) (tempo / 2.5);
}

void CmodPlayer::init_trackord()
{
  unsigned long i;

  for(i=0;i<npats*nchans;i++)
    trackord[i / nchans][i % nchans] = i + 1;
}

bool CmodPlayer::init_specialarp()
{
  arplist = new unsigned char[SPECIALARPLEN];
  arpcmd = new unsigned char[SPECIALARPLEN];

  return true;
}

void CmodPlayer::init_notetable(const unsigned short *newnotetable)
{
  memcpy(notetable, newnotetable, 12 * 2);
}

bool CmodPlayer::realloc_order(unsigned long len)
{
  if(order) delete [] order;
  order = new unsigned char[len];
  return true;
}

bool CmodPlayer::realloc_patterns(unsigned long pats, unsigned long rows, unsigned long chans)
{
  unsigned long i;

  dealloc_patterns();

  // set new number of tracks, rows and channels
  npats = pats; nrows = rows; nchans = chans;

  // alloc new patterns
  tracks = new Tracks *[pats * chans];
  for(i=0;i<pats*chans;i++) tracks[i] = new Tracks[rows];
  trackord = new unsigned short *[pats];
  for(i=0;i<pats;i++) trackord[i] = new unsigned short[chans];
  channel = new Channel[chans];

  // initialize new patterns
  for(i=0;i<pats*chans;i++) memset(tracks[i],0,sizeof(Tracks)*rows);
  for(i=0;i<pats;i++) memset(trackord[i],0,chans*2);

  return true;
}

void CmodPlayer::dealloc_patterns()
{
  unsigned long i;

  // dealloc everything previously allocated
  if(npats && nrows && nchans) {
    for(i=0;i<npats*nchans;i++) delete [] tracks[i];
    delete [] tracks;
    for(i=0;i<npats;i++) delete [] trackord[i];
    delete [] trackord;
    delete [] channel;
  }
}

bool CmodPlayer::realloc_instruments(unsigned long len)
{
  // dealloc previous instance, if any
  if(inst) delete [] inst;

  inst = new Instrument[len];
  memset(inst,0,sizeof(Instrument)*len);	// reset instruments
  return true;
}

void CmodPlayer::dealloc()
{
  if(inst) delete [] inst;
  if(order) delete [] order;
  if(arplist) delete [] arplist;
  if(arpcmd) delete [] arpcmd;
  dealloc_patterns();
}

/*** private methods *************************************/

void CmodPlayer::setvolume(unsigned char chan)
{
  unsigned char oplchan = set_opl_chip(chan);

  if(flags & Faust)
    setvolume_alt(chan);
  else {
    opl->write(0x40 + op_table[oplchan], 63-channel[chan].vol2 + (inst[channel[chan].inst].data[9] & 192));
    opl->write(0x43 + op_table[oplchan], 63-channel[chan].vol1 + (inst[channel[chan].inst].data[10] & 192));
  }
}

void CmodPlayer::setvolume_alt(unsigned char chan)
{
  unsigned char oplchan = set_opl_chip(chan);
  unsigned char ivol2 = inst[channel[chan].inst].data[9] & 63;
  unsigned char ivol1 = inst[channel[chan].inst].data[10] & 63;

  opl->write(0x40 + op_table[oplchan], (((63 - channel[chan].vol2 & 63) + ivol2) >> 1) + (inst[channel[chan].inst].data[9] & 192));
  opl->write(0x43 + op_table[oplchan], (((63 - channel[chan].vol1 & 63) + ivol1) >> 1) + (inst[channel[chan].inst].data[10] & 192));
}

void CmodPlayer::setfreq(unsigned char chan)
{
  unsigned char oplchan = set_opl_chip(chan);

  opl->write(0xa0 + oplchan, channel[chan].freq & 255);
  if(channel[chan].key)
    opl->write(0xb0 + oplchan, ((channel[chan].freq & 768) >> 8) + (channel[chan].oct << 2) | 32);
  else
    opl->write(0xb0 + oplchan, ((channel[chan].freq & 768) >> 8) + (channel[chan].oct << 2));
}

void CmodPlayer::playnote(unsigned char chan)
{
  unsigned char oplchan = set_opl_chip(chan);
  unsigned char op = op_table[oplchan], insnr = channel[chan].inst;

  if(!(flags & NoKeyOn))
    opl->write(0xb0 + oplchan, 0);	// stop old note

  // set instrument data
  opl->write(0x20 + op, inst[insnr].data[1]);
  opl->write(0x23 + op, inst[insnr].data[2]);
  opl->write(0x60 + op, inst[insnr].data[3]);
  opl->write(0x63 + op, inst[insnr].data[4]);
  opl->write(0x80 + op, inst[insnr].data[5]);
  opl->write(0x83 + op, inst[insnr].data[6]);
  opl->write(0xe0 + op, inst[insnr].data[7]);
  opl->write(0xe3 + op, inst[insnr].data[8]);
  opl->write(0xc0 + oplchan, inst[insnr].data[0]);
  opl->write(0xbd, inst[insnr].misc);	// set misc. register

  // set frequency, volume & play
  channel[chan].key = 1;
  setfreq(chan);

  if (flags & Faust) {
    channel[chan].vol2 = 63;
    channel[chan].vol1 = 63;
  }
  setvolume(chan);
}

void CmodPlayer::setnote(unsigned char chan, int note)
{
  if(note > 96)
    if(note == 127) {	// key off
      channel[chan].key = 0;
      setfreq(chan);
      return;
    } else
      note = 96;

  if(note < 13)
    channel[chan].freq = notetable[note - 1];
  else
    if(note % 12 > 0)
      channel[chan].freq = notetable[(note % 12) - 1];
    else
      channel[chan].freq = notetable[11];
  channel[chan].oct = (note - 1) / 12;
  channel[chan].freq += inst[channel[chan].inst].slide;	// apply pre-slide
}

void CmodPlayer::slide_down(unsigned char chan, int amount)
{
  channel[chan].freq -= amount;
  if(channel[chan].freq <= 342)
    if(channel[chan].oct) {
      channel[chan].oct--;
      channel[chan].freq <<= 1;
    } else
      channel[chan].freq = 342;
}

void CmodPlayer::slide_up(unsigned char chan, int amount)
{
  channel[chan].freq += amount;
  if(channel[chan].freq >= 686)
    if(channel[chan].oct < 7) {
      channel[chan].oct++;
      channel[chan].freq >>= 1;
    } else
      channel[chan].freq = 686;
}

void CmodPlayer::tone_portamento(unsigned char chan, unsigned char info)
{
  if(channel[chan].freq + (channel[chan].oct << 10) < channel[chan].nextfreq +
     (channel[chan].nextoct << 10)) {
    slide_up(chan,info);
    if(channel[chan].freq + (channel[chan].oct << 10) > channel[chan].nextfreq +
       (channel[chan].nextoct << 10)) {
      channel[chan].freq = channel[chan].nextfreq;
      channel[chan].oct = channel[chan].nextoct;
    }
  }
  if(channel[chan].freq + (channel[chan].oct << 10) > channel[chan].nextfreq +
     (channel[chan].nextoct << 10)) {
    slide_down(chan,info);
    if(channel[chan].freq + (channel[chan].oct << 10) < channel[chan].nextfreq +
       (channel[chan].nextoct << 10)) {
      channel[chan].freq = channel[chan].nextfreq;
      channel[chan].oct = channel[chan].nextoct;
    }
  }
  setfreq(chan);
}

void CmodPlayer::vibrato(unsigned char chan, unsigned char speed, unsigned char depth)
{
  int i;

  if(!speed || !depth)
    return;

  if(depth > 14)
    depth = 14;

  for(i=0;i<speed;i++) {
    channel[chan].trigger++;
    while(channel[chan].trigger >= 64)
      channel[chan].trigger -= 64;
    if(channel[chan].trigger >= 16 && channel[chan].trigger < 48)
      slide_down(chan,vibratotab[channel[chan].trigger - 16] / (16-depth));
    if(channel[chan].trigger < 16)
      slide_up(chan,vibratotab[channel[chan].trigger + 16] / (16-depth));
    if(channel[chan].trigger >= 48)
      slide_up(chan,vibratotab[channel[chan].trigger - 48] / (16-depth));
  }
  setfreq(chan);
}

void CmodPlayer::vol_up(unsigned char chan, int amount)
{
  if(channel[chan].vol1 + amount < 63)
    channel[chan].vol1 += amount;
  else
    channel[chan].vol1 = 63;

  if(channel[chan].vol2 + amount < 63)
    channel[chan].vol2 += amount;
  else
    channel[chan].vol2 = 63;
}

void CmodPlayer::vol_down(unsigned char chan, int amount)
{
  if(channel[chan].vol1 - amount > 0)
    channel[chan].vol1 -= amount;
  else
    channel[chan].vol1 = 0;

  if(channel[chan].vol2 - amount > 0)
    channel[chan].vol2 -= amount;
  else
    channel[chan].vol2 = 0;
}

void CmodPlayer::vol_up_alt(unsigned char chan, int amount)
{
  if(channel[chan].vol1 + amount < 63)
    channel[chan].vol1 += amount;
  else
    channel[chan].vol1 = 63;
  if(inst[channel[chan].inst].data[0] & 1)
    if(channel[chan].vol2 + amount < 63)
      channel[chan].vol2 += amount;
    else
      channel[chan].vol2 = 63;
}

void CmodPlayer::vol_down_alt(unsigned char chan, int amount)
{
  if(channel[chan].vol1 - amount > 0)
    channel[chan].vol1 -= amount;
  else
    channel[chan].vol1 = 0;
  if(inst[channel[chan].inst].data[0] & 1)
    if(channel[chan].vol2 - amount > 0)
      channel[chan].vol2 -= amount;
    else
      channel[chan].vol2 = 0;
}
