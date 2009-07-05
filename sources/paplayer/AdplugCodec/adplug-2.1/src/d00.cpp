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
 * d00.c - D00 Player by Simon Peter <dn.tlp@gmx.net>
 *
 * NOTES:
 * Sorry for the goto's, but the code looks so much nicer now.
 * I tried it with while loops but it was just a mess. If you
 * can come up with a nicer solution, just tell me.
 *
 * BUGS:
 * Hard restart SR is sometimes wrong
 */

#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "debug.h"
#include "d00.h"

#define HIBYTE(val)	(val >> 8)
#define LOBYTE(val)	(val & 0xff)

static const unsigned short notetable[12] =	// D00 note table
  {340,363,385,408,432,458,485,514,544,577,611,647};

static inline uint16_t LE_WORD(const uint16_t *val)
{
  const uint8_t *b = (const uint8_t *)val;
  return (b[1] << 8) + b[0];
}

/*** public methods *************************************/

CPlayer *Cd00Player::factory(Copl *newopl)
{
  return new Cd00Player(newopl);
}

bool Cd00Player::load(const std::string &filename, const CFileProvider &fp)
{
  binistream	*f = fp.open(filename); if(!f) return false;
  d00header	*checkhead;
  d00header1	*ch;
  unsigned long	filesize;
  int		i,ver1=0;
  char		*str;

  // file validation section
  checkhead = new d00header;
  f->readString((char *)checkhead, sizeof(d00header));

  // Check for version 2-4 header
  if(strncmp(checkhead->id,"JCH\x26\x02\x66",6) || checkhead->type ||
     !checkhead->subsongs || checkhead->soundcard) {
    // Check for version 0 or 1 header (and .d00 file extension)
    delete checkhead;
    if(!fp.extension(filename, ".d00")) { fp.close(f); return false; }
    ch = new d00header1;
    f->seek(0); f->readString((char *)ch, sizeof(d00header1));
    if(ch->version > 1 || !ch->subsongs)
      { delete ch; fp.close(f); return false; }
    delete ch;
    ver1 = 1;
  } else
    delete checkhead;

  AdPlug_LogWrite("Cd00Player::load(f,\"%s\"): %s format D00 file detected!\n",
		  filename.c_str(), ver1 ? "Old" : "New");

  // load section
  filesize = fp.filesize(f); f->seek(0);
  filedata = new char [filesize + 1];			// 1 byte is needed for old-style DataInfo block
  f->readString((char *)filedata, filesize);
  fp.close(f);
  if(!ver1) {	// version 2 and above
    header = (struct d00header *)filedata;
    version = header->version;
    datainfo = (char *)filedata + LE_WORD(&header->infoptr);
    inst = (struct Sinsts *)((char *)filedata + LE_WORD(&header->instptr));
    seqptr = (unsigned short *)((char *)filedata + LE_WORD(&header->seqptr));
    for(i=31;i>=0;i--)	// erase whitespace
      if(header->songname[i] == ' ')
	header->songname[i] = '\0';
      else
	break;
    for(i=31;i>=0;i--)
      if(header->author[i] == ' ')
	header->author[i] = '\0';
      else
	break;
  } else {	// version 1
    header1 = (struct d00header1 *)filedata;
    version = header1->version;
    datainfo = (char *)filedata + LE_WORD(&header1->infoptr);
    inst = (struct Sinsts *)((char *)filedata + LE_WORD(&header1->instptr));
    seqptr = (unsigned short *)((char *)filedata + LE_WORD(&header1->seqptr));
  }
  switch(version) {
  case 0:
    levpuls = 0;
    spfx = 0;
    header1->speed = 70;		// v0 files default to 70Hz
    break;
  case 1:
    levpuls = (struct Slevpuls *)((char *)filedata + LE_WORD(&header1->lpulptr));
    spfx = 0;
    break;
  case 2:
    levpuls = (struct Slevpuls *)((char *)filedata + LE_WORD(&header->spfxptr));
    spfx = 0;
    break;
  case 3:
    spfx = 0;
    levpuls = 0;
    break;
  case 4:
    spfx = (struct Sspfx *)((char *)filedata + LE_WORD(&header->spfxptr));
    levpuls = 0;
    break;
  }
  if((str = strstr(datainfo,"\xff\xff")))
    while((*str == '\xff' || *str == ' ') && str >= datainfo) {
      *str = '\0'; str--;
    }
  else	// old-style block
    memset((char *)filedata+filesize,0,1);

  rewind(0);
  return true;
}

bool Cd00Player::update()
{
  unsigned char	c,cnt,trackend=0,fx,note;
  unsigned short ord,*patt,buf,fxop,pattpos;

  // effect handling (timer dependant)
  for(c=0;c<9;c++) {
    channel[c].slideval += channel[c].slide; setfreq(c);	// sliding
    vibrato(c);	// vibrato

    if(channel[c].spfx != 0xffff) {	// SpFX
      if(channel[c].fxdel)
	channel[c].fxdel--;
      else {
	channel[c].spfx = LE_WORD(&spfx[channel[c].spfx].ptr);
	channel[c].fxdel = spfx[channel[c].spfx].duration;
	channel[c].inst = LE_WORD(&spfx[channel[c].spfx].instnr) & 0xfff;
	if(spfx[channel[c].spfx].modlev != 0xff)
	  channel[c].modvol = spfx[channel[c].spfx].modlev;
	setinst(c);
	if(LE_WORD(&spfx[channel[c].spfx].instnr) & 0x8000)	// locked frequency
	  note = spfx[channel[c].spfx].halfnote;
	else							// unlocked frequency
	  note = spfx[channel[c].spfx].halfnote + channel[c].note;
	channel[c].freq = notetable[note%12] + ((note/12) << 10);
	setfreq(c);
      }
      channel[c].modvol += spfx[channel[c].spfx].modlevadd; channel[c].modvol &= 63;
      setvolume(c);
    }

    if(channel[c].levpuls != 0xff)	// Levelpuls
      if(channel[c].frameskip)
	channel[c].frameskip--;
      else {
	channel[c].frameskip = inst[channel[c].inst].timer;
	if(channel[c].fxdel)
	  channel[c].fxdel--;
	else {
	  channel[c].levpuls = levpuls[channel[c].levpuls].ptr - 1;
	  channel[c].fxdel = levpuls[channel[c].levpuls].duration;
	  if(levpuls[channel[c].levpuls].level != 0xff)
	    channel[c].modvol = levpuls[channel[c].levpuls].level;
	}
	channel[c].modvol += levpuls[channel[c].levpuls].voladd; channel[c].modvol &= 63;
	setvolume(c);
      }
  }

  // song handling
  for(c=0;c<9;c++)
    if(version < 3 ? channel[c].del : channel[c].del <= 0x7f) {
      if(version == 4)	// v4: hard restart SR
	if(channel[c].del == inst[channel[c].inst].timer)
	  if(channel[c].nextnote)
	    opl->write(0x83 + op_table[c], inst[channel[c].inst].sr);
      if(version < 3)
	channel[c].del--;
      else
	if(channel[c].speed)
	  channel[c].del += channel[c].speed;
	else {
	  channel[c].seqend = 1;
	  continue;
	}
    } else {
      if(channel[c].speed) {
	if(version < 3)
	  channel[c].del = channel[c].speed;
	else {
	  channel[c].del &= 0x7f;
	  channel[c].del += channel[c].speed;
	}
      } else {
	channel[c].seqend = 1;
	continue;
      }
      if(channel[c].rhcnt) {	// process pending REST/HOLD events
	channel[c].rhcnt--;
	continue;
      }
    readorder:	// process arrangement (orderlist)
      ord = LE_WORD(&channel[c].order[channel[c].ordpos]);
      switch(ord) {
      case 0xfffe: channel[c].seqend = 1; continue;	// end of arrangement stream
      case 0xffff:		// jump to order
	channel[c].ordpos = LE_WORD(&channel[c].order[channel[c].ordpos + 1]);
	channel[c].seqend = 1;
	goto readorder;
      default:
	if(ord >= 0x9000) {	// set speed
	  channel[c].speed = ord & 0xff;
	  ord = LE_WORD(&channel[c].order[channel[c].ordpos - 1]);
	  channel[c].ordpos++;
	} else
	  if(ord >= 0x8000) {	// transpose track
	    channel[c].transpose = ord & 0xff;
	    if(ord & 0x100)
	      channel[c].transpose = -channel[c].transpose;
	    ord = LE_WORD(&channel[c].order[++channel[c].ordpos]);
	  }
	patt = (unsigned short *)((char *)filedata + LE_WORD(&seqptr[ord]));
	break;
      }
    channel[c].fxflag = 0;
    readseq:	// process sequence (pattern)
      if(!version)	// v0: always initialize rhcnt
	channel[c].rhcnt = channel[c].irhcnt;
      pattpos = LE_WORD(&patt[channel[c].pattpos]);
      if(pattpos == 0xffff) {	// pattern ended?
	channel[c].pattpos = 0;
	channel[c].ordpos++;
	goto readorder;
      }
      cnt = HIBYTE(pattpos);
      note = LOBYTE(pattpos);
      fx = pattpos >> 12;
      fxop = pattpos & 0x0fff;
      channel[c].pattpos++; pattpos = LE_WORD(&patt[channel[c].pattpos]);
      channel[c].nextnote = LOBYTE(pattpos) & 0x7f;
      if(version ? cnt < 0x40 : !fx) {	// note event
	switch(note) {
	case 0:						// REST event
	case 0x80:
	  if(!note || version) {
	    channel[c].key = 0;
	    setfreq(c);
	  }
	  // fall through...
	case 0x7e:					// HOLD event
	  if(version)
	    channel[c].rhcnt = cnt;
	  channel[c].nextnote = 0;
	  break;
	default:					// play note
	  // restart fx
	  if(!(channel[c].fxflag & 1))
	    channel[c].vibdepth = 0;
	  if(!(channel[c].fxflag & 2))
	    channel[c].slideval = channel[c].slide = 0;

	  if(version) {	// note handling for v1 and above
	    if(note > 0x80)	// locked note (no channel transpose)
	      note -= 0x80;
	    else			// unlocked note
	      note += channel[c].transpose;
	    channel[c].note = note;	// remember note for SpFX

	    if(channel[c].ispfx != 0xffff && cnt < 0x20) {	// reset SpFX
	      channel[c].spfx = channel[c].ispfx;
	      if(LE_WORD(&spfx[channel[c].spfx].instnr) & 0x8000)	// locked frequency
		note = spfx[channel[c].spfx].halfnote;
	      else												// unlocked frequency
		note += spfx[channel[c].spfx].halfnote;
	      channel[c].inst = LE_WORD(&spfx[channel[c].spfx].instnr) & 0xfff;
	      channel[c].fxdel = spfx[channel[c].spfx].duration;
	      if(spfx[channel[c].spfx].modlev != 0xff)
		channel[c].modvol = spfx[channel[c].spfx].modlev;
	      else
		channel[c].modvol = inst[channel[c].inst].data[7] & 63;
	    }

	    if(channel[c].ilevpuls != 0xff && cnt < 0x20) {	// reset LevelPuls
	      channel[c].levpuls = channel[c].ilevpuls;
	      channel[c].fxdel = levpuls[channel[c].levpuls].duration;
	      channel[c].frameskip = inst[channel[c].inst].timer;
	      if(levpuls[channel[c].levpuls].level != 0xff)
		channel[c].modvol = levpuls[channel[c].levpuls].level;
	      else
		channel[c].modvol = inst[channel[c].inst].data[7] & 63;
	    }

	    channel[c].freq = notetable[note%12] + ((note/12) << 10);
	    if(cnt < 0x20)	// normal note
	      playnote(c);
	    else {			// tienote
	      setfreq(c);
	      cnt -= 0x20;	// make count proper
	    }
	    channel[c].rhcnt = cnt;
	  } else {	// note handling for v0
	    if(cnt < 2)	// unlocked note
	      note += channel[c].transpose;
	    channel[c].note = note;

	    channel[c].freq = notetable[note%12] + ((note/12) << 10);
	    if(cnt == 1)	// tienote
	      setfreq(c);
	    else			// normal note
	      playnote(c);
	  }
	  break;
	}
	continue;	// event is complete
      } else {		// effect event
	switch(fx) {
	case 6:		// Cut/Stop Voice
	  buf = channel[c].inst;
	  channel[c].inst = 0;
	  playnote(c);
	  channel[c].inst = buf;
	  channel[c].rhcnt = fxop;
	  continue;	// no note follows this event
	case 7:		// Vibrato
	  channel[c].vibspeed = fxop & 0xff;
	  channel[c].vibdepth = fxop >> 8;
	  channel[c].trigger = fxop >> 9;
	  channel[c].fxflag |= 1;
	  break;
	case 8:		// v0: Duration
	  if(!version)
	    channel[c].irhcnt = fxop;
	  break;
	case 9:		// New Level
	  channel[c].vol = fxop & 63;
	  if(channel[c].vol + channel[c].cvol < 63)	// apply channel volume
	    channel[c].vol += channel[c].cvol;
	  else
	    channel[c].vol = 63;
	  setvolume(c);
	  break;
	case 0xb:	// v4: Set SpFX
	  if(version == 4)
	    channel[c].ispfx = fxop;
	  break;
	case 0xc:	// Set Instrument
	  channel[c].ispfx = 0xffff;
	  channel[c].spfx = 0xffff;
	  channel[c].inst = fxop;
	  channel[c].modvol = inst[fxop].data[7] & 63;
	  if(version < 3 && version && inst[fxop].tunelev)	// Set LevelPuls
	    channel[c].ilevpuls = inst[fxop].tunelev - 1;
	  else {
	    channel[c].ilevpuls = 0xff;
	    channel[c].levpuls = 0xff;
	  }
	  break;
	case 0xd:	// Slide up
	  channel[c].slide = fxop;
	  channel[c].fxflag |= 2;
	  break;
	case 0xe:	// Slide down
	  channel[c].slide = -fxop;
	  channel[c].fxflag |= 2;
	  break;
	}
	goto readseq;	// event is incomplete, note follows
      }
    }

  for(c=0;c<9;c++)
    if(channel[c].seqend)
      trackend++;
  if(trackend == 9)
    songend = 1;

  return !songend;
}

void Cd00Player::rewind(int subsong)
{
  struct Stpoin {
    unsigned short ptr[9];
    unsigned char volume[9],dummy[5];
  } *tpoin;
  int i;

  if(version > 1) {	// do nothing if subsong > number of subsongs
    if(subsong >= header->subsongs)
      return;
  } else
    if(subsong >= header1->subsongs)
      return;

  memset(channel,0,sizeof(channel));
  if(version > 1)
    tpoin = (struct Stpoin *)((char *)filedata + LE_WORD(&header->tpoin));
  else
    tpoin = (struct Stpoin *)((char *)filedata + LE_WORD(&header1->tpoin));
  for(i=0;i<9;i++) {
    if(LE_WORD(&tpoin[subsong].ptr[i])) {	// track enabled
      channel[i].speed = LE_WORD((unsigned short *)
				 ((char *)filedata + LE_WORD(&tpoin[subsong].ptr[i])));
      channel[i].order = (unsigned short *)
	((char *)filedata + LE_WORD(&tpoin[subsong].ptr[i]) + 2);
    } else {					// track disabled
      channel[i].speed = 0;
      channel[i].order = 0;
    }
    channel[i].ispfx = 0xffff; channel[i].spfx = 0xffff;	// no SpFX
    channel[i].ilevpuls = 0xff; channel[i].levpuls = 0xff;	// no LevelPuls
    channel[i].cvol = tpoin[subsong].volume[i] & 0x7f;	// our player may savely ignore bit 7
    channel[i].vol = channel[i].cvol;			// initialize volume
  }
  songend = 0;
  opl->init(); opl->write(1,32);	// reset OPL chip
}

std::string Cd00Player::gettype()
{
  char	tmpstr[40];

  sprintf(tmpstr,"EdLib packed (version %d)",version > 1 ? header->version : header1->version);
  return std::string(tmpstr);
}

float Cd00Player::getrefresh()
{
  if(version > 1)
    return header->speed;
  else
    return header1->speed;
}

unsigned int Cd00Player::getsubsongs()
{
  if(version <= 1)	// return number of subsongs
    return header1->subsongs;
  else
    return header->subsongs;
}

/*** private methods *************************************/

void Cd00Player::setvolume(unsigned char chan)
{
  unsigned char	op = op_table[chan];
  unsigned short	insnr = channel[chan].inst;

  opl->write(0x43 + op,(int)(63-((63-(inst[insnr].data[2] & 63))/63.0)*(63-channel[chan].vol)) +
	     (inst[insnr].data[2] & 192));
  if(inst[insnr].data[10] & 1)
    opl->write(0x40 + op,(int)(63-((63-channel[chan].modvol)/63.0)*(63-channel[chan].vol)) +
	       (inst[insnr].data[7] & 192));
  else
    opl->write(0x40 + op,channel[chan].modvol + (inst[insnr].data[7] & 192));
}

void Cd00Player::setfreq(unsigned char chan)
{
  unsigned short freq = channel[chan].freq;

  if(version == 4)	// v4: apply instrument finetune
    freq += inst[channel[chan].inst].tunelev;

  freq += channel[chan].slideval;
  opl->write(0xa0 + chan, freq & 255);
  if(channel[chan].key)
    opl->write(0xb0 + chan, ((freq >> 8) & 31) | 32);
  else
    opl->write(0xb0 + chan, (freq >> 8) & 31);
}

void Cd00Player::setinst(unsigned char chan)
{
  unsigned char	op = op_table[chan];
  unsigned short	insnr = channel[chan].inst;

  // set instrument data
  opl->write(0x63 + op, inst[insnr].data[0]);
  opl->write(0x83 + op, inst[insnr].data[1]);
  opl->write(0x23 + op, inst[insnr].data[3]);
  opl->write(0xe3 + op, inst[insnr].data[4]);
  opl->write(0x60 + op, inst[insnr].data[5]);
  opl->write(0x80 + op, inst[insnr].data[6]);
  opl->write(0x20 + op, inst[insnr].data[8]);
  opl->write(0xe0 + op, inst[insnr].data[9]);
  if(version)
    opl->write(0xc0 + chan, inst[insnr].data[10]);
  else
    opl->write(0xc0 + chan, (inst[insnr].data[10] << 1) + (inst[insnr].tunelev & 1));
}

void Cd00Player::playnote(unsigned char chan)
{
  // set misc vars & play
  opl->write(0xb0 + chan, 0);	// stop old note
  setinst(chan);
  channel[chan].key = 1;
  setfreq(chan);
  setvolume(chan);
}

void Cd00Player::vibrato(unsigned char chan)
{
  if(!channel[chan].vibdepth)
    return;

  if(channel[chan].trigger)
    channel[chan].trigger--;
  else {
    channel[chan].trigger = channel[chan].vibdepth;
    channel[chan].vibspeed = -channel[chan].vibspeed;
  }
  channel[chan].freq += channel[chan].vibspeed;
  setfreq(chan);
}
