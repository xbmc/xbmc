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
#include <skyrocket.hh>
#include <smoke.hh>
#include <vector.hh>

float Smoke::_times[] = { 0.4f, 0.8f, 0.4f, 2.0f, 0.4f, 0.8f, 0.4f, 4.0f };
unsigned int Smoke::_timeIndex;

void Smoke::update() {
	if (Hack::frameToggle != _frameToggle) {
		_addedRGB.set(0.0f, 0.0f, 0.0f);
		_frameToggle = Hack::frameToggle;
	}

	_remaining -= Common::elapsedTime;
	if (_remaining <= 0.0f || _pos.y() < 0.0f) {
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	_brightness = _remaining / _lifetime * 0.7f;
	_size += (30.0f - _size) * (1.2f * Common::elapsedTime);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Smoke::updateCameraOnly() {
	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Smoke::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
	glTranslatef(_pos.x(), _pos.y(), _pos.z());
	glScalef(_size, _size, _size);
	glMultMatrixf(Hack::cameraMat.get());
	glColor4f(
		_RGB.r() + _addedRGB.r(),
		_RGB.g() + _addedRGB.g(),
		_RGB.b() + _addedRGB.b(),
		_brightness
	);
	glCallList(_list);
	glPopMatrix();
}

void Smoke::illuminate(const Vector& pos, const RGBColor& RGB,
		float brightness, float smokeDistanceSquared) {
	if (Hack::frameToggle != _frameToggle) {
		_addedRGB.set(0.0f, 0.0f, 0.0f);
		_frameToggle = Hack::frameToggle;
	}
	float dSquared = (_pos - pos).lengthSquared();
	if (dSquared < smokeDistanceSquared) {
		float temp = (smokeDistanceSquared - dSquared) / smokeDistanceSquared;
		temp *= temp * brightness;
		_addedRGB += RGB * temp;
	}
}
