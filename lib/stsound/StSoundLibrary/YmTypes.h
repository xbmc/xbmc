/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	Define YM types for multi-platform compilcation.
	Change that file depending of your platform. Please respect the right size
	for each type.

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

	This file is part of ST-Sound

	ST-Sound is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	ST-Sound is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ST-Sound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------------------*/

#ifndef __YMTYPES__
#define __YMTYPES__

#define YM_INTEGER_ONLY

//-----------------------------------------------------------
// Platform specific stuff
//-----------------------------------------------------------

#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)

// These settings are ok for Windows 32bits platform.

#ifdef YM_INTEGER_ONLY
#if defined(__linux__) || defined(__FreeBSD__)
#include <inttypes.h>
typedef 	int64_t				yms64;
#else
typedef		__int64				yms64;
#endif
#else
typedef		float				ymfloat;
#endif

typedef		signed char			yms8;			//  8 bits signed integer
typedef		signed short		yms16;			// 16 bits signed integer
typedef		signed long			yms32;			// 32 bits signed integer

typedef		unsigned char		ymu8;			//  8 bits unsigned integer
typedef		unsigned short		ymu16;			// 16 bits unsigned integer
typedef		unsigned long		ymu32;			// 32 bits unsigned integer

typedef		int					ymint;			// Native "int" for speed purpose. StSound suppose int is signed and at least 32bits. If not, change it to match to yms32

typedef		char				ymchar;			// 8 bits char character (used for null terminated strings)

#else

	fail
		
	Please make the right definition for your platform ( for 8,16,32 signed and unsigned integer)

#endif

#ifndef NULL
#define NULL	(0L)
#endif

//-----------------------------------------------------------
// Multi-platform
//-----------------------------------------------------------
typedef		int					ymbool;			// boolean ( theorically nothing is assumed for its size in StSound,so keep using int)
typedef		yms16				ymsample;		// StSound emulator render mono 16bits signed PCM samples

#define		YMFALSE				(0)
#define		YMTRUE				(!YMFALSE)

#endif

