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

#include <cubemaps.hh>
#include <hyperspace.hh>
#include <nebula.hh>
#include <pngimage.hh>

namespace Nebula {
	GLuint _texture;
};

void Nebula::init() {
	PNG png("nebula.png");

#if USE_GL_EXTENSIONS
	if (Hack::shaders) {
		_texture = Common::resources->genCubeMapTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
			png
		);
		WavyNormalCubeMaps::init();
	} else
#endif // USE_GL_EXTENSIONS
	{
		unsigned int h = png.height();
		unsigned int w = png.width();
		float halfH = float(h) / 2.0f;
		float halfW = float(w) / 2.0f;

		stdx::dim3<float, 3> _nebulaMap(w, h);
		for (unsigned int i = 0; i < h; ++i) {
			float y = (float(i) - halfH) / halfH;
			for (unsigned int j = 0; j < w; ++j) {
				float x = (float(j) - halfW) / halfW;
				float temp = Common::clamp(
					(x * x) + (y * y),
					0.0f, 1.0f
				);
				RGBColor color(png(i, j));
				_nebulaMap(i, j, 0) = color.r() * temp;
				_nebulaMap(i, j, 1) = color.g() * temp;
				_nebulaMap(i, j, 2) = color.b() * temp;
			}
		}
		_texture = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			3, w, h, GL_RGB, GL_FLOAT, &_nebulaMap.front()
		);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	}
}
