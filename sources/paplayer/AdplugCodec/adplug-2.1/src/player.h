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
 * player.h - Replayer base class, by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_PLAYER
#define H_ADPLUG_PLAYER

#include <string>

#include "fprovide.h"
#include "opl.h"
#include "database.h"

class CPlayer
{
public:
        CPlayer(Copl *newopl);
	virtual ~CPlayer();

/***** Operational methods *****/
	void seek(unsigned long ms);

	virtual bool load(const std::string &filename,	// loads file
			  const CFileProvider &fp = CProvider_Filesystem()) = 0;
	virtual bool update() = 0;			// executes replay code for 1 tick
	virtual void rewind(int subsong = -1) = 0;	// rewinds to specified subsong
	virtual float getrefresh() = 0;			// returns needed timer refresh rate

/***** Informational methods *****/
	unsigned long songlength(int subsong = -1);

	virtual std::string gettype() = 0;	// returns file type
	virtual std::string gettitle()		// returns song title
	{ return std::string(); }
	virtual std::string getauthor()		// returns song author name
	{ return std::string(); }
	virtual std::string getdesc()		// returns song description
	{ return std::string(); }
	virtual unsigned int getpatterns()	// returns number of patterns
	{ return 0; }
	virtual unsigned int getpattern()	// returns currently playing pattern
	{ return 0; }
	virtual unsigned int getorders()	// returns size of orderlist
	{ return 0; }
	virtual unsigned int getorder()		// returns currently playing song position
	{ return 0; }
	virtual unsigned int getrow()		// returns currently playing row
	{ return 0; }
	virtual unsigned int getspeed()		// returns current song speed
	{ return 0; }
	virtual unsigned int getsubsongs()	// returns number of subsongs
	{ return 1; }
	virtual unsigned int getinstruments()	// returns number of instruments
	{ return 0; }
	virtual std::string getinstrument(unsigned int n)	// returns n-th instrument name
	{ return std::string(); }

protected:
	Copl		*opl;	// our OPL chip
	CAdPlugDatabase	*db;	// AdPlug Database

	static const unsigned short	note_table[12];	// standard adlib note table
	static const unsigned char	op_table[9];	// the 9 operators as expected by the OPL
};

#endif
