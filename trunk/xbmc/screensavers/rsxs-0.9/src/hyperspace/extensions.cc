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
#include <common.hh>

#include <extensions.hh>

#if USE_GL_EXTENSIONS

namespace Extensions {
	PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
	PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
	PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
	PFNGLBINDPROGRAMARBPROC glBindProgramARB;
	PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
};

#if GLX_ARB_get_proc_address

void Extensions::init() {
	std::string extensions((const char*)glGetString(GL_EXTENSIONS));
	TRACE("Extensions: " << extensions);

	std::string::size_type a = 0;
	unsigned int count = 0;
	while (a < extensions.size()) {
		std::string::size_type b = extensions.find(' ', a);
		if (b == std::string::npos) b = extensions.size();
		std::string s(extensions.substr(a, b - a));
		if (
			s == "GL_ARB_multitexture" ||
			s == "GL_ARB_texture_cube_map" ||
			s == "GL_ARB_vertex_program" ||
			s == "GL_ARB_fragment_program"
		)
			count++;
		a = b + 1;
	}

	if (count != 4)
		throw Exception("Not all required GL extensions available");

	glActiveTextureARB = PFNGLACTIVETEXTUREARBPROC(
		glXGetProcAddressARB((const GLubyte*)"glActiveTextureARB"));
	glDeleteProgramsARB = PFNGLDELETEPROGRAMSARBPROC(
		glXGetProcAddressARB((const GLubyte*)"glDeleteProgramsARB"));
	glGenProgramsARB = PFNGLGENPROGRAMSARBPROC(
		glXGetProcAddressARB((const GLubyte*)"glGenProgramsARB"));
	glBindProgramARB = PFNGLBINDPROGRAMARBPROC(
		glXGetProcAddressARB((const GLubyte*)"glBindProgramARB"));
	glProgramStringARB = PFNGLPROGRAMSTRINGARBPROC(
		glXGetProcAddressARB((const GLubyte*)"glProgramStringARB"));
}

#else // !GLX_ARB_get_proc_address

void Extensions::init() {
	// This one's easy!
	throw Exception("Can not dynamically get GL procedure addresses");
}

#endif // !GLX_ARB_get_proc_address

#endif // USE_GL_EXTENSIONS
