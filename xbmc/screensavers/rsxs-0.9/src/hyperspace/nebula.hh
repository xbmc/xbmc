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
#ifndef _NEBULA_HH
#define _NEBULA_HH

#include <common.hh>

#include <cubemaps.hh>
#include <extensions.hh>
#include <hyperspace.hh>
#include <shaders.hh>

namespace Nebula {
	typedef Common::Exception Exception;

	void init();
	float update();

	extern GLuint _texture;

	inline void use() {
#if USE_GL_EXTENSIONS
		if (Hack::shaders) {
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
			Extensions::glActiveTextureARB(GL_TEXTURE2_ARB);
			glBindTexture(
				GL_TEXTURE_CUBE_MAP_ARB,
				_texture
			);
			Extensions::glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(
				GL_TEXTURE_CUBE_MAP_ARB,
				WavyNormalCubeMaps::textures[(Hack::current + 1) % Hack::frames]
			);
			Extensions::glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(
				GL_TEXTURE_CUBE_MAP_ARB,
				WavyNormalCubeMaps::textures[Hack::current]
			);
			Extensions::glBindProgramARB(
				GL_VERTEX_PROGRAM_ARB,
				Shaders::gooVP
			);
			glEnable(GL_VERTEX_PROGRAM_ARB);
			Extensions::glBindProgramARB(
				GL_FRAGMENT_PROGRAM_ARB,
				Shaders::gooFP
			);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
		} else
#endif // USE_GL_EXTENSIONS
		{
			glBindTexture(GL_TEXTURE_2D, _texture);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
		}
	}
};

#endif // _NEBULA_HH
