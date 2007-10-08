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
#ifndef _HELIOS_HH
#define _HELIOS_HH

#include <common.hh>

namespace Hack {
	extern unsigned int numIons;
	extern float size;
	extern unsigned int numEmitters;
	extern unsigned int numAttractors;
	extern float speed;
	extern float cameraSpeed;
	extern bool surface;
	extern bool wireframe;
	extern float blur;
	extern std::string texture;
};

#endif // _HELIOS_HH
