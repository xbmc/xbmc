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

#include <color.hh>
#include <flux.hh>
#include <resource.hh>
#include <trail.hh>

#define LIGHTSIZE 64

GLuint Trail::_list;
GLuint Trail::_lightTexture;

void Trail::init() {
	switch (Hack::geometry) {
	case Hack::SPHERES_GEOMETRY:
		{
			_list = Common::resources->genLists(1);
			glNewList(_list, GL_COMPILE);
				GLUquadricObj* qObj = gluNewQuadric();
				gluSphere(qObj, 0.005f * Hack::size, Hack::complexity + 2,
					Hack::complexity + 1);
				gluDeleteQuadric(qObj);
			glEndList();

			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			float ambient[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			float diffuse[4] = {1.0f, 1.0f, 1.0f, 0.0f};
			float specular[4] = {1.0f, 1.0f, 1.0f, 0.0f};
			float position[4] = {500.0f, 500.0f, 500.0f, 0.0f};
			glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
			glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
			glLightfv(GL_LIGHT0, GL_POSITION, position);
			glEnable(GL_COLOR_MATERIAL);
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		}
		break;
	case Hack::LIGHTS_GEOMETRY:
		{
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
			_lightTexture = Common::resources->genTexture(
				GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
				1, LIGHTSIZE, LIGHTSIZE,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, light, false
			);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnable(GL_TEXTURE_2D);

			float temp = Hack::size * 0.005f;
			_list = Common::resources->genLists(1);
			glNewList(_list, GL_COMPILE);
				glBindTexture(GL_TEXTURE_2D, _lightTexture);
				glBegin(GL_TRIANGLES);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(-temp, -temp, 0.0f);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f(temp, -temp, 0.0f);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(temp, temp, 0.0f);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(-temp, -temp, 0.0f);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(temp, temp, 0.0f);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(-temp, temp, 0.0f);
				glEnd();
			glEndList();
		}
		break;
	default:
		break;
	}
}

void Trail::update(const float* c, float cosCameraAngle, float sinCameraAngle) {
	unsigned int oldCounter = _counter;

	++_counter;
	if (_counter >= Hack::trailLength)
		_counter = 0;

	// Here's the iterative math for calculating new vertex positions
	// first calculate limiting terms which keep vertices from constantly
	// flying off to infinity
	float cx = _vertices[oldCounter].x() *
		(1.0f - 1.0f / (_vertices[oldCounter].x() *
		_vertices[oldCounter].x() + 1.0f));
	float cy = _vertices[oldCounter].y() *
		(1.0f - 1.0f / (_vertices[oldCounter].y() *
		_vertices[oldCounter].y() + 1.0f));
	float cz = _vertices[oldCounter].z() *
		(1.0f - 1.0f / (_vertices[oldCounter].z() *
		_vertices[oldCounter].z() + 1.0f));
	// then calculate new positions
	_vertices[_counter].set(
		_vertices[oldCounter].x() + c[6] * _offset.x() - cx
			+ c[2] * _vertices[oldCounter].y()
			+ c[5] * _vertices[oldCounter].z(),
		_vertices[oldCounter].y() + c[6] * _offset.y() - cy
			+ c[1] * _vertices[oldCounter].z()
			+ c[4] * _vertices[oldCounter].x(),
		_vertices[oldCounter].z() + c[6] * _offset.z() - cz
			+ c[0] * _vertices[oldCounter].x()
			+ c[3] * _vertices[oldCounter].y()
	);

	// Pick a hue
	_hues[_counter] = cx * cx + cy * cy + cz * cz;
	if (_hues[_counter] > 1.0f) _hues[_counter] = 1.0f;
	_hues[_counter] += c[7];
	// Limit the hue (0 - 1)
	if (_hues[_counter] > 1.0f) _hues[_counter] -= 1.0f;
	if (_hues[_counter] < 0.0f) _hues[_counter] += 1.0f;
	// Pick a saturation
	_sats[_counter] = c[0] + _hues[_counter];
	// Limit the saturation (0 - 1)
	if (_sats[_counter] < 0.0f) _sats[_counter] = -_sats[_counter];
	_sats[_counter] -= float(int(_sats[_counter]));
	_sats[_counter] = 1.0f - (_sats[_counter] * _sats[_counter]);

	// Bring particles back if they escape
	if (!_counter) {
		if (
			(_vertices[0].x() > 10000.0f) ||
			(_vertices[0].x() < -10000.0f) ||
			(_vertices[0].y() > 10000.0f) ||
			(_vertices[0].y() < -10000.0f) ||
			(_vertices[2].z() > 10000.0f) ||
			(_vertices[0].z() < -10000.0f)
		) {
			_vertices[0].set(
				Common::randomFloat(2.0f) - 1.0f,
				Common::randomFloat(2.0f) - 1.0f,
				Common::randomFloat(2.0f) - 1.0f
			);
		}
	}

	// Draw every vertex in particle trail
	unsigned int p = _counter;
	unsigned int growth = 0;
	static float lumDiff = 1.0f / float(Hack::trailLength);
	float luminosity = lumDiff;
	for (unsigned int i = 0; i < Hack::trailLength; ++i) {
		++p;
		if (p >= Hack::trailLength) p = 0;
		++growth;

		// assign color to particle
		glColor3fv(RGBColor(HSLColor(_hues[p], _sats[p], luminosity)).get());

		float depth = 0.0f;
		glPushMatrix();
		if (Hack::geometry == Hack::SPHERES_GEOMETRY)
			glTranslatef(
				_vertices[p].x(),
				_vertices[p].y(),
				_vertices[p].z()
			);
		else {	// Points or lights
			depth =
				cosCameraAngle * _vertices[p].z() -
				sinCameraAngle * _vertices[p].x();
			glTranslatef(
				cosCameraAngle * _vertices[p].x() +
					sinCameraAngle * _vertices[p].z(),
				_vertices[p].y(),
				depth
			);
		}
		if (Hack::geometry != Hack::POINTS_GEOMETRY) {
			switch (Hack::trailLength - growth) {
			case 0:
				glScalef(0.259f, 0.259f, 0.259f);
				break;
			case 1:
				glScalef(0.5f, 0.5f, 0.5f);
				break;
			case 2:
				glScalef(0.707f, 0.707f, 0.707f);
				break;
			case 3:
				glScalef(0.866f, 0.866f, 0.866f);
				break;
			case 4:
				glScalef(0.966f, 0.966f, 0.966f);
				break;
			}
		}
		switch (Hack::geometry) {
		case Hack::POINTS_GEOMETRY:
			switch (Hack::trailLength - growth) {
			case 0:
				glPointSize(float(Hack::size * (depth + 200.0f) * 0.001036f));
				break;
			case 1:
				glPointSize(float(Hack::size * (depth + 200.0f) * 0.002f));
				break;
			case 2:
				glPointSize(float(Hack::size * (depth + 200.0f) * 0.002828f));
				break;
			case 3:
				glPointSize(float(Hack::size * (depth + 200.0f) * 0.003464f));
				break;
			case 4:
				glPointSize(float(Hack::size * (depth + 200.0f) * 0.003864f));
				break;
			default:
				glPointSize(float(Hack::size * (depth + 200.0f) * 0.004f));
				break;
			}
			glBegin(GL_POINTS);
				glVertex3f(0.0f, 0.0f, 0.0f);
			glEnd();
			break;
		case Hack::SPHERES_GEOMETRY:
		case Hack::LIGHTS_GEOMETRY:
			glCallList(_list);
		}
		glPopMatrix();

		static float expander = 1.0f + 0.0005f * Hack::expansion;
		static float blower = 0.001f * Hack::wind;

		_vertices[p] *= expander;
		_vertices[p].z() += blower;
		luminosity += lumDiff;
	}
}
