/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * adtrack.cpp - Adlib Tracker 1.0 Loader by Simon Peter <dn.tlp@gmx.net>
 *
 * NOTES:
 * The original Adlib Tracker 1.0 is behaving a little different from the
 * official spec: The 'octave' integer from the instrument file is stored
 * "minus 1" from the actual value, underflowing from 0 to 0xffff.
 *
 * I also noticed that my player is playing everything transposed a few tones
 * higher than the original tracker. As far as i can see, my player perfectly
 * follows the official spec, so it "must" be the tracker that does something
 * wrong here...
 */

#include <stdlib.h>
#include <string.h>

#include "adtrack.h"
#include "debug.h"

/*** Public methods ***/

CPlayer *CadtrackLoader::factory(Copl *newopl)
{
  return new CadtrackLoader(newopl);
}

bool CadtrackLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  binistream *instf;
  char note[2];
  unsigned short rwp;
  unsigned char chp, octave, pnote = 0;
  int i,j;
  AdTrackInst myinst;

  // file validation
  if(!fp.extension(filename, ".sng") || fp.filesize(f) != 36000)
    { fp.close(f); return false; }

  // check for instruments file
  std::string instfilename(filename, 0, filename.find_last_of('.'));
  instfilename += ".ins";
  AdPlug_LogWrite("CadtrackLoader::load(,\"%s\"): Checking for \"%s\"...\n",
		  filename.c_str(), instfilename.c_str());
  instf = fp.open(instfilename);
  if(!instf || fp.filesize(instf) != 468) { fp.close(f); return false; }

  // give CmodPlayer a hint on what we're up to
  realloc_patterns(1,1000,9); realloc_instruments(9); realloc_order(1);
  init_trackord(); flags = NoKeyOn;
  (*order) = 0; length = 1; restartpos = 0; bpm = 120; initspeed = 3;

  // load instruments from instruments file
  for(i=0;i<9;i++) {
    for(j=0;j<2;j++) {
      myinst.op[j].appampmod = instf->readInt(2);
      myinst.op[j].appvib = instf->readInt(2);
      myinst.op[j].maintsuslvl = instf->readInt(2);
      myinst.op[j].keybscale = instf->readInt(2);
      myinst.op[j].octave = instf->readInt(2);
      myinst.op[j].freqrisevollvldn = instf->readInt(2);
      myinst.op[j].softness = instf->readInt(2);
      myinst.op[j].attack = instf->readInt(2);
      myinst.op[j].decay = instf->readInt(2);
      myinst.op[j].release = instf->readInt(2);
      myinst.op[j].sustain = instf->readInt(2);
      myinst.op[j].feedback = instf->readInt(2);
      myinst.op[j].waveform = instf->readInt(2);
    }
    convert_instrument(i, &myinst);
  }
  fp.close(instf);

  // load file
  for(rwp=0;rwp<1000;rwp++)
    for(chp=0;chp<9;chp++) {
      // read next record
      f->readString(note, 2); octave = f->readInt(1); f->ignore();
      switch(*note) {
      case 'C': if(note[1] == '#') pnote = 2; else pnote = 1; break;
      case 'D': if(note[1] == '#') pnote = 4; else pnote = 3; break;
      case 'E': pnote = 5; break;
      case 'F': if(note[1] == '#') pnote = 7; else pnote = 6; break;
      case 'G': if(note[1] == '#') pnote = 9; else pnote = 8; break;
      case 'A': if(note[1] == '#') pnote = 11; else pnote = 10; break;
      case 'B': pnote = 12; break;
      case '\0':
	if(note[1] == '\0')
	  tracks[chp][rwp].note = 127;
	else {
	  fp.close(f);
	  return false;
	}
	break;
      default: fp.close(f); return false;
      }
      if((*note) != '\0') {
	tracks[chp][rwp].note = pnote + (octave * 12);
	tracks[chp][rwp].inst = chp + 1;
      }
    }

  fp.close(f);
  rewind(0);
  return true;
}

float CadtrackLoader::getrefresh()
{
  return 18.2f;
}

/*** Private methods ***/

void CadtrackLoader::convert_instrument(unsigned int n, AdTrackInst *i)
{
  // Carrier "Amp Mod / Vib / Env Type / KSR / Multiple" register
  inst[n].data[2] = i->op[Carrier].appampmod ? 1 << 7 : 0;
  inst[n].data[2] += i->op[Carrier].appvib ? 1 << 6 : 0;
  inst[n].data[2] += i->op[Carrier].maintsuslvl ? 1 << 5 : 0;
  inst[n].data[2] += i->op[Carrier].keybscale ? 1 << 4 : 0;
  inst[n].data[2] += (i->op[Carrier].octave + 1) & 0xffff; // Bug in original tracker
  // Modulator...
  inst[n].data[1] = i->op[Modulator].appampmod ? 1 << 7 : 0;
  inst[n].data[1] += i->op[Modulator].appvib ? 1 << 6 : 0;
  inst[n].data[1] += i->op[Modulator].maintsuslvl ? 1 << 5 : 0;
  inst[n].data[1] += i->op[Modulator].keybscale ? 1 << 4 : 0;
  inst[n].data[1] += (i->op[Modulator].octave + 1) & 0xffff; // Bug in original tracker

  // Carrier "Key Scaling / Level" register
  inst[n].data[10] = (i->op[Carrier].freqrisevollvldn & 3) << 6;
  inst[n].data[10] += i->op[Carrier].softness & 63;
  // Modulator...
  inst[n].data[9] = (i->op[Modulator].freqrisevollvldn & 3) << 6;
  inst[n].data[9] += i->op[Modulator].softness & 63;

  // Carrier "Attack / Decay" register
  inst[n].data[4] = (i->op[Carrier].attack & 0x0f) << 4;
  inst[n].data[4] += i->op[Carrier].decay & 0x0f;
  // Modulator...
  inst[n].data[3] = (i->op[Modulator].attack & 0x0f) << 4;
  inst[n].data[3] += i->op[Modulator].decay & 0x0f;

  // Carrier "Release / Sustain" register
  inst[n].data[6] = (i->op[Carrier].release & 0x0f) << 4;
  inst[n].data[6] += i->op[Carrier].sustain & 0x0f;
  // Modulator...
  inst[n].data[5] = (i->op[Modulator].release & 0x0f) << 4;
  inst[n].data[5] += i->op[Modulator].sustain & 0x0f;

  // Channel "Feedback / Connection" register
  inst[n].data[0] = (i->op[Carrier].feedback & 7) << 1;

  // Carrier/Modulator "Wave Select" registers
  inst[n].data[8] = i->op[Carrier].waveform & 3;
  inst[n].data[7] = i->op[Modulator].waveform & 3;
}
