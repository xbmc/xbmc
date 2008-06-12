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

#include <fountain.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <star.hh>
#include <vector.hh>

void Fountain::update() {
	_remaining -= Common::elapsedTime;

	if (_remaining <= 0.0f) {
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	float life = _remaining / _lifetime;
	_brightness = life * life;
	float temp = _lifetime - _remaining;
	if (temp < 0.5f)
		_brightness *= temp * 2.0f;

	_starTimer += Common::elapsedTime * _brightness *
		(Common::randomFloat(10.0f) + 10.0f);
	unsigned int stars = (unsigned int)_starTimer;
	_starTimer -= float(stars);
	for (unsigned int i = 0; i < stars; i++) {
		Vector starPos(
			0.0f, Common::randomFloat(Common::elapsedTime * 100.0f), 0.0f
		);
		starPos += _pos;
		if (starPos.y() > 50.0f) starPos.y() = 50.0f;
		Hack::pending.push_back(new Star(starPos, Vector(
			Common::randomFloat(20.0f) - 10.0f,
			Common::randomFloat(30.0f) + 100.0f,
			Common::randomFloat(20.0f) - 10.0f
		), 0.342f, Common::randomFloat(1.0f) + 2.0f, _RGB, 10.0f));
	}

	Hack::illuminate(_pos, _RGB, _brightness, 40000.0f, 0.0f);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Fountain::updateCameraOnly() {
	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Fountain::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(30.0f, 30.0f, 30.0f);
		glMultMatrixf(Hack::cameraMat.get());
		glColor4f(_RGB.r(), _RGB.g(), _RGB.b(), _brightness);
		glCallList(Resources::DisplayLists::flares);
	glPopMatrix();
}
