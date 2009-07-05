/*
  Adplug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  fmc.h - FMC loader by Riven the Mage <riven@ok.ru>
*/

#include "protrack.h"

class CfmcLoader: public CmodPlayer
{
	public:
		static CPlayer *factory(Copl *newopl);

		CfmcLoader(Copl *newopl) : CmodPlayer(newopl) { };

		bool	load(const std::string &filename, const CFileProvider &fp);
		float	getrefresh();

		std::string	gettype();
		std::string	gettitle();
		std::string	getinstrument(unsigned int n);
		unsigned int	getinstruments();

	private:

		struct fmc_event
		{
			unsigned char   byte0;
			unsigned char   byte1;
			unsigned char   byte2;
		};

		struct fmc_header
		{
			char            id[4];
			char            title[21];
			unsigned char   numchan;
		} header;

		struct fmc_instrument
		{
			unsigned char   synthesis;
			unsigned char   feedback;

			unsigned char   mod_attack;
			unsigned char   mod_decay;
			unsigned char   mod_sustain;
			unsigned char   mod_release;
			unsigned char   mod_volume;
			unsigned char   mod_ksl;
			unsigned char   mod_freq_multi;
			unsigned char   mod_waveform;
			unsigned char   mod_sustain_sound;
			unsigned char   mod_ksr;
			unsigned char   mod_vibrato;
			unsigned char   mod_tremolo;
			unsigned char   car_attack;
			unsigned char   car_decay;
			unsigned char   car_sustain;
			unsigned char   car_release;
			unsigned char   car_volume;
			unsigned char   car_ksl;
			unsigned char   car_freq_multi;
			unsigned char   car_waveform;
			unsigned char   car_sustain_sound;
			unsigned char   car_ksr;
			unsigned char   car_vibrato;
			unsigned char   car_tremolo;

			signed char     pitch_shift;

			char            name[21];
		} instruments[32];

		void            buildinst(unsigned char i);
};
