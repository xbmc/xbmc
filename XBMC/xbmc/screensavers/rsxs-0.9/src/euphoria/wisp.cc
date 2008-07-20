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

#include <euphoria.hh>
#include <wisp.hh>

Wisp::Wisp() {
	float recHalfDens = 1.0f / (float(Hack::density) * 0.5f);

	_vertex.resize(   Hack::density + 1, Hack::density + 1);
	_intensity.resize(Hack::density + 1, Hack::density + 1);
	_gridPos.resize(  Hack::density + 1, Hack::density + 1);

	for (unsigned int i = 0; i <= Hack::density; ++i)
		for (unsigned int j = 0; j <= Hack::density; ++j) {
			Vector v(
				float(i) * recHalfDens - 1.0f,
				float(j) * recHalfDens - 1.0f,
				0.0f
			);
			v.z() = v.lengthSquared();
			_gridPos(i, j) = v;
		}

	// initialize constants
	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_c[i] = Common::randomFloat(2.0f) - 1.0f;
		_cr[i] = Common::randomFloat(M_PI * 2.0f);
		_cv[i] = Common::randomFloat(Hack::speed * 0.03f) +
			(Hack::speed * 0.001f);
	}

	// pick color
	_HSL.set(
		Common::randomFloat(1.0f),
		0.1f + Common::randomFloat(0.9f),
		1.0f
	);
	_hueSpeed = Common::randomFloat(0.1f) - 0.05f;
	_saturationSpeed = Common::randomFloat(0.04f) + 0.001f;
}

void Wisp::update() {
	// visibility constants
	static float viscon1 = Hack::visibility * 0.01f;
	static float viscon2 = 1.0f / viscon1;

	// update constants
	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_cr[i] += _cv[i] * Common::elapsedSecs;
		if (_cr[i] > M_PI * 2.0f)
			_cr[i] -= M_PI * 2.0f;
		_c[i] = std::cos(_cr[i]);
	}

	// update vertex positions
	for (unsigned int i = 0; i <= Hack::density; ++i)
		for (unsigned int j = 0; j <= Hack::density; ++j)
			_vertex(i, j).set(
				_gridPos(i, j).x() * _gridPos(i, j).x() *
					_gridPos(i, j).y() * _c[0] +
					_gridPos(i, j).z() * _c[1] + 0.5f * _c[2],
				_gridPos(i, j).y() * _gridPos(i, j).y() *
					_gridPos(i, j).z() * _c[3] +
					_gridPos(i, j).x() * _c[4] + 0.5f * _c[5],
				_gridPos(i, j).z() * _gridPos(i, j).z() *
					_gridPos(i, j).x() * _c[6] +
					_gridPos(i, j).y() * _c[7] + _c[8]
			);

	// update vertex normals for most of mesh
	for (unsigned int i = 1; i < Hack::density; ++i)
		for (unsigned int j = 1; j < Hack::density; ++j) {
			Vector up(_vertex(i, j + 1) - _vertex(i, j - 1));
			Vector right(_vertex(i + 1, j) - _vertex(i - 1, j));
			up.normalize();
			right.normalize();
			Vector crossVec(Vector::cross(right, up));
			// Use depth component of normal to compute intensity
			// This way only edges of wisp are bright
			_intensity(i, j) = Common::clamp(
				viscon2 * (viscon1 - std::abs(crossVec.z())),
				0.0f, 1.0f
			);
		}

	// update color
	float h = _HSL.h() + _hueSpeed * Common::elapsedSecs;
	if (h < 0.0f) h += 1.0f;
	if (h > 1.0f) h -= 1.0f;
	float s = _HSL.s() + _saturationSpeed * Common::elapsedSecs;
	if (s <= 0.1f) {
		s = 0.1f;
		_saturationSpeed = -_saturationSpeed;
	}
	if (s >= 1.0f) {
		s = 1.0f;
		_saturationSpeed = -_saturationSpeed;
	}
	_HSL.h() = h;
	_HSL.s() = s;
	_RGB = _HSL;
}

void Wisp::draw() const {
	glPushMatrix();

	if (Hack::wireframe) {
		for (unsigned int i = 1; i < Hack::density; ++i) {
			glBegin(GL_LINE_STRIP);
			for (unsigned int j = 0; j <= Hack::density; ++j) {
				glColor3f(
					_RGB.r() + _intensity(i, j) - 1.0f,
					_RGB.g() + _intensity(i, j) - 1.0f,
					_RGB.b() + _intensity(i, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i, j).x() - _vertex(i, j).x(),
					_gridPos(i, j).y() - _vertex(i, j).y()
				);
				glVertex3fv(_vertex(i, j).get());
			}
			glEnd();
		}
		for (unsigned int j = 1; j < Hack::density; ++j) {
			glBegin(GL_LINE_STRIP);
			for (unsigned int i = 0; i <= Hack::density; ++i) {
				glColor3f(
					_RGB.r() + _intensity(i, j) - 1.0f,
					_RGB.g() + _intensity(i, j) - 1.0f,
					_RGB.b() + _intensity(i, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i, j).x() - _vertex(i, j).x(),
					_gridPos(i, j).y() - _vertex(i, j).y()
				);
				glVertex3fv(_vertex(i, j).get());
			}
			glEnd();
		}
	} else {
		for (unsigned int i = 0; i < Hack::density; ++i) {
			glBegin(GL_TRIANGLE_STRIP);
			for (unsigned int j = 0; j <= Hack::density; ++j) {
				glColor3f(
					_RGB.r() + _intensity(i + 1, j) - 1.0f,
					_RGB.g() + _intensity(i + 1, j) - 1.0f,
					_RGB.b() + _intensity(i + 1, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i + 1, j).x() - _vertex(i + 1, j).x(),
					_gridPos(i + 1, j).y() - _vertex(i + 1, j).y()
				);
				glVertex3fv(_vertex(i + 1, j).get());
				glColor3f(
					_RGB.r() + _intensity(i, j) - 1.0f,
					_RGB.g() + _intensity(i, j) - 1.0f,
					_RGB.b() + _intensity(i, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i, j).x() - _vertex(i, j).x(),
					_gridPos(i, j).y() - _vertex(i, j).y()
				);
				glVertex3fv(_vertex(i, j).get());
			}
			glEnd();
		}
	}

	glPopMatrix();
}

void Wisp::drawAsBackground() const {
	glPushMatrix();
	glTranslatef(_c[0] * 0.2f, _c[1] * 0.2f, 1.6f);

	if (Hack::wireframe) {
		for (unsigned int i = 1; i < Hack::density; ++i) {
			glBegin(GL_LINE_STRIP);
			for (unsigned int j = 0; j <= Hack::density; ++j) {
				glColor3f(
					_RGB.r() + _intensity(i, j) - 1.0f,
					_RGB.g() + _intensity(i, j) - 1.0f,
					_RGB.b() + _intensity(i, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i, j).x() - _vertex(i, j).x(),
					_gridPos(i, j).y() - _vertex(i, j).y()
				);
				glVertex3f(
					_gridPos(i, j).x(),
					_gridPos(i, j).y(),
					_intensity(i, j)
				);
			}
			glEnd();
		}
		for (unsigned int j = 1; j < Hack::density; ++j) {
			glBegin(GL_LINE_STRIP);
			for (unsigned int i = 0; i <= Hack::density; ++i) {
				glColor3f(
					_RGB.r() + _intensity(i, j) - 1.0f,
					_RGB.g() + _intensity(i, j) - 1.0f,
					_RGB.b() + _intensity(i, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i, j).x() - _vertex(i, j).x(),
					_gridPos(i, j).y() - _vertex(i, j).y()
				);
				glVertex3f(
					_gridPos(i, j).x(),
					_gridPos(i, j).y(),
					_intensity(i, j)
				);
			}
			glEnd();
		}
	} else {
		for (unsigned int i = 0; i < Hack::density; ++i) {
			glBegin(GL_TRIANGLE_STRIP);
			for (unsigned int j = 0; j <= Hack::density; ++j) {
				glColor3f(
					_RGB.r() + _intensity(i + 1, j) - 1.0f,
					_RGB.g() + _intensity(i + 1, j) - 1.0f,
					_RGB.b() + _intensity(i + 1, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i + 1, j).x() - _vertex(i + 1, j).x(),
					_gridPos(i + 1, j).y() - _vertex(i + 1, j).y()
				);
				glVertex3f(
					_gridPos(i + 1, j).x(),
					_gridPos(i + 1, j).y(),
					_intensity(i + 1, j)
				);
				glColor3f(
					_RGB.r() + _intensity(i, j) - 1.0f,
					_RGB.g() + _intensity(i, j) - 1.0f,
					_RGB.b() + _intensity(i, j) - 1.0f
				);
				glTexCoord2d(
					_gridPos(i, j).x() - _vertex(i, j).x(),
					_gridPos(i, j).y() - _vertex(i, j).y()
				);
				glVertex3f(
					_gridPos(i, j).x(),
					_gridPos(i, j).y(),
					_intensity(i, j)
				);
			}
			glEnd();
		}
	}

	glPopMatrix();
}
