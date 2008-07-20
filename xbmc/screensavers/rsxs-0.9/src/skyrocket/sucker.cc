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
#include <explosion.hh>
#include <resources.hh>
#include <shockwave.hh>
#include <skyrocket.hh>
#include <star.hh>
#include <sucker.hh>
#include <vector.hh>

Sucker::Sucker(const Vector& pos, const Vector& vel) :
		Particle(pos, vel, 0.612f, 4.0f) {
	RGBColor RGB(Particle::randomColor());
	float heading = Common::randomFloat(M_PI);
	float pitch = Common::randomFloat(M_PI);
	float ch = std::cos(heading);
	float sh = std::sin(heading);
	float cp = std::cos(pitch);
	float sp = std::sin(pitch);

	for (unsigned int i = 0; i < 90; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			0.0f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		float rx = velocity.x();
		float rz = velocity.z();
		float ry = sp * rz;
		rz = cp * rz;
		float temp = rx;
		rx = ch * temp + sh * ry;
		ry = -sh * temp + ch * ry;
		rx *= 350.0f + Common::randomFloat(30.0f);
		ry *= 350.0f + Common::randomFloat(30.0f);
		rz *= 350.0f + Common::randomFloat(30.0f);

		velocity = Vector(rx, ry, rz) + _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(2.0f) + 2.0f, RGB, 30.0f));
	}

	RGB = Particle::randomColor();
	heading = Common::randomFloat(M_PI);
	pitch = Common::randomFloat(M_PI);
	ch = std::cos(heading);
	sh = std::sin(heading);
	cp = std::cos(pitch);
	sp = std::sin(pitch);

	for (unsigned int i = 0; i < 90; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			0.0f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		float rx = velocity.x();
		float rz = velocity.z();
		float ry = sp * rz;
		rz = cp * rz;
		float temp = rx;
		rx = ch * temp + sh * ry;
		ry = -sh * temp + ch * ry;
		rx *= 600.0f + Common::randomFloat(50.0f);
		ry *= 600.0f + Common::randomFloat(50.0f);
		rz *= 600.0f + Common::randomFloat(50.0f);

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(2.0f) + 2.0f, RGB, 30.0f));
	}

	if (Hack::volume > 0.0f)
		Resources::Sounds::suck->play(_pos);
}

void Sucker::update() {
	_remaining -= Common::elapsedTime;

	if (_remaining <= 0.0f || _pos.y() <= 0.0f) {
		Hack::pending.push_back(new Shockwave(_pos, _vel));
		Hack::pending.push_back(new Explosion(_pos, _vel,
			Explosion::EXPLODE_SHOCKWAVE, RGBColor(1.0f, 1.0f, 1.0f), 3.0f));
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	_vel.y() -= Common::elapsedTime * 32.0f;
	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	_brightness = _remaining / _lifetime;
	_size = 250.0f * _brightness;

	Hack::suck(_pos, (1.0f - _brightness) * 0.01f * Common::elapsedTime);
	Hack::superFlare(_pos, RGBColor(1.0f, 1.0f, 1.0f), _brightness);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Sucker::updateCameraOnly() {
	Hack::superFlare(_pos, RGBColor(1.0f, 1.0f, 1.0f), _brightness);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Sucker::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(_size, _size, _size);
		glMultMatrixf(Hack::cameraMat.get());
		glColor4f(1.0f, 1.0f, 1.0f, _brightness);
		glCallList(Resources::DisplayLists::flares + 2);
	glPopMatrix();
}

void Sucker::suck(const Vector&, float) {
}
