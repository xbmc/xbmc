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

#include <goo.hh>
#include <hyperspace.hh>
#include <implicit.hh>
#include <nebula.hh>
#include <vector.hh>

namespace Goo {
	int _resolution;
	float _unitSize;
	float _volumeSize;
	int _arraySize;

	std::vector<Implicit> _surface;
	std::vector<bool> _useSurface;

	float _centerX, _centerZ;
	float _shiftX, _shiftZ;

	float _goo[4];
	float _gooPhase[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
	float _gooSpeed[4] = {
		0.1f + Common::randomFloat(0.4f),
		0.1f + Common::randomFloat(0.4f),
		0.1f + Common::randomFloat(0.4f),
		0.1f + Common::randomFloat(0.4f)
	};
	float _gooRGB[4];

	float function(const Vector&);
};

#define AT(x, y) ((x) * _arraySize + (y))

void Goo::init() {
	_volumeSize = 2.0f;
	_resolution = (Hack::resolution < 5 ? 5 : Hack::resolution);
	_unitSize = _volumeSize / float(_resolution);
	_arraySize = 2 * int(0.99f + Hack::fogDepth / _volumeSize);

	Implicit::init(_resolution, _resolution, _resolution, _unitSize);

	stdx::construct_n(_surface, _arraySize * _arraySize, &function);
	stdx::construct_n(_useSurface, _arraySize * _arraySize, false);
}

void Goo::update(float heading, float fov) {
	float halfFov = 0.5f * fov;

	_centerX = _unitSize * float(int(0.5f + Hack::camera.x() / _unitSize));
	_centerZ = _unitSize * float(int(0.5f + Hack::camera.z() / _unitSize));

	float clip[3][2];
	clip[0][0] = std::cos(heading + halfFov);
	clip[0][1] = -std::sin(heading + halfFov);
	clip[1][0] = -std::cos(heading - halfFov);
	clip[1][1] = std::sin(heading - halfFov);
	clip[2][0] = std::sin(heading);
	clip[2][1] = -std::cos(heading);

	for (int i = 0; i < _arraySize; ++i) {
		for (int j = 0; j < _arraySize; ++j) {
			_shiftX = _volumeSize * (0.5f + float(int(i) - int(_arraySize) / 2));
			_shiftZ = _volumeSize * (0.5f + float(int(j) - int(_arraySize) / 2));
			if (_shiftX * clip[0][0] + _shiftZ * clip[0][1] > _volumeSize * -M_SQRT2) {
				if (_shiftX * clip[1][0] + _shiftZ * clip[1][1] > _volumeSize * -M_SQRT2) {
					if (_shiftX * clip[2][0] + _shiftZ * clip[2][1] < Hack::fogDepth + _volumeSize * M_SQRT2) {
						_shiftX += _centerX;
						_shiftZ += _centerZ;
						_surface[AT(i, j)].update(0.4f);
						_useSurface[AT(i, j)] = true;
					}
				}
			}
		}
	}

	// calculate color
	static float gooRGBphase[3] = { -0.1f, -0.1f, -0.1f };
	static float gooRGBspeed[3] = {
		Common::randomFloat(0.02f) + 0.02f,
		Common::randomFloat(0.02f) + 0.02f,
		Common::randomFloat(0.02f) + 0.02f
	};
	for (int i = 0; i < 3; ++i) {
		gooRGBphase[i] += gooRGBspeed[i] * Common::elapsedTime;
		if (gooRGBphase[i] >= M_PI * 2.0f) gooRGBphase[i] -= M_PI * 2.0f;
		_gooRGB[i] = std::sin(gooRGBphase[i]);
		if (_gooRGB[i] < 0.0f) _gooRGB[i] = 0.0f;
	}

	// update goo function constants
	for (int i = 0; i < 4; ++i) {
		_gooPhase[i] += _gooSpeed[i] * Common::elapsedTime;
		if (_gooPhase[i] >= M_PI * 2.0f) _gooPhase[i] -= M_PI * 2.0f;
		_goo[i] = 0.25f * std::cos(_gooPhase[i]);
	}
}

void Goo::draw() {
	glPushAttrib(GL_ENABLE_BIT);
		Nebula::use();
		if (Hack::shaders)
			_gooRGB[3] = Hack::lerp;
		else
			_gooRGB[3] = 1.0f;

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glEnable(GL_BLEND);
		glColor4fv(_gooRGB);

		for (int i = 0; i < _arraySize; ++i) {
			for (int j = 0; j < _arraySize; ++j) {
				if (_useSurface[AT(i, j)]) {
					float shiftX = _centerX + _volumeSize * (0.5f + float(int(i) - int(_arraySize) / 2));
					float shiftZ = _centerZ + _volumeSize * (0.5f + float(int(j) - int(_arraySize) / 2));
					glPushMatrix();
						glTranslatef(shiftX, 0.0f, shiftZ);
						_surface[AT(i, j)].draw();
					glPopMatrix();
					_useSurface[AT(i, j)] = false;
				}
			}
		}
	glPopAttrib();
}

float Goo::function(const Vector& XYZ) {
	float pX = XYZ.x() + _shiftX;
	float pZ = XYZ.z() + _shiftZ;
	float camX = pX - Hack::camera.x();
	float camZ = pZ - Hack::camera.z();

	return
		// This first term defines upper and lower surfaces.
		XYZ.y() * XYZ.y() * 1.25f
		// These terms make the surfaces wavy.
		+ _goo[0] * std::cos(pX - 2.71f * XYZ.y())
		+ _goo[1] * std::cos(4.21f * XYZ.y() + pZ)
		+ _goo[2] * std::cos(1.91f * pX - 1.67f * pZ)
		+ _goo[3] * std::cos(1.53f * pX + 1.11f * XYZ.y() + 2.11f * pZ)
		// The last term creates a bubble around the eyepoint so it doesn't
		// punch through the surface.
		- 0.1f / (camX * camX + XYZ.y() * XYZ.y() + camZ * camZ);
}
