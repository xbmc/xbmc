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
#ifndef _PNGIMAGE_HH
#define _PNGIMAGE_HH

#include <common.hh>

#if HAVE_STDINT_H
	#include <stdint.h>
#endif

#include <color.hh>
#include <cstdio>

class PNG {
public:
	typedef std::string Exception;
private:
	GLint _numComponents;
	GLsizei _width;
	GLsizei _height;
	GLenum _format;
	GLenum _type;
	uint8_t* _data;

	bool _hasAlphaChannel;

	unsigned int _bytesPerPixel;
	unsigned int _rowLength;

	void load(FILE*, bool);
public:
	PNG(const std::string&, bool = false);
	~PNG();

	GLint numComponents() const  { return _numComponents; }
	unsigned int width() const   { return _width; }
	unsigned int height() const  { return _height; }
	GLenum format() const        { return _format; }
	GLenum type() const          { return _type; }
	const GLvoid* data() const   { return _data; }

	bool hasAlphaChannel() const { return _hasAlphaChannel; }
	const RGBColor operator()(GLsizei, GLsizei) const;
};

#endif // _PNGIMAGE_HH
