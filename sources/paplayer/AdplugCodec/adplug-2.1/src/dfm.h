/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2003 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * dfm.h - Digital-FM Loader by Simon Peter <dn.tlp@gmx.net>
 */

#include "protrack.h"

class CdfmLoader: public CmodPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CdfmLoader(Copl *newopl)
		: CmodPlayer(newopl)
	{ };

	bool load(const std::string &filename, const CFileProvider &fp);
	float getrefresh();

	std::string gettype();
	unsigned int getinstruments()
	{ return 32; };
	std::string getinstrument(unsigned int n)
	{ if(*instname[n]) return std::string(instname[n],1,*instname[n]); else return std::string(); };
	std::string getdesc()
	{ return std::string(songinfo,1,*songinfo); };

private:
	struct {
		char id[4];
		unsigned char hiver,lover;
	} header;

	char songinfo[33];
	char instname[32][12];
};
