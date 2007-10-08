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

#include <climits>
#include <solarwinds.hh>
#include <resource.hh>
#include <wind.hh>

#define LIGHTSIZE 64

GLuint Wind::_list;
GLuint Wind::_texture;

void Wind::init() {
	switch (Hack::geometry) {
	case Hack::LIGHTS_GEOMETRY:
		{
			GLubyte light[LIGHTSIZE][LIGHTSIZE];
			for (int i = 0; i < LIGHTSIZE; ++i) {
				for(int j = 0; j < LIGHTSIZE; ++j) {
					float x = float(i - LIGHTSIZE / 2) / float(LIGHTSIZE / 2);
					float y = float(j - LIGHTSIZE / 2) / float(LIGHTSIZE / 2);
					float temp = Common::clamp(
						1.0f - float(std::sqrt((x * x) + (y * y))),
						0.0f, 1.0f
					);
					light[i][j] = GLubyte(255.0f * temp);
				}
			}
			_texture = Common::resources->genTexture(
				GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
				1, LIGHTSIZE, LIGHTSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, light,
				false
			);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			float temp = 0.02f * Hack::size;
			_list = Common::resources->genLists(1);
			glNewList(_list, GL_COMPILE);
				glBindTexture(GL_TEXTURE_2D, _texture);
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(-temp, -temp, 0.0f);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f(temp, -temp, 0.0f);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(-temp, temp, 0.0f);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(temp, temp, 0.0f);
				glEnd();
			glEndList();
		}
		break;
	case Hack::POINTS_GEOMETRY:
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		break;
	case Hack::LINES_GEOMETRY:
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		break;
	}
}

Wind::Wind() {
	for (unsigned int i = 0; i < Hack::numEmitters; ++i)
		_emitters.push_back(Vector(
			Common::randomFloat(60.0f) - 30.0f,
			Common::randomFloat(60.0f) - 30.0f,
			Common::randomFloat(30.0f) - 15.0f
		));

	_particlesXYZ.resize(Hack::numParticles, Vector(0.0f, 0.0f, 100.0f));
	_particlesRGB.resize(Hack::numParticles, RGBColor());

	_whichParticle = 0;

	if (Hack::geometry == Hack::LINES_GEOMETRY) {
		_lineList.resize(Hack::numParticles, std::make_pair(UINT_MAX, UINT_MAX));
		for (unsigned int i = 0; i < Hack::numEmitters; ++i)
			_lastParticle.push_back(i);
	}

	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_ct[i] = Common::randomFloat(M_PI * 2.0f);
		_cv[i] = Common::randomFloat(
			0.00005f * float(Hack::windSpeed) * float(Hack::windSpeed)
		) + 0.00001f * float(Hack::windSpeed) * float(Hack::windSpeed);
	}
}

void Wind::update() {
	// update constants
	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_ct[i] += _cv[i];
		if (_ct[i] > M_PI * 2.0f)
			_ct[i] -= M_PI * 2.0f;
		_c[i] = std::cos(_ct[i]);
	}

	static float eVel = Hack::emitterSpeed * 0.01f;

	// calculate emissions
	for (unsigned int i = 0; i < Hack::numEmitters; ++i) {
		// emitter moves toward viewer
		_emitters[i].z() += eVel;
		if (_emitters[i].z() > 15.0f)	// reset emitter
			_emitters[i].set(
				Common::randomFloat(60.0f) - 30.0f,
				Common::randomFloat(60.0f) - 30.0f,
				-15.0f
			);
		_particlesXYZ[_whichParticle] = _emitters[i];
		if (Hack::geometry == Hack::LINES_GEOMETRY) {
			// link particles to form lines
			if (_lineList[_whichParticle].first != UINT_MAX)
				_lineList[_lineList[_whichParticle].first].second = UINT_MAX;
			_lineList[_whichParticle].first = UINT_MAX;
			if (_emitters[i].z() == -15.0f)
				_lineList[_whichParticle].second = UINT_MAX;
			else
				_lineList[_whichParticle].second = _lastParticle[i];
			_lineList[_lastParticle[i]].first = _whichParticle;
			_lastParticle[i] = _whichParticle;
		}
		++_whichParticle;
		if (_whichParticle >= Hack::numParticles)
			_whichParticle = 0;
	}

	// calculate particle positions and colors
	// first modify constants that affect colors
	_c[6] *= 9.0f / Hack::particleSpeed;
	_c[7] *= 9.0f / Hack::particleSpeed;
	_c[8] *= 9.0f / Hack::particleSpeed;
	// then update each particle
	static float pVel = Hack::particleSpeed * 0.01f;
	for (unsigned int i = 0; i < Hack::numParticles; ++i) {
		// store old positions
		float x = _particlesXYZ[i].x();
		float y = _particlesXYZ[i].y();
		float z = _particlesXYZ[i].z();
		// make new positions
		_particlesXYZ[i].set(
			x + (_c[0] * y + _c[1] * z) * pVel,
			y + (_c[2] * z + _c[3] * x) * pVel,
			z + (_c[4] * x + _c[5] * y) * pVel
		);
		// calculate colors
		_particlesRGB[i].set(
			std::abs((_particlesXYZ[i].x() - x) * _c[6]),
			std::abs((_particlesXYZ[i].y() - y) * _c[7]),
			std::abs((_particlesXYZ[i].z() - z) * _c[8])
		);
		// clamp colors
		_particlesRGB[i].clamp();
	}

	static float pointSize = 0.04f * Hack::size;
	static float lineSize = 0.005f * Hack::size;

	// draw particles
	switch (Hack::geometry) {
	case Hack::LIGHTS_GEOMETRY:
		for (unsigned int i = 0; i < Hack::numParticles; ++i) {
			glColor3fv(_particlesRGB[i].get());
			glPushMatrix();
				glTranslatef(
					_particlesXYZ[i].x(),
					_particlesXYZ[i].y(),
					_particlesXYZ[i].z()
				);
				glCallList(_list);
			glPopMatrix();
		}
		break;
	case Hack::POINTS_GEOMETRY:
		for (unsigned int i = 0; i < Hack::numParticles; ++i) {
			float temp = _particlesXYZ[i].z() + 40.0f;
			if (temp < 0.01f) temp = 0.01f;
			glPointSize(pointSize * temp);
			glBegin(GL_POINTS);
				glColor3fv(_particlesRGB[i].get());
				glVertex3fv(_particlesXYZ[i].get());
			glEnd();
		}
		break;
	case Hack::LINES_GEOMETRY:
		{
			for (unsigned int i = 0; i < Hack::numParticles; ++i) {
				float temp = _particlesXYZ[i].z() + 40.0f;
				if (temp < 0.01f) temp = 0.01f;
				glLineWidth(lineSize * temp);
				glBegin(GL_LINES);
					if (_lineList[i].second != UINT_MAX) {
						glColor3fv(_particlesRGB[i].get());
						if (_lineList[i].first == UINT_MAX)
							glColor3f(0.0f, 0.0f, 0.0f);
						glVertex3fv(_particlesXYZ[i].get());
						glColor3fv(_particlesRGB[_lineList[i].second].get());
						if (_lineList[_lineList[i].second].second == UINT_MAX)
							glColor3f(0.0f, 0.0f, 0.0f);
						glVertex3fv(_particlesXYZ[_lineList[i].second].get());
					}
				glEnd();
			}
		}
		break;
	}
}
