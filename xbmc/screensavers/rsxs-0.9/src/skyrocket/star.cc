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

#include <skyrocket.hh>
#include <star.hh>
#include <smoke.hh>
#include <vector.hh>

void Star::update() {
	_remaining -= Common::elapsedTime;

	if (_remaining <= 0.0f || _pos.y() <= 0.0f) {
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	_vel.y() -= Common::elapsedTime * 32.0f;
	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	float temp = (_lifetime - _remaining) / _lifetime;
	_brightness = 1.0f - temp * temp * temp * temp;

	if (_smoker && Hack::smoke) {
		Vector step(_pos - _smokePos);
		float distance = step.normalize();

		if (distance > 2.0f) {
			unsigned int n = (unsigned int)(distance / 2.0f);
			step *= 2.0f;
			for (unsigned int i = 0; i < n; ++i) {
				Hack::pending.push_back(new Smoke(_smokePos,
					Vector(
						Common::randomFloat(20.0f) - 10.0f,
						Common::randomFloat(20.0f) - 10.0f,
						Common::randomFloat(20.0f) - 10.0f
					))
				);
				_smokePos += step;
			}
		}
	}

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Star::updateCameraOnly() {
	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Star::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(_size, _size, _size);
		glMultMatrixf(Hack::cameraMat.get());
		glColor4f(_RGB.r(), _RGB.g(), _RGB.b(), _brightness);
		glCallList(_list);
	glPopMatrix();
}
