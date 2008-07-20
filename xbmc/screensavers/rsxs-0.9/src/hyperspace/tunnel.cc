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

#include <caustic.hh>
#include <color.hh>
#include <spline.hh>
#include <tunnel.hh>
#include <vector.hh>

#define TUNNEL_RESOLUTION 20

namespace Tunnel {
	unsigned int _numSections;
	unsigned int _section;

	float _radius;
	float _widthOffset;
	float _texSpin;

	stdx::dim3<Vector, TUNNEL_RESOLUTION + 1, TUNNEL_RESOLUTION + 1> _v;
	stdx::dim3<Vector, TUNNEL_RESOLUTION + 1, TUNNEL_RESOLUTION + 1> _t;
	stdx::dim3<RGBColor, TUNNEL_RESOLUTION + 1, TUNNEL_RESOLUTION + 1> _c;

	float _loH, _loS, _loL;
	float _hiH, _hiS, _hiL;

	void make();
};

void Tunnel::init() {
	CausticTextures::init();

	_radius = 0.1f;
	_widthOffset = 0.0f;
	_texSpin = 0.0f;

	_numSections = Spline::points - 5;
	_section = 0;

	_v.resize(_numSections);
	_t.resize(_numSections);
	_c.resize(_numSections);

	_loH = _loS = _hiH = _hiS = 0.0f;
	_loL = _hiL = M_PI;
}

void Tunnel::make() {
	_widthOffset += Common::elapsedTime * 1.5f;
	while (_widthOffset >= M_PI * 2.0f)
		_widthOffset -= M_PI * 2.0f;
	_texSpin += Common::elapsedTime * 0.1f;
	while (_texSpin >= M_PI * 2.0f)
		_texSpin -= M_PI * 2.0f;

	_loH += Common::elapsedTime * 0.04f;
	_hiH += Common::elapsedTime * 0.15f;
	_loS += Common::elapsedTime * 0.04f;
	_hiS += Common::elapsedTime;
	_loL += Common::elapsedTime * 0.04f;
	_hiL += Common::elapsedTime * 0.5f;

	while (_loH > M_PI * 2.0f) _loH -= M_PI * 2.0f;
	while (_hiH > M_PI * 2.0f) _hiH -= M_PI * 2.0f;
	while (_loS > M_PI * 2.0f) _loS -= M_PI * 2.0f;
	while (_hiS > M_PI * 2.0f) _hiS -= M_PI * 2.0f;
	while (_loL > M_PI * 2.0f) _loL -= M_PI * 2.0f;
	while (_hiL > M_PI * 2.0f) _hiL -= M_PI * 2.0f;

	unsigned int n = _numSections;
	for (unsigned int k = 0; k < n; ++k) {
		// create new vertex data for this section
		for (unsigned int i = 0; i <= TUNNEL_RESOLUTION; ++i) {
			float where = float(i) / float(TUNNEL_RESOLUTION);

			Vector pos(Spline::at(k + 2, where));
			Vector dir(Spline::direction(k + 2, where));
			RotationMatrix rot(RotationMatrix::lookAt(Vector(), dir, Vector(0.0f, 1.0f, 0.0f)));

			for (unsigned int j = 0; j <= TUNNEL_RESOLUTION; ++j) {
				float angle = float(j) * M_PI * 2.0f / float(TUNNEL_RESOLUTION);
				Vector vertex(rot.transform(Vector(
					(_radius + _radius * 0.5f * std::cos(2.0f * pos.x() + _widthOffset)) * std::cos(angle),
					(_radius + _radius * 0.5f * std::cos(pos.z() + _widthOffset)) * std::sin(angle),
					0.0f
				)));

				// set vertex coordinates
				_v(k, i, j) = pos + vertex;

				// set texture coordinates
				_t(k, i, j).x() = 4.0f * float(i) / float(TUNNEL_RESOLUTION);
				_t(k, i, j).y() = float(j) / float(TUNNEL_RESOLUTION) + std::cos(_texSpin);

				// set colors
				HSLColor HSL(
					2.0f * std::cos(0.1f * _v(k, i, j).x() + _loH) - 1.0f,
					0.25f * (std::cos(0.013f * _v(k, i, j).y() - _loS)
						+ std::cos(_v(k, i, j).z() + _hiS) + 2.0f),
					2.0f * std::cos(0.01f * _v(k, i, j).z() + _loL)
						+ std::cos(0.4f * _v(k, i, j).x() - _hiL)
						+ 0.3f * std::cos(4.0f * (_v(k, i, j).x() + _v(k, i, j).y() + _v(k, i, j).z()))
				);
				HSL.clamp();
				if (HSL.s() > 0.7f) HSL.s() = 0.7f;
				_c(k, i, j) = RGBColor(HSL);
			}
		}
	}
}

void Tunnel::draw() {
	Tunnel::make();

	glPushAttrib(GL_ENABLE_BIT);
		glEnable(GL_TEXTURE_2D);
		CausticTextures::use();

		unsigned int n = _numSections;
		if (Hack::shaders) {
			for (unsigned int k = 0; k < n; ++k) {
				for (unsigned int i = 0; i < TUNNEL_RESOLUTION; ++i) {
					glBegin(GL_TRIANGLE_STRIP);
					for (unsigned int j = 0; j <= TUNNEL_RESOLUTION; ++j) {
						glColor4f(_c(k, i + 1, j).r(), _c(k, i + 1, j).g(), _c(k, i + 1, j).b(), Hack::lerp);
						glTexCoord2fv(_t(k, i + 1, j).get());
						glVertex3fv(_v(k, i + 1, j).get());
						glColor4f(_c(k, i, j).r(), _c(k, i, j).g(), _c(k, i, j).b(), Hack::lerp);
						glTexCoord2fv(_t(k, i, j).get());
						glVertex3fv(_v(k, i, j).get());
					}
					glEnd();
				}
			}
		} else {
			for (unsigned int k = 0; k < n; ++k) {
				for (unsigned int i = 0; i < TUNNEL_RESOLUTION; ++i) {
					glBegin(GL_TRIANGLE_STRIP);
					for (unsigned int j = 0; j <= TUNNEL_RESOLUTION; ++j) {
						glColor3fv(_c(k, i + 1, j).get());
						glTexCoord2fv(_t(k, i + 1, j).get());
						glVertex3fv(_v(k, i + 1, j).get());
						glColor3fv(_c(k, i, j).get());
						glTexCoord2fv(_t(k, i, j).get());
						glVertex3fv(_v(k, i, j).get());
					}
					glEnd();
				}
			}
		}

	glPopAttrib();
}
