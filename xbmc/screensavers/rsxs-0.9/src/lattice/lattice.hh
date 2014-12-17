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
#ifndef _LATTICE_HH
#define _LATTICE_HH

#include <common.hh>

namespace Hack {
	enum LinkType {
		UNKNOWN_LINKS = -1,
		SOLID_LINKS,
		TRANSLUCENT_LINKS,
		HOLLOW_LINKS
	};

	struct Texture {
		std::string filename;
		float shininess;
		bool sphereMap;
		bool colored;
		bool modulate;
	};

	extern unsigned int longitude;
	extern unsigned int latitude;
	extern float thickness;
	extern unsigned int density;
	extern unsigned int depth;
	extern float fov;
	extern unsigned int pathRand;
	extern float speed;
	extern LinkType linkType;
	extern std::vector<Texture> textures;
	extern bool smooth;
	extern bool fog;
	extern bool widescreen;
};

#endif // _LATTICE_HH
