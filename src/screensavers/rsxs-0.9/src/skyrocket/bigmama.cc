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

#include <bigmama.hh>
#include <color.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <star.hh>
#include <vector.hh>

BigMama::BigMama(const Vector& pos, const Vector& vel) :
		Particle(pos, vel, 0.612f, 5.0f) {
	Hack::pending.push_back(new Star(_pos, _vel + Vector(0.0f, 15.0f, 0.0f),
		0.0f, 3.0f, RGBColor(1.0f, 1.0f, 0.9f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel - Vector(0.0f, 15.0f, 0.0f),
		0.0f, 3.0f, RGBColor(1.0f, 1.0f, 0.9f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel + Vector(0.0f, 45.0f, 0.0f),
		0.0f, 3.5f, RGBColor(1.0f, 1.0f, 0.6f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel - Vector(0.0f, 45.0f, 0.0f),
		0.0f, 3.5f, RGBColor(1.0f, 1.0f, 0.6f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel + Vector(0.0f, 75.0f, 0.0f),
		0.0f, 4.0f, RGBColor(1.0f, 0.5f, 0.3f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel - Vector(0.0f, 75.0f, 0.0f),
		0.0f, 4.0f, RGBColor(1.0f, 0.5f, 0.3f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel + Vector(0.0f, 105.0f, 0.0f),
		0.0f, 4.5f, RGBColor(1.0f, 0.0f, 0.0f), 400.0f));
	Hack::pending.push_back(new Star(_pos, _vel - Vector(0.0f, 105.0f, 0.0f),
		0.0f, 4.5f, RGBColor(1.0f, 0.0f, 0.0f), 400.0f));

	RGBColor RGB(Particle::randomColor());

	for (unsigned int i = 0; i < 75; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= 600.0f + Common::randomFloat(50.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(2.0f) + 2.0f, RGB, 30.0f));
	}

	RGB = Particle::randomColor();

	for (unsigned int i = 0; i < 50; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			0.0f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity.x() *= 1000.0f + Common::randomFloat(100.0f);
		velocity.y() += Common::randomFloat(100.0f) - 50.0f;
		velocity.z() *= 1000.0f + Common::randomFloat(100.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(6.0f) + 3.0f, RGB, 100.0f));
	}

	if (Hack::volume > 0.0f)
		Resources::Sounds::nuke->play(_pos);
}

void BigMama::update() {
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

	float life = _remaining / _lifetime;
	_brightness = life * 2.0f - 1.0f;
	if (_brightness < 0.0f)
		_brightness = 0.0f;
	_size += 1500.0f * Common::elapsedTime;

	Hack::superFlare(_pos, RGBColor(0.6f, 0.6f, 1.0f), _brightness);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void BigMama::updateCameraOnly() {
	Hack::superFlare(_pos, RGBColor(0.6f, 0.6f, 1.0f), _brightness);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void BigMama::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(_size, _size, _size);
		glMultMatrixf(Hack::cameraMat.get());
		glColor4f(0.6f, 0.6f, 1.0f, _brightness);
		glCallList(Resources::DisplayLists::flares + 2);
	glPopMatrix();
}
