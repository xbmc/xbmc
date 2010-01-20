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
#ifndef _FLOCKS_HH
#define _FLOCKS_HH

#include <common.hh>

namespace Hack {
	extern unsigned int numLeaders;
	extern unsigned int numFollowers;
	extern bool blobs;
	extern float size;
	extern unsigned int complexity;
	extern float speed;
	extern float stretch;
	extern float colorFadeSpeed;
	extern bool chromatek;
	extern bool connections;
};

#endif // _FLOCKS_HH
