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
#include <rocket.hh>
#include <skyrocket.hh>
#include <smoke.hh>
#include <spinner.hh>
#include <star.hh>
#include <stretcher.hh>
#include <sucker.hh>
#include <vector.hh>

Rocket::Rocket(Explosion::Type explosionType) :
		Particle(
			Vector(
				Common::randomFloat(200.0f) - 100.0f,
				5.0f,
				Common::randomFloat(200.0f) - 100.0f
			), Vector(0.0f, 60.0f, 0.0f), 0.281f,	// terminal velocity = 50 ft/s
			Common::randomFloat(2.0f) + 5.0f
		), _RGB(
			Common::randomFloat(0.7f) + 0.3f,
			Common::randomFloat(0.7f) + 0.3f,
			0.3f
		), _brightness(0.0f),
		_thrust(Common::randomFloat(100.0f) + 200.0f),
		_thrustLifetime(_lifetime * (Common::randomFloat(0.1f) + 0.6f)),
		_spin(Common::randomFloat(2.0f * M_PI) - M_PI),
		_rotation(Common::randomFloat(std::abs(_spin) * 0.6f)),
		_direction(UnitQuat::heading(Common::randomFloat(M_PI * 2))),
		_smokePos(_pos), _starPos(_pos),
		_explosionType(explosionType) {
	// Crash the occasional rocket
	if (!Common::randomInt(200)) {
		_spin = 0.0f;
		_rotation = Common::randomFloat(M_PI / 2.0f) + M_PI / 3.0f;
	}
	++Hack::numRockets;

	if (Hack::volume > 0.0f) {
		if (Common::randomInt(2))
			Resources::Sounds::launch1->play(_pos);
		else
			Resources::Sounds::launch2->play(_pos);
	}
}

void Rocket::update() {
	_remaining -= Common::elapsedTime;

	bool alive = _remaining > 0.0f && _pos.y() >= 0.0f;
	if (!alive) {
		switch (_explosionType) {
		case Explosion::EXPLODE_SPINNER:
			Hack::pending.push_back(new Spinner(_pos, _vel));
			break;
		case Explosion::EXPLODE_SUCKER:
			Hack::pending.push_back(new Sucker(_pos, _vel));
			Hack::pending.push_back(new Explosion(_pos, _vel,
				Explosion::EXPLODE_SUCKER, RGBColor(1.0f, 1.0f, 1.0f), 4.0f));
			break;
		case Explosion::EXPLODE_STRETCHER:
			Hack::pending.push_back(new Stretcher(_pos, _vel));
			Hack::pending.push_back(new Explosion(_pos, _vel,
				Explosion::EXPLODE_STRETCHER, RGBColor(1.0f, 1.0f, 1.0f), 4.0f));
			break;
		default:
			Hack::pending.push_back(new Explosion(_pos, _vel, _explosionType));
			break;
		}
		_depth = DEAD_DEPTH;
		--Hack::numRockets;
		++Hack::numDead;
		return;
	}

	if (_remaining + _thrustLifetime > _lifetime) {
		_brightness += 2.0f * Common::elapsedTime;
		if (_brightness > 1.0f)
			_brightness = 1.0f;
	} else {
		_brightness -= Common::elapsedTime;
		if (_brightness < 0.0f)
			_brightness = 0.0f;
	}

	_direction.multiplyBy(UnitQuat::heading(_spin * Common::elapsedTime));
	_direction.multiplyBy(UnitQuat::pitch(_rotation * Common::elapsedTime));
	_directionMat = RotationMatrix(_direction);

	if (_remaining + _thrustLifetime > _lifetime)
		_vel += _direction.up() * _thrust * Common::elapsedTime;

	_vel.y() -= Common::elapsedTime * 32.0f;
	float temp = 1.0f / (1.0f + _drag * Common::elapsedTime);
	_vel *= temp * temp;

	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	Vector step(_pos - _smokePos);
	float distance = step.normalize();

	if (Hack::smoke && distance > 2.0f) {
		unsigned int n = (unsigned int)(distance / 2.0f);
		step *= 2.0f;
		if (_remaining + _thrustLifetime > _lifetime) {
			// exhaustDirection is the exhaust direction and velocity
			Vector exhaustDirection(step * -_thrust *
				(_remaining + _thrustLifetime - _lifetime) / _lifetime);
			for (unsigned int i = 0; i < n; ++i) {
				Hack::pending.push_back(new Smoke(_smokePos,
					exhaustDirection + Vector(
					Common::randomFloat(20.0f) - 10.0f,
					Common::randomFloat(20.0f) - 10.0f,
					Common::randomFloat(20.0f) - 10.0f
				)));
				_smokePos += step;
			}
		} else {
			for (unsigned int i = 0; i < n; ++i) {
				Hack::pending.push_back(new Smoke(_smokePos,
				Vector(
					Common::randomFloat(20.0f) - 10.0f,
					Common::randomFloat(20.0f) - 10.0f,
					Common::randomFloat(20.0f) - 10.0f
				)));
				_smokePos += step;
			}
		}
	}

	step = _pos - _starPos;
	distance = step.normalize();

	if (_remaining + _thrustLifetime > _lifetime && distance > 2.5f) {
		// starDirection is the star direction and velocity
		Vector starDirection(_vel);
		starDirection.normalize();
		starDirection *= _thrust * (_lifetime - _thrustLifetime - _remaining) /
			_lifetime;
		unsigned int n = (unsigned int)(distance / 2.5f);
		step *= 2.5f;
		for (unsigned int i = 0; i < n; ++i) {
			Hack::pending.push_back(new Star(_starPos + Common::randomFloat(1.0f),
				starDirection + Vector(
					Common::randomFloat(60.0f) - 30.0f,
					Common::randomFloat(60.0f) - 30.0f,
					Common::randomFloat(60.0f) - 30.0f
				), 0.612f, Common::randomFloat(0.2f) + 0.1f, _RGB,
				8.0f * _remaining / _lifetime, false,
				Resources::DisplayLists::flares + 3)
			);
			_starPos += step;
		}
	}

	Hack::illuminate(_pos, _RGB, _brightness, 40000.0f, 0.0f);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Rocket::updateCameraOnly() {
	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Rocket::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(3.0f, 3.0f, 3.0f);
		glMultMatrixf(_directionMat.get());
		glDisable(GL_TEXTURE_2D);
		glColor4f(
			_RGB.r() + Hack::ambient * 0.005f,
			_RGB.g() + Hack::ambient * 0.005f,
			_RGB.b() + Hack::ambient * 0.005f,
			_brightness
		);
		glCallList(Resources::DisplayLists::rocket);
		glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}
