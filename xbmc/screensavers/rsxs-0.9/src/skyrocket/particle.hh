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
#ifndef _PARTICLE_HH
#define _PARTICLE_HH

#include <common.hh>

#include <color.hh>
#include <vector.hh>

#define DEAD_DEPTH (-1000000.0f)
#define PARTICLE_DEAD (1 << 0)
#define PARTICLE_ROCKET (1 << 1)

class Particle {
protected:
	Vector _pos;
	Vector _vel;
	float _drag;

	float _lifetime;
	float _remaining;

	float _depth;

	static RGBColor randomColor() {
		switch (Common::randomInt(6)) {
		case 0:
			return RGBColor(
				1.0f, Common::randomFloat(1.0f), Common::randomFloat(0.2f)
			);
		case 1:
			return RGBColor(
				1.0f, Common::randomFloat(0.2f), Common::randomFloat(1.0f)
			);
		case 2:
			return RGBColor(
				Common::randomFloat(1.0f), 1.0f, Common::randomFloat(0.2f)
			);
		case 3:
			return RGBColor(
				Common::randomFloat(0.2f), 1.0f, Common::randomFloat(1.0f)
			);
		case 4:
			return RGBColor(
				Common::randomFloat(1.0f), Common::randomFloat(0.2f), 1.0f
			);
		default:
			return RGBColor(
				Common::randomFloat(0.2f), Common::randomFloat(1.0f), 1.0f
			);
		}
	}

	Particle(
		const Vector& pos, const Vector& vel, float drag, float lifetime
	) : _pos(pos), _vel(vel), _drag(drag), _lifetime(lifetime),
		_remaining(lifetime) {}
public:
	virtual ~Particle() {}

	virtual void update() = 0;
	virtual void updateCameraOnly() = 0;
	virtual void draw() const = 0;

	bool operator<(const Particle& other) const {
		return _depth > other._depth;
	}
	virtual void illuminate(
		const Vector&, const RGBColor&, float, float
	) {}
	virtual void suck(const Vector& pos, float factor) {
		if (_remaining <= 0.0f)
			return;

		Vector diff(pos - _pos);
		float dSquared = diff.lengthSquared();
		if (dSquared < 250000.0f && dSquared > 0.0f) {
			diff.normalize();
			_vel += diff * (250000.0f - dSquared) * factor;
		}
	}
	virtual void shock(const Vector& pos, float factor) {
		if (_remaining <= 0.0f)
			return;

		Vector diff(_pos - pos);
		float dSquared = diff.lengthSquared();
		if (dSquared < 640000.0f && dSquared > 0.0f) {
			diff.normalize();
			_vel += diff * (640000.0f - dSquared) * factor;
		}
	}
	virtual void stretch(const Vector& pos, float factor) {
		if (_remaining <= 0.0f)
			return;

		Vector diff(pos - _pos);
		float dSquared = diff.lengthSquared();
		if (dSquared < 640000.0f && dSquared > 0.0f) {
			diff.normalize();
			float temp = (640000.0f - dSquared) * factor;
			_vel.x() += diff.x() * temp * 5.0f;
			_vel.y() -= diff.y() * temp;
			_vel.z() += diff.z() * temp * 5.0f;
		}
	}
};

#endif // _PARTICLE_HH
