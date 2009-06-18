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
 * sa2.h - SAdT2 Loader by Simon Peter <dn.tlp@gmx.net>
 *         SAdT Loader by Mamiya <mamiya@users.sourceforge.net>
 */

#include "protrack.h"

class Csa2Loader: public CmodPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	Csa2Loader(Copl *newopl)
		: CmodPlayer(newopl)
	{ }

	bool load(const std::string &filename, const CFileProvider &fp);

	std::string gettype();
	std::string gettitle();
	unsigned int getinstruments()
	{ return 31; }
	std::string getinstrument(unsigned int n)
	{
	  if(n < 29)
	    return std::string(instname[n],1,16);
	  else
	    return std::string("-broken-");
	}

private:
	struct sa2header {
		char sadt[4];
		unsigned char version;
	} header;

	char instname[29][17];
};
