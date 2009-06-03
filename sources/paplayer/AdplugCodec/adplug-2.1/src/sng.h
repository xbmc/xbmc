/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * sng.h - SNG Player by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_SNGPLAYER
#define H_ADPLUG_SNGPLAYER

#include "player.h"

class CsngPlayer: public CPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

	CsngPlayer(Copl *newopl)
		: CPlayer(newopl), data(0)
	{ };
	~CsngPlayer()
	{ if(data) delete [] data; };

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);
	float getrefresh()
	{ return 70.0f; };

	std::string gettype()
	{ return std::string("SNG File Format"); };

protected:
	struct {
		char id[4];
		unsigned short length,start,loop;
		unsigned char delay;
		bool compressed;
	} header;

	struct Sdata {
		unsigned char val,reg;
	} *data;

	unsigned char del;
	unsigned short pos;
	bool songend;
};

#endif
