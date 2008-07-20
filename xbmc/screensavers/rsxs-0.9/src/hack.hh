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
#ifndef _HACK_HH
#define _HACK_HH

#include <common.hh>

#include <argp.h>

namespace Hack {
	typedef Common::Exception Exception;

	std::string getShortName();
	std::string getName();

	const struct argp* getParser();
	void start();
	void reshape();
	void tick();
	void stop();

	void keyPress(char, const KeySym&);
	void keyRelease(char, const KeySym&);
	void buttonPress(unsigned int);
	void buttonRelease(unsigned int);
	void pointerMotion(int, int);
	void pointerEnter();
	void pointerLeave();
};

#endif // _HACK_HH
