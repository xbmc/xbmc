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
#include <common.hh>

#if HAVE_PNG_H
	#include <png.h>
#endif

#if HAVE_SETJMP_H
	#include <setjmp.h>
#endif

#include <cerrno>
#include <color.hh>
#include <pngimage.hh>

void PNG::load(FILE* in, bool fullColor) {
	png_byte sig[8];
	int sigBytes = fread(sig, 1, 8, in);

	if (png_sig_cmp(sig, 0, sigBytes))
		throw Exception("Not a PNG file");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (!png)
		throw Exception("Could not create PNG read structure");
	png_infop pngInfo = png_create_info_struct(png);
	if (!pngInfo)
		throw Exception("Could not create PNG info structure");

	try {
		if (setjmp(png_jmpbuf(png)))
			throw Exception("PNG could not be decoded");

		png_init_io(png, in);
		png_set_sig_bytes(png, sigBytes);

		png_read_info(png, pngInfo);

		if (png_get_color_type(png, pngInfo) == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);
		if (
			(png_get_color_type(png, pngInfo) == PNG_COLOR_TYPE_GRAY) &&
			png_get_bit_depth(png, pngInfo) < 8
		)
			png_set_gray_1_2_4_to_8(png);
		if (png_get_valid(png, pngInfo, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);
		if (fullColor)
			png_set_gray_to_rgb(png);
		if (png_get_bit_depth(png, pngInfo) < 8)
			png_set_packing(png);
		png_read_update_info(png, pngInfo);

		_width = png_get_image_width(png, pngInfo);
		_height = png_get_image_height(png, pngInfo);

		switch (png_get_color_type(png, pngInfo)) {
		case PNG_COLOR_TYPE_GRAY:
			_format = GL_LUMINANCE;
			_bytesPerPixel = 1;
			_hasAlphaChannel = false;
			break;
		case PNG_COLOR_TYPE_RGB:
			_format = GL_RGB;
			_bytesPerPixel = 3;
			_hasAlphaChannel = false;
			break;
		case PNG_COLOR_TYPE_RGBA:
			_format = GL_RGBA;
			_bytesPerPixel = 4;
			_hasAlphaChannel = true;
			break;
		case PNG_COLOR_TYPE_GA:
			_format = GL_LUMINANCE_ALPHA;
			_bytesPerPixel = 2;
			_hasAlphaChannel = true;
			break;
		default:
			throw Exception("Unhandled image type");
		}
		switch (png_get_bit_depth(png, pngInfo)) {
		case 8:
			_type = GL_UNSIGNED_BYTE;
			break;
		case 16:
			_type = GL_UNSIGNED_SHORT;
			_bytesPerPixel *= 2;
			break;
		default:
			throw Exception("Unhandled image depth");
		}
		_numComponents = png_get_channels(png, pngInfo);

		GLint alignment;
 		glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
		_rowLength =
			((_width * _bytesPerPixel + alignment - 1) / alignment) * alignment;
		_data = new uint8_t[_height * _rowLength];
		png_bytep* rows = new png_bytep[_height];
		for (GLsizei i = 0; i < _height; ++i)
			rows[i] = _data + _rowLength * i;
		png_read_image(png, rows);
		delete[] rows;

		png_read_end(png, NULL);
		png_destroy_read_struct(&png, &pngInfo, NULL);
	} catch (...) {
		png_destroy_read_struct(&png, &pngInfo, NULL);
		throw;
	}
}

PNG::PNG(const std::string& filename, bool fullColor) {
	if (filename.empty())
		throw Exception("Empty filename");

	FILE* in = NULL;
	if (filename[0] != '/')
		in = std::fopen((Common::resourceDir + '/' + filename).c_str(), "rb");

	if (!in)
		in = std::fopen(filename.c_str(), "rb");

	if (!in)
		throw Exception(stdx::oss() << filename << ": " << std::strerror(errno));

	try {
		load(in, fullColor);
	} catch (Exception e) {
		throw Exception(stdx::oss() << filename << ": " << e);
	}
	std::fclose(in);
}

PNG::~PNG() {
	if (_data) delete[] _data;
}

const RGBColor PNG::operator()(GLsizei x, GLsizei y) const {
	if (x >= _width || y >= _height)
		return RGBColor();

	uint8_t* pos = _data + _rowLength * y + _bytesPerPixel * x;
	if (_type == GL_UNSIGNED_BYTE) {
		switch (_format) {
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			{
				float l = float(*pos) / 255.0f;
				return RGBColor(l, l, l);
			}
		case GL_RGB:
		case GL_RGBA:
			{
				float r = float(pos[0]) / 255.0f;
				float g = float(pos[1]) / 255.0f;
				float b = float(pos[2]) / 255.0f;
				return RGBColor(r, g, b);
			}
		default:
			return RGBColor();
		}
	} else {
		switch (_format) {
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			{
				float l = float((unsigned int)(pos[0]) * 255 + pos[1]) / 65535.0f;
				return RGBColor(l, l, l);
			}
		case GL_RGB:
		case GL_RGBA:
			{
				float r = float((unsigned int)(pos[0]) * 255 + pos[1]) / 65535.0f;
				float g = float((unsigned int)(pos[2]) * 255 + pos[3]) / 65535.0f;
				float b = float((unsigned int)(pos[4]) * 255 + pos[5]) / 65535.0f;
				return RGBColor(r, g, b);
			}
		default:
			return RGBColor();
		}
	}
}
