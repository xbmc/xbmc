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
#ifndef _EXTENSIONS_HH
#define _EXTENSIONS_HH

#include <common.hh>

#if GL_ARB_multitexture
#if GL_ARB_texture_cube_map
#if GL_ARB_vertex_program
#if GL_ARB_fragment_program
#define USE_GL_EXTENSIONS 1

namespace Extensions {
	typedef Common::Exception Exception;

	extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
	extern PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
	extern PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
	extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
	extern PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;

	void init();
};

#endif // GL_ARB_fragment_program
#endif // GL_ARB_vertex_program
#endif // GL_ARB_texture_cube_map
#endif // GL_ARB_multitexture

#endif // _EXTENSIONS_HH
