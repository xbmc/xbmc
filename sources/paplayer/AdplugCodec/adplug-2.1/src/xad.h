/*
  AdPlug - Replayer for many OPL2/OPL3 audio file formats.
  Copyright (C) 1999 - 2003 Simon Peter <dn.tlp@gmx.net>, et al.

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

  xad.h - XAD shell player by Riven the Mage <riven@ok.ru>
*/

#ifndef H_ADPLUG_XAD
#define H_ADPLUG_XAD

#include "player.h"

class CxadPlayer: public CPlayer
{
public:
  static CPlayer *factory(Copl *newopl);

        CxadPlayer(Copl * newopl);
        ~CxadPlayer();

        bool	load(const std::string &filename, const CFileProvider &fp);
        bool	update();
        void	rewind(int subsong);
        float	getrefresh();

        std::string     gettype();
        std::string     gettitle();
        std::string     getauthor();
        std::string     getinstrument(unsigned int i);
        unsigned int    getinstruments();

protected:
	virtual void xadplayer_rewind(int subsong) = 0;
	virtual bool xadplayer_load() = 0;
	virtual void xadplayer_update() = 0;
	virtual float xadplayer_getrefresh() = 0;
	virtual std::string xadplayer_gettype() = 0;
	virtual std::string xadplayer_gettitle()
	  {
	    return std::string(xad.title);
	  }
	virtual std::string xadplayer_getauthor()
	  {
	    return std::string(xad.author);
	  }
	virtual std::string xadplayer_getinstrument(unsigned int i)
	  {
	    return std::string("");
	  }
	virtual unsigned int xadplayer_getinstruments()
	  {
	    return 0;
	  }

	enum { HYP=1, PSI, FLASH, BMF, RAT, HYBRID };

        struct xad_header
        {
	    unsigned long   id;
            char            title[36];
            char            author[36];
            unsigned short  fmt;
            unsigned char   speed;
            unsigned char   reserved_a;
        } xad;

        unsigned char * tune;
        unsigned long   tune_size;

        struct
        {
            int             playing;
            int             looping;
            unsigned char   speed;
            unsigned char   speed_counter;
        } plr;

        unsigned char   adlib[256];

        void opl_write(int reg, int val);
};

#endif
