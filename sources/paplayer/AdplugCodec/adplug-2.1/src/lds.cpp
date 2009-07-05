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
 * lds.cpp - LOUDNESS Player by Simon Peter <dn.tlp@gmx.net>
 */

#include <string.h>

#include "lds.h"
#include "debug.h"

// Note frequency table (16 notes / octave)
const unsigned short CldsPlayer::frequency[] = {
  343, 344, 345, 347, 348, 349, 350, 352, 353, 354, 356, 357, 358,
  359, 361, 362, 363, 365, 366, 367, 369, 370, 371, 373, 374, 375,
  377, 378, 379, 381, 382, 384, 385, 386, 388, 389, 391, 392, 393,
  395, 396, 398, 399, 401, 402, 403, 405, 406, 408, 409, 411, 412,
  414, 415, 417, 418, 420, 421, 423, 424, 426, 427, 429, 430, 432,
  434, 435, 437, 438, 440, 442, 443, 445, 446, 448, 450, 451, 453,
  454, 456, 458, 459, 461, 463, 464, 466, 468, 469, 471, 473, 475,
  476, 478, 480, 481, 483, 485, 487, 488, 490, 492, 494, 496, 497,
  499, 501, 503, 505, 506, 508, 510, 512, 514, 516, 518, 519, 521,
  523, 525, 527, 529, 531, 533, 535, 537, 538, 540, 542, 544, 546,
  548, 550, 552, 554, 556, 558, 560, 562, 564, 566, 568, 571, 573,
  575, 577, 579, 581, 583, 585, 587, 589, 591, 594, 596, 598, 600,
  602, 604, 607, 609, 611, 613, 615, 618, 620, 622, 624, 627, 629,
  631, 633, 636, 638, 640, 643, 645, 647, 650, 652, 654, 657, 659,
  662, 664, 666, 669, 671, 674, 676, 678, 681, 683
};

// Vibrato (sine) table
const unsigned char CldsPlayer::vibtab[] = {
  0, 13, 25, 37, 50, 62, 74, 86, 98, 109, 120, 131, 142, 152, 162,
  171, 180, 189, 197, 205, 212, 219, 225, 231, 236, 240, 244, 247,
  250, 252, 254, 255, 255, 255, 254, 252, 250, 247, 244, 240, 236,
  231, 225, 219, 212, 205, 197, 189, 180, 171, 162, 152, 142, 131,
  120, 109, 98, 86, 74, 62, 50, 37, 25, 13
};

// Tremolo (sine * sine) table
const unsigned char CldsPlayer::tremtab[] = {
  0, 0, 1, 1, 2, 4, 5, 7, 10, 12, 15, 18, 21, 25, 29, 33, 37, 42, 47,
  52, 57, 62, 67, 73, 79, 85, 90, 97, 103, 109, 115, 121, 128, 134,
  140, 146, 152, 158, 165, 170, 176, 182, 188, 193, 198, 203, 208,
  213, 218, 222, 226, 230, 234, 237, 240, 243, 245, 248, 250, 251,
  253, 254, 254, 255, 255, 255, 254, 254, 253, 251, 250, 248, 245,
  243, 240, 237, 234, 230, 226, 222, 218, 213, 208, 203, 198, 193,
  188, 182, 176, 170, 165, 158, 152, 146, 140, 134, 127, 121, 115,
  109, 103, 97, 90, 85, 79, 73, 67, 62, 57, 52, 47, 42, 37, 33, 29,
  25, 21, 18, 15, 12, 10, 7, 5, 4, 2, 1, 1, 0
};

// 'maxsound' is maximum number of patches (instruments)
// 'maxpos' is maximum number of entries in position list (orderlist)
const unsigned short CldsPlayer::maxsound = 0x3f, CldsPlayer::maxpos = 0xff;

/*** public methods *************************************/

CldsPlayer::CldsPlayer(Copl *newopl)
  : CPlayer(newopl), soundbank(0), positions(0), patterns(0)
{
}

CldsPlayer::~CldsPlayer()
{
  if(soundbank) delete [] soundbank;
  if(positions) delete [] positions;
  if(patterns) delete [] patterns;
}

bool CldsPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream	*f;
  unsigned int	i, j;
  SoundBank	*sb;

  // file validation section (actually just an extension check)
  if(!fp.extension(filename, ".lds")) return false;
  f = fp.open(filename); if(!f) return false;

  // file load section (header)
  mode = f->readInt(1);
  if(mode > 2) { fp.close(f); return false; }
  speed = f->readInt(2);
  tempo = f->readInt(1);
  pattlen = f->readInt(1);
  for(i = 0; i < 9; i++) chandelay[i] = f->readInt(1);
  regbd = f->readInt(1);

  // load patches
  numpatch = f->readInt(2);
  soundbank = new SoundBank[numpatch];
  for(i = 0; i < numpatch; i++) {
    sb = &soundbank[i];
    sb->mod_misc = f->readInt(1); sb->mod_vol = f->readInt(1);
    sb->mod_ad = f->readInt(1); sb->mod_sr = f->readInt(1);
    sb->mod_wave = f->readInt(1); sb->car_misc = f->readInt(1);
    sb->car_vol = f->readInt(1); sb->car_ad = f->readInt(1);
    sb->car_sr = f->readInt(1); sb->car_wave = f->readInt(1);
    sb->feedback = f->readInt(1); sb->keyoff = f->readInt(1);
    sb->portamento = f->readInt(1); sb->glide = f->readInt(1);
    sb->finetune = f->readInt(1); sb->vibrato = f->readInt(1);
    sb->vibdelay = f->readInt(1); sb->mod_trem = f->readInt(1);
    sb->car_trem = f->readInt(1); sb->tremwait = f->readInt(1);
    sb->arpeggio = f->readInt(1);
    for(j = 0; j < 12; j++) sb->arp_tab[j] = f->readInt(1);
    sb->start = f->readInt(2); sb->size = f->readInt(2);
    sb->fms = f->readInt(1); sb->transp = f->readInt(2);
    sb->midinst = f->readInt(1); sb->midvelo = f->readInt(1);
    sb->midkey = f->readInt(1); sb->midtrans = f->readInt(1);
    sb->middum1 = f->readInt(1); sb->middum2 = f->readInt(1);
  }

  // load positions
  numposi = f->readInt(2);
  positions = new Position[9 * numposi];
  for(i = 0; i < numposi; i++)
    for(j = 0; j < 9; j++) {
      /*
       * patnum is a pointer inside the pattern space, but patterns are 16bit
       * word fields anyway, so it ought to be an even number (hopefully) and
       * we can just divide it by 2 to get our array index of 16bit words.
       */
      positions[i * 9 + j].patnum = f->readInt(2) / 2;
      positions[i * 9 + j].transpose = f->readInt(1);
    }

  AdPlug_LogWrite("CldsPlayer::load(\"%s\",fp): loading LOUDNESS file: mode = "
		  "%d, pattlen = %d, numpatch = %d, numposi = %d\n",
		  filename.c_str(), mode, pattlen, numpatch, numposi);

  // load patterns
  f->ignore(2);		// ignore # of digital sounds (not played by this player)
  patterns = new unsigned short[(fp.filesize(f) - f->pos()) / 2 + 1];
  for(i = 0; !f->eof(); i++)
    patterns[i] = f->readInt(2);

  fp.close(f);
  rewind(0);
  return true;
}

bool CldsPlayer::update()
{
  unsigned short	comword, freq, octave, chan, tune, wibc, tremc, arpreg;
  bool			vbreak;
  unsigned char		level, regnum, comhi, comlo;
  int			i;
  Channel		*c;

  if(!playing) return false;

  // handle fading
  if(fadeonoff)
    if(fadeonoff <= 128) {
      if(allvolume > fadeonoff || allvolume == 0)
	allvolume -= fadeonoff;
      else {
	allvolume = 1;
	fadeonoff = 0;
	if(hardfade != 0) {
	  playing = false;
	  hardfade = 0;
	  for(i = 0; i < 9; i++)
	    channel[i].keycount = 1;
	}
      }
    } else
      if(((allvolume + (0x100 - fadeonoff)) & 0xff) <= mainvolume)
	allvolume += 0x100 - fadeonoff;
      else {
	allvolume = mainvolume;
	fadeonoff = 0;
      }

  // handle channel delay
  for(chan = 0; chan < 9; chan++) {
    c = &channel[chan];
    if(c->chancheat.chandelay)
      if(!(--c->chancheat.chandelay))
	playsound(c->chancheat.sound, chan, c->chancheat.high);
  }

  // handle notes
  if(!tempo_now) {
    vbreak = false;
    for(chan = 0; chan < 9; chan++) {
      c = &channel[chan];
      if(!c->packwait) {
	unsigned short	patnum = positions[posplay * 9 + chan].patnum;
	unsigned char	transpose = positions[posplay * 9 + chan].transpose;

	comword = patterns[patnum + c->packpos];
	comhi = comword >> 8; comlo = comword & 0xff;
	if(comword)
	  if(comhi == 0x80)
	    c->packwait = comlo;
	  else
	    if(comhi >= 0x80) {
	      switch(comhi) {
	      case 0xff:
		c->volcar = (((c->volcar & 0x3f) * comlo) >> 6) & 0x3f;
		if(fmchip[0xc0 + chan] & 1)
		  c->volmod = (((c->volmod & 0x3f) * comlo) >> 6) & 0x3f;
		break;
	      case 0xfe:
		tempo = comword & 0x3f;
		break;
	      case 0xfd:
		c->nextvol = comlo;
		break;
	      case 0xfc:
		playing = false;
		// in real player there's also full keyoff here, but we don't need it
		break;
	      case 0xfb:
		c->keycount = 1;
		break;
	      case 0xfa:
		vbreak = true;
		jumppos = (posplay + 1) & maxpos;
		break;
	      case 0xf9:
		vbreak = true;
		jumppos = comlo & maxpos;
		jumping = 1;
		if(jumppos < posplay) songlooped = true;
		break;
	      case 0xf8:
		c->lasttune = 0;
		break;
	      case 0xf7:
		c->vibwait = 0;
		// PASCAL: c->vibspeed = ((comlo >> 4) & 15) + 2;
		c->vibspeed = (comlo >> 4) + 2;
		c->vibrate = (comlo & 15) + 1;
		break;
	      case 0xf6:
		c->glideto = comlo;
		break;
	      case 0xf5:
		c->finetune = comlo;
		break;
	      case 0xf4:
		if(!hardfade) {
		  allvolume = mainvolume = comlo;
		  fadeonoff = 0;
		}
		break;
	      case 0xf3:
		if(!hardfade) fadeonoff = comlo;
		break;
	      case 0xf2:
		c->trmstay = comlo;
		break;
	      case 0xf1:	// panorama
	      case 0xf0:	// progch
		// MIDI commands (unhandled)
		AdPlug_LogWrite("CldsPlayer(): not handling MIDI command 0x%x, "
				"value = 0x%x\n", comhi);
		break;
	      default:
		if(comhi < 0xa0)
		  c->glideto = comhi & 0x1f;
		else
		  AdPlug_LogWrite("CldsPlayer(): unknown command 0x%x encountered!"
				  " value = 0x%x\n", comhi, comlo);
		break;
	      }
	    } else {
	      unsigned char	sound;
	      unsigned short	high;
	      signed char	transp = transpose & 127;

	      /*
	       * Originally, in assembler code, the player first shifted
	       * logically left the transpose byte by 1 and then shifted
	       * arithmetically right the same byte to achieve the final,
	       * signed transpose value. Since we can't do arithmetic shifts
	       * in C, we just duplicate the 7th bit into the 8th one and
	       * discard the 8th one completely.
	       */

	      if(transpose & 64) transp |= 128;

	      if(transpose & 128) {
		sound = (comlo + transp) & maxsound;
		high = comhi << 4;
	      } else {
		sound = comlo & maxsound;
		high = (comhi + transp) << 4;
	      }

	      /*
		PASCAL:
	      sound = comlo & maxsound;
	      high = (comhi + (((transpose + 0x24) & 0xff) - 0x24)) << 4;
	      */

	      if(!chandelay[chan])
		playsound(sound, chan, high);
	      else {
		c->chancheat.chandelay = chandelay[chan];
		c->chancheat.sound = sound;
		c->chancheat.high = high;
	      }
	    }

	c->packpos++;
      } else
	c->packwait--;
    }

    tempo_now = tempo;
    /*
      The continue table is updated here, but this is only used in the
      original player, which can be paused in the middle of a song and then
      unpaused. Since AdPlug does all this for us automatically, we don't
      have a continue table here. The continue table update code is noted
      here for reference only.

      if(!pattplay) {
        conttab[speed & maxcont].position = posplay & 0xff;
        conttab[speed & maxcont].tempo = tempo;
      }
    */
    pattplay++;
    if(vbreak) {
      pattplay = 0;
      for(i = 0; i < 9; i++) channel[i].packpos = channel[i].packwait = 0;
      posplay = jumppos;
    } else
      if(pattplay >= pattlen) {
	pattplay = 0;
	for(i = 0; i < 9; i++) channel[i].packpos = channel[i].packwait = 0;
	posplay = (posplay + 1) & maxpos;
      }
  } else
    tempo_now--;

  // make effects
  for(chan = 0; chan < 9; chan++) {
    c = &channel[chan];
    regnum = op_table[chan];
    if(c->keycount > 0) {
      if(c->keycount == 1)
	setregs_adv(0xb0 + chan, 0xdf, 0);
      c->keycount--;
    }

    // arpeggio
    if(c->arp_size == 0)
      arpreg = 0;
    else {
      arpreg = c->arp_tab[c->arp_pos] << 4;
      if(arpreg == 0x800) {
	if(c->arp_pos > 0) c->arp_tab[0] = c->arp_tab[c->arp_pos - 1];
	c->arp_size = 1; c->arp_pos = 0;
	arpreg = c->arp_tab[0] << 4;
      }

      if(c->arp_count == c->arp_speed) {
	c->arp_pos++;
	if(c->arp_pos >= c->arp_size) c->arp_pos = 0;
	c->arp_count = 0;
      } else
	c->arp_count++;
    }

    // glide & portamento
    if(c->lasttune && (c->lasttune != c->gototune)) {
      if(c->lasttune > c->gototune) {
	if(c->lasttune - c->gototune < c->portspeed)
	  c->lasttune = c->gototune;
	else
	  c->lasttune -= c->portspeed;
      } else {
	if(c->gototune - c->lasttune < c->portspeed)
	  c->lasttune = c->gototune;
	else
	  c->lasttune += c->portspeed;
      }

      if(arpreg >= 0x800)
	arpreg = c->lasttune - (arpreg ^ 0xff0) - 16;
      else
	arpreg += c->lasttune;

      freq = frequency[arpreg % (12 * 16)];
      octave = arpreg / (12 * 16) - 1;
      setregs(0xa0 + chan, freq & 0xff);
      setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
    } else {
      // vibrato
      if(!c->vibwait) {
	if(c->vibrate) {
	  wibc = vibtab[c->vibcount & 0x3f] * c->vibrate;

	  if((c->vibcount & 0x40) == 0)
	    tune = c->lasttune + (wibc >> 8);
	  else
	    tune = c->lasttune - (wibc >> 8);

	  if(arpreg >= 0x800)
	    tune = tune - (arpreg ^ 0xff0) - 16;
	  else
	    tune += arpreg;

	  freq = frequency[tune % (12 * 16)];
	  octave = tune / (12 * 16) - 1;
	  setregs(0xa0 + chan, freq & 0xff);
	  setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
	  c->vibcount += c->vibspeed;
	} else
	  if(c->arp_size != 0) {	// no vibrato, just arpeggio
	    if(arpreg >= 0x800)
	      tune = c->lasttune - (arpreg ^ 0xff0) - 16;
	    else
	      tune = c->lasttune + arpreg;

	    freq = frequency[tune % (12 * 16)];
	    octave = tune / (12 * 16) - 1;
	    setregs(0xa0 + chan, freq & 0xff);
	    setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
	  }
      } else {	// no vibrato, just arpeggio
	c->vibwait--;

	if(c->arp_size != 0) {
	  if(arpreg >= 0x800)
	    tune = c->lasttune - (arpreg ^ 0xff0) - 16;
	  else
	    tune = c->lasttune + arpreg;

	  freq = frequency[tune % (12 * 16)];
	  octave = tune / (12 * 16) - 1;
	  setregs(0xa0 + chan, freq & 0xff);
	  setregs_adv(0xb0 + chan, 0x20, ((octave << 2) + (freq >> 8)) & 0xdf);
	}
      }
    }

    // tremolo (modulator)
    if(!c->trmwait) {
      if(c->trmrate) {
	tremc = tremtab[c->trmcount & 0x7f] * c->trmrate;
	if((tremc >> 8) <= (c->volmod & 0x3f))
	  level = (c->volmod & 0x3f) - (tremc >> 8);
	else
	  level = 0;

	if(allvolume != 0 && (fmchip[0xc0 + chan] & 1))
	  setregs_adv(0x40 + regnum, 0xc0, ((level * allvolume) >> 8) ^ 0x3f);
	else
	  setregs_adv(0x40 + regnum, 0xc0, level ^ 0x3f);

	c->trmcount += c->trmspeed;
      } else
	if(allvolume != 0 && (fmchip[0xc0 + chan] & 1))
	  setregs_adv(0x40 + regnum, 0xc0, ((((c->volmod & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
	else
	  setregs_adv(0x40 + regnum, 0xc0, (c->volmod ^ 0x3f) & 0x3f);
    } else {
      c->trmwait--;
      if(allvolume != 0 && (fmchip[0xc0 + chan] & 1))
	setregs_adv(0x40 + regnum, 0xc0, ((((c->volmod & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
    }

    // tremolo (carrier)
    if(!c->trcwait) {
      if(c->trcrate) {
	tremc = tremtab[c->trccount & 0x7f] * c->trcrate;
	if((tremc >> 8) <= (c->volcar & 0x3f))
	  level = (c->volcar & 0x3f) - (tremc >> 8);
	else
	  level = 0;

	if(allvolume != 0)
	  setregs_adv(0x43 + regnum, 0xc0, ((level * allvolume) >> 8) ^ 0x3f);
	else
	  setregs_adv(0x43 + regnum, 0xc0, level ^ 0x3f);
	c->trccount += c->trcspeed;
      } else
	if(allvolume != 0)
	  setregs_adv(0x43 + regnum, 0xc0, ((((c->volcar & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
	else
	  setregs_adv(0x43 + regnum, 0xc0, (c->volcar ^ 0x3f) & 0x3f);
    } else {
      c->trcwait--;
      if(allvolume != 0)
	setregs_adv(0x43 + regnum, 0xc0, ((((c->volcar & 0x3f) * allvolume) >> 8) ^ 0x3f) & 0x3f);
    }
  }

  return (!playing || songlooped) ? false : true;
}

void CldsPlayer::rewind(int subsong)
{
  int i;

  // init all with 0
  tempo_now = 3; playing = true; songlooped = false;
  jumping = fadeonoff = allvolume = hardfade = pattplay = posplay = jumppos =
    mainvolume = 0;
  memset(channel, 0, sizeof(channel));
  memset(fmchip, 0, sizeof(fmchip));

  // OPL2 init
  opl->init();				// Reset OPL chip
  opl->write(1, 0x20);
  opl->write(8, 0);
  opl->write(0xbd, regbd);

  for(i = 0; i < 9; i++) {
    opl->write(0x20 + op_table[i], 0);
    opl->write(0x23 + op_table[i], 0);
    opl->write(0x40 + op_table[i], 0x3f);
    opl->write(0x43 + op_table[i], 0x3f);
    opl->write(0x60 + op_table[i], 0xff);
    opl->write(0x63 + op_table[i], 0xff);
    opl->write(0x80 + op_table[i], 0xff);
    opl->write(0x83 + op_table[i], 0xff);
    opl->write(0xe0 + op_table[i], 0);
    opl->write(0xe3 + op_table[i], 0);
    opl->write(0xa0 + i, 0);
    opl->write(0xb0 + i, 0);
    opl->write(0xc0 + i, 0);
  }
}

/*** private methods *************************************/

void CldsPlayer::playsound(int inst_number, int channel_number, int tunehigh)
{
  Channel		*c = &channel[channel_number];		// current channel
  SoundBank		*i = &soundbank[inst_number];		// current instrument
  unsigned int		regnum = op_table[channel_number];	// channel's OPL2 register
  unsigned char		volcalc, octave;
  unsigned short	freq;

  // set fine tune
  tunehigh += ((i->finetune + c->finetune + 0x80) & 0xff) - 0x80;

  // arpeggio handling
  if(!i->arpeggio) {
    unsigned short	arpcalc = i->arp_tab[0] << 4;

    if(arpcalc > 0x800)
      tunehigh = tunehigh - (arpcalc ^ 0xff0) - 16;
    else
      tunehigh += arpcalc;
  }

  // glide handling
  if(c->glideto != 0) {
    c->gototune = tunehigh;
    c->portspeed = c->glideto;
    c->glideto = c->finetune = 0;
    return;
  }

  // set modulator registers
  setregs(0x20 + regnum, i->mod_misc);
  volcalc = i->mod_vol;
  if(!c->nextvol || !(i->feedback & 1))
    c->volmod = volcalc;
  else
    c->volmod = (volcalc & 0xc0) | ((((volcalc & 0x3f) * c->nextvol) >> 6));

  if((i->feedback & 1) == 1 && allvolume != 0)
    setregs(0x40 + regnum, ((c->volmod & 0xc0) | (((c->volmod & 0x3f) * allvolume) >> 8)) ^ 0x3f);
  else
    setregs(0x40 + regnum, c->volmod ^ 0x3f);
  setregs(0x60 + regnum, i->mod_ad);
  setregs(0x80 + regnum, i->mod_sr);
  setregs(0xe0 + regnum, i->mod_wave);

  // Set carrier registers
  setregs(0x23 + regnum, i->car_misc);
  volcalc = i->car_vol;
  if(!c->nextvol)
    c->volcar = volcalc;
  else
    c->volcar = (volcalc & 0xc0) | ((((volcalc & 0x3f) * c->nextvol) >> 6));

  if(allvolume)
    setregs(0x43 + regnum, ((c->volcar & 0xc0) | (((c->volcar & 0x3f) * allvolume) >> 8)) ^ 0x3f);
  else
    setregs(0x43 + regnum, c->volcar ^ 0x3f);
  setregs(0x63 + regnum, i->car_ad);
  setregs(0x83 + regnum, i->car_sr);
  setregs(0xe3 + regnum, i->car_wave);
  setregs(0xc0 + channel_number, i->feedback);
  setregs_adv(0xb0 + channel_number, 0xdf, 0);		// key off

  freq = frequency[tunehigh % (12 * 16)];
  octave = tunehigh / (12 * 16) - 1;
  if(!i->glide) {
    if(!i->portamento || !c->lasttune) {
      setregs(0xa0 + channel_number, freq & 0xff);
      setregs(0xb0 + channel_number, (octave << 2) + 0x20 + (freq >> 8));
      c->lasttune = c->gototune = tunehigh;
    } else {
      c->gototune = tunehigh;
      c->portspeed = i->portamento;
      setregs_adv(0xb0 + channel_number, 0xdf, 0x20);	// key on
    }
  } else {
    setregs(0xa0 + channel_number, freq & 0xff);
    setregs(0xb0 + channel_number, (octave << 2) + 0x20 + (freq >> 8));
    c->lasttune = tunehigh;
    c->gototune = tunehigh + ((i->glide + 0x80) & 0xff) - 0x80;	// set destination
    c->portspeed = i->portamento;
  }

  if(!i->vibrato)
    c->vibwait = c->vibspeed = c->vibrate = 0;
  else {
    c->vibwait = i->vibdelay;
    // PASCAL:    c->vibspeed = ((i->vibrato >> 4) & 15) + 1;
    c->vibspeed = (i->vibrato >> 4) + 2;
    c->vibrate = (i->vibrato & 15) + 1;
  }

  if(!(c->trmstay & 0xf0)) {
    c->trmwait = (i->tremwait & 0xf0) >> 3;
    // PASCAL:    c->trmspeed = (i->mod_trem >> 4) & 15;
    c->trmspeed = i->mod_trem >> 4;
    c->trmrate = i->mod_trem & 15;
    c->trmcount = 0;
  }

  if(!(c->trmstay & 0x0f)) {
    c->trcwait = (i->tremwait & 15) << 1;
    // PASCAL:    c->trcspeed = (i->car_trem >> 4) & 15;
    c->trcspeed = i->car_trem >> 4;
    c->trcrate = i->car_trem & 15;
    c->trccount = 0;
  }

  c->arp_size = i->arpeggio & 15;
  c->arp_speed = i->arpeggio >> 4;
  memcpy(c->arp_tab, i->arp_tab, 12);
  c->keycount = i->keyoff;
  c->nextvol = c->glideto = c->finetune = c->vibcount = c->arp_pos = c->arp_count = 0;
}

inline void CldsPlayer::setregs(unsigned char reg, unsigned char val)
{
  if(fmchip[reg] == val) return;

  fmchip[reg] = val;
  opl->write(reg, val);
}

inline void CldsPlayer::setregs_adv(unsigned char reg, unsigned char mask,
				    unsigned char val)
{
  setregs(reg, (fmchip[reg] & mask) | val);
}
