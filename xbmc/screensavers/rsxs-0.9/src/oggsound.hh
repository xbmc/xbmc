/*
 * Really Slick XScreenSavers
 * Copyright (C) 2002-2006  Michael Chapman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 * This is a Linux port of the Really Slick Screensavers,
 * Copyright (C) 2002 Terence M. Welsh, available from www.reallyslick.com
 */
#ifndef _OGGSOUND_HH
#define _OGGSOUND_HH

#include <common.hh>

#include <sound.hh>

#if HAVE_SOUND

class OGG : public Sound {
public:
	typedef Common::Exception Exception;
private:
	void load(FILE*);
public:
	OGG(const std::string&, float, float = 0.0f);
};

#else // !HAVE_SOUND

class OGG : public Sound {
public:
	OGG(const std::string&, float, float = 0.0f) {}
};

#endif // !HAVE_SOUND

#endif // _OGGSOUND_HH
