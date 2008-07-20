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
#ifndef _CAUSTIC_HH
#define _CAUSTIC_HH

#include <common.hh>

#include <extensions.hh>
#include <hyperspace.hh>
#include <shaders.hh>
#include <common.hh>

namespace CausticTextures {
	void init();

	extern std::vector<GLuint> _textures;

	inline void use() {
#if USE_GL_EXTENSIONS
		if (Hack::shaders) {
			Extensions::glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(
				GL_TEXTURE_2D, _textures[(Hack::current + 1) % Hack::frames]
			);
			Extensions::glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(
				GL_TEXTURE_2D, _textures[Hack::current]
			);
			Extensions::glBindProgramARB(
				GL_VERTEX_PROGRAM_ARB, Shaders::tunnelVP
			);
			glEnable(GL_VERTEX_PROGRAM_ARB);
			Extensions::glBindProgramARB(
				GL_FRAGMENT_PROGRAM_ARB, Shaders::tunnelFP
			);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
		} else
#endif // USE_GL_EXTENSIONS
			glBindTexture(GL_TEXTURE_2D, _textures[Hack::current]);
	}
};

#endif // _CAUSTIC_HH
