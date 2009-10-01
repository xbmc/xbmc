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
#ifndef _RESOURCES_HH
#define _RESOURCES_HH

#include <common.hh>

#include <sound.hh>
#include <vector.hh>

#define CLOUDMESH 48

namespace Resources {
	namespace Sounds {
		extern Sound* boom1;
		extern Sound* boom2;
		extern Sound* boom3;
		extern Sound* boom4;
		extern Sound* launch1;
		extern Sound* launch2;
		extern Sound* nuke;
		extern Sound* popper;
		extern Sound* suck;
		extern Sound* whistle;
	};

	namespace Textures {
		extern GLuint cloud;
		extern GLuint stars;
		extern GLuint moon;
		extern GLuint moonGlow;
		extern GLuint sunset;
		extern GLuint earthNear;
		extern GLuint earthFar;
		extern GLuint earthLight;
		extern GLuint smoke[5];
		extern GLuint flare[4];
	};

	namespace DisplayLists {
		extern GLuint flares;
		extern GLuint rocket;
		extern GLuint smokes;
		extern GLuint stars;
		extern GLuint moon;
		extern GLuint moonGlow;
		extern GLuint sunset;
		extern GLuint earthNear;
		extern GLuint earthFar;
		extern GLuint earthLight;
	};

	void init();
};

#endif // _RESOURCES_HH
