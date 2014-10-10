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

#include <resources.hh>
#include <skyrocket.hh>
#include <spinner.hh>
#include <star.hh>
#include <vector.hh>

void Spinner::update() {
	_remaining -= Common::elapsedTime;

	if (_remaining <= 0.0f || _pos.y() < 0.0f) {
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	float life = _remaining / _lifetime;
	_brightness = life * life;
	float temp = _lifetime - _remaining;
	if (temp < 0.5f)
		_brightness *= temp * 2.0f;

	_vel.y() -= Common::elapsedTime * 32.0f;
	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	Vector crossVector(Vector::cross(Vector(1.0f, 0.0f, 0.0f), _spinAxis));
	crossVector.normalize();
	crossVector *= 400.0f;

	float dr = _radialVelocity * Common::elapsedTime;
	_starTimer += Common::elapsedTime * _brightness *
		(Common::randomFloat(10.0f) + 90.0f);
	unsigned int stars = (unsigned int)(_starTimer);
	_starTimer -= float(stars);
	for (unsigned int i = 0; i < stars; ++i) {
		Hack::pending.push_back(new Star(_pos, _vel - RotationMatrix(
			UnitQuat(
				_radialPosition + Common::randomFloat(dr),
				_spinAxis
			)
		).transform(crossVector) + Vector(
			Common::randomFloat(20.0f) - 10.0f,
			Common::randomFloat(20.0f) - 10.0f,
			Common::randomFloat(20.0f) - 10.0f
		), 0.612f, Common::randomFloat(0.5f) + 1.5f, _RGB, 15.0f));
	}
	_radialPosition += dr;
	if (_radialPosition > M_PI * 2)
		_radialPosition -= M_PI * 2;

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Spinner::updateCameraOnly() {
	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Spinner::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(20.0f, 20.0f, 20.0f);
		glMultMatrixf(Hack::cameraMat.get());
		glColor4f(_RGB.r(), _RGB.g(), _RGB.b(), _brightness);
		glCallList(Resources::DisplayLists::flares);
	glPopMatrix();
}
