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
 * Copyright (C) 2005 Terence M. Welsh, available from www.reallyslick.com
 */
#ifndef _HYPERSPACE_HH
#define _HYPERSPACE_HH

#include <common.hh>

#include <vector.hh>

namespace Hack {
	extern unsigned int resolution;
	extern bool shaders;

	extern unsigned int frames;
	extern unsigned int current;

	extern float fogDepth;
	extern Vector camera, dir;
	extern float unroll;
	extern float lerp;

	extern int viewport[4];
	extern double projMat[16];
	extern double modelMat[16];
};

#endif // _HYPERSPACE_HH
