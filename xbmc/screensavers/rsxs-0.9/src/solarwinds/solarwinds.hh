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
#ifndef _SOLARWINDS_HH
#define _SOLARWINDS_HH

#include <common.hh>

namespace Hack {
	enum GeometryType {
		LIGHTS_GEOMETRY,
		POINTS_GEOMETRY,
		LINES_GEOMETRY
	};

	extern unsigned int numWinds;
	extern unsigned int numEmitters;
	extern unsigned int numParticles;
	extern GeometryType geometry;
	extern float size;
	extern float windSpeed;
	extern float emitterSpeed;
	extern float particleSpeed;
	extern float blur;
};

#endif // _SOLARWINDS_HH
