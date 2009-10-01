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

#include <particle.hh>
#include <resource.hh>

GLuint Ion::_texture;
GLuint Ion::_list;

void Ion::init() {
	GLubyte light[LIGHTSIZE][LIGHTSIZE];
	for (int i = 0; i < LIGHTSIZE; ++i) {
		for (int j = 0; j < LIGHTSIZE; ++j) {
			float x = float(i - LIGHTSIZE / 2) / float(LIGHTSIZE / 2);
			float y = float(j - LIGHTSIZE / 2) / float(LIGHTSIZE / 2);
			float temp = Common::clamp(
				1.0f - float(std::sqrt((x * x) + (y * y))),
				0.0f, 1.0f
			);
			light[i][j] = GLubyte(255.0f * temp * temp);
		}
	}
	_texture = Common::resources->genTexture(
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
		1, LIGHTSIZE, LIGHTSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, light
	);

	_list = Common::resources->genLists(1);
	glNewList(_list, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, _texture);
		glBegin(GL_TRIANGLES);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-0.5f, -0.5f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(0.5f, -0.5f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(0.5f, 0.5f, 0.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-0.5f, -0.5f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(0.5f, 0.5f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-0.5f, 0.5f, 0.0f);
		glEnd();
	glEndList();
}
