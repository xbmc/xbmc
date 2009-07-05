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
 * adtrack.h - Adlib Tracker 1.0 Loader by Simon Peter <dn.tlp@gmx.net>
 */

#include "protrack.h"

class CadtrackLoader: public CmodPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CadtrackLoader(Copl *newopl)
		: CmodPlayer(newopl)
	{ };

	bool load(const std::string &filename, const CFileProvider &fp);
	float getrefresh();

	std::string gettype()
	{ return std::string("Adlib Tracker 1.0"); };
	unsigned int getinstruments()
	{ return 9; };

private:
        enum Operators {Carrier = 1, Modulator = 0};

	typedef struct {
	  struct {
	    unsigned short appampmod, appvib, maintsuslvl, keybscale, octave,
	      freqrisevollvldn, softness, attack, decay, release, sustain,
	      feedback, waveform;
	  } op[2];
	} AdTrackInst;

	void convert_instrument(unsigned int n, AdTrackInst *i);
};
