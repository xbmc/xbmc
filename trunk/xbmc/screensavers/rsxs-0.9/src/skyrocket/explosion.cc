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

#include <bee.hh>
#include <bomb.hh>
#include <color.hh>
#include <explosion.hh>
#include <meteor.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <star.hh>
#include <streamer.hh>
#include <vector.hh>

void Explosion::popSphere(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= speed + Common::randomFloat(50.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(1.0f) + 2.0f, RGB, 30.0f,
			Common::randomInt(100) < int(Hack::explosionSmoke)));
	}
}

void Explosion::popSplitSphere(
	unsigned int n, float speed, const RGBColor& RGB1, const RGBColor& RGB2
) const {
	Vector planeNormal(
		Common::randomFloat(1.0f) - 0.5f,
		Common::randomFloat(1.0f) - 0.5f,
		Common::randomFloat(1.0f) - 0.5f
	);
	planeNormal.normalize();

	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= speed + Common::randomFloat(50.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(1.0f) + 2.0f,
			Vector::dot(planeNormal, velocity) > 0.0f ? RGB1 : RGB2, 30.0f,
			Common::randomInt(100) < int(Hack::explosionSmoke)));
	}
}

void Explosion::popMultiColorSphere(
	unsigned int n, float speed, const RGBColor RGB[3]
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= speed + Common::randomFloat(50.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(1.0f) + 2.0f, RGB[i % 3], 30.0f,
			Common::randomInt(100) < int(Hack::explosionSmoke)));
	}
}

void Explosion::popRing(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	float heading = Common::randomFloat(M_PI);
	float pitch = Common::randomFloat(M_PI);
	float ch = std::cos(heading);
	float sh = std::sin(heading);
	float cp = std::cos(pitch);
	float sp = std::sin(pitch);

	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		float r0 = velocity.x();
		float r1 = velocity.z();
		velocity.y() = sp * r1;
		velocity.z() = cp * r1;
		velocity.x() = ch * r0 + sh * sp * r1;
		velocity.y() = -sh * r0 + ch * sp * r1;

		velocity *= speed;
		velocity += Vector(
			Common::randomFloat(50.0f),
			Common::randomFloat(50.0f),
			Common::randomFloat(50.0f)
		) + _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(1.0f) + 2.0f, RGB, 30.0f,
			Common::randomInt(100) < int(Hack::explosionSmoke)));
	}
}

void Explosion::popStreamers(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= speed + Common::randomFloat(50.0f);
		velocity += _vel;

		Hack::pending.push_back(new Streamer(_pos, velocity, 0.612f,
			Common::randomFloat(1.0f) + 3.0f, RGB, 30.0f));
	}
}

void Explosion::popMeteors(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= speed + Common::randomFloat(50.0f);
		velocity += _vel;

		Hack::pending.push_back(new Meteor(_pos, velocity, 0.612f,
			Common::randomFloat(1.0f) + 3.0f, RGB, 20.0f));
	}
}

void Explosion::popStarBombs(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed
		);
		velocity += _vel;

		Hack::pending.push_back(new Bomb(_pos, velocity, Bomb::BOMB_STARS,
			RGB));
	}
}

void Explosion::popStreamerBombs(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed
		);
		velocity += _vel;

		Hack::pending.push_back(new Bomb(_pos, velocity, Bomb::BOMB_STREAMERS,
			RGB));
	}
}

void Explosion::popMeteorBombs(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed
		);
		velocity += _vel;

		Hack::pending.push_back(new Bomb(_pos, velocity, Bomb::BOMB_METEORS,
			RGB));
	}
}

void Explosion::popCrackerBombs(unsigned int n, float speed) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed,
			Common::randomFloat(speed * 2) - speed
		);
		velocity += _vel;

		Hack::pending.push_back(new Bomb(_pos, velocity, Bomb::BOMB_CRACKER));
	}
}

void Explosion::popBees(
	unsigned int n, float speed, const RGBColor& RGB
) const {
	for (unsigned int i = 0; i < n; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity *= speed;
		velocity += _vel;

		Hack::pending.push_back(new Bee(_pos, velocity, RGB));
	}
}

Explosion::Explosion(
	const Vector& pos, const Vector& vel, Type explosionType,
	const RGBColor& RGB, float lifetime
) : Particle(pos, vel, 0.612f, lifetime), _RGB(RGB), _size(100.0f) {
	switch (explosionType) {
	case EXPLODE_SPHERE:
		_RGB = Particle::randomColor();
		if (!Common::randomInt(10))
			popSphere(225, 1000.0f, _RGB);
		else
			popSphere(175, Common::randomFloat(100.0f) + 400.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_SPLIT_SPHERE:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			if (!Common::randomInt(10))
				popSplitSphere(225, 1000.0f, RGB1, RGB2);
			else
				popSplitSphere(175, Common::randomFloat(100.0f) + 400.0f, RGB1, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_MULTICOLORED_SPHERE:
		{
			RGBColor RGB[] = {
				Particle::randomColor(),
				Particle::randomColor(),
				Particle::randomColor()
			};
			_RGB = (RGB[0] + RGB[1] + RGB[2]) / 3;
			if (!Common::randomInt(10))
				popMultiColorSphere(225, 1000.0f, RGB);
			else
				popMultiColorSphere(175, Common::randomFloat(100.0f) + 400.0f, RGB);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_RING:
		_RGB = Particle::randomColor();
		popRing(70, Common::randomFloat(100.0f) + 400.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_DOUBLE_SPHERE:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			popSphere(90, Common::randomFloat(50.0f) + 200.0f, RGB1);
			popSphere(150, Common::randomFloat(100.0f) + 500.0f, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_SPHERE_INSIDE_RING:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			popSphere(150, Common::randomFloat(50.0f) + 200.0f, RGB1);
			popRing(70, Common::randomFloat(100.0f) + 500.0f, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STREAMERS:
		_RGB = Particle::randomColor();
		popStreamers(40, Common::randomFloat(100.0f) + 400.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_METEORS:
		_RGB = Particle::randomColor();
		popMeteors(40, Common::randomFloat(100.0f) + 400.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STARS_INSIDE_STREAMERS:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			popSphere(90, Common::randomFloat(50.0f) + 200.0f, RGB1);
			popStreamers(25, Common::randomFloat(100.0f) + 500.0f, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STARS_INSIDE_METEORS:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			popSphere(90, Common::randomFloat(50.0f) + 200.0f, RGB1);
			popMeteors(25, Common::randomFloat(100.0f) + 500.0f, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STREAMERS_INSIDE_STARS:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			popStreamers(20, Common::randomFloat(100.0f) + 450.0f, RGB1);
			popSphere(150, Common::randomFloat(50.0f) + 500.0f, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_METEORS_INSIDE_STARS:
		{
			RGBColor RGB1 = Particle::randomColor();
			RGBColor RGB2 = Particle::randomColor();
			_RGB = (RGB1 + RGB2) / 2;
			popMeteors(20, Common::randomFloat(100.0f) + 450.0f, RGB1);
			popSphere(150, Common::randomFloat(50.0f) + 500.0f, RGB2);
		}
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STAR_BOMBS:
		_RGB = Particle::randomColor();
		popStarBombs(8, Common::randomFloat(100.0f) + 300.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STREAMER_BOMBS:
		_RGB = Particle::randomColor();
		popStreamerBombs(8, Common::randomFloat(100.0f) + 300.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_METEOR_BOMBS:
		_RGB = Particle::randomColor();
		popMeteorBombs(8, Common::randomFloat(100.0f) + 300.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_CRACKER_BOMBS:
		_RGB = Particle::randomColor();
		popCrackerBombs(250, Common::randomFloat(50.0f) + 150.0f);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
			Resources::Sounds::popper->play(_pos);
		}
		break;
	case EXPLODE_BEES:
		_RGB = Particle::randomColor();
		popBees(30, 10.0f, _RGB);
		if (Hack::volume > 0.0f && Common::randomInt(2))
			Resources::Sounds::whistle->play(_pos);
		break;
	case EXPLODE_FLASH:
		_RGB.set(1.0f, 1.0f, 1.0f);
		_size = 150.0f;
		if (Hack::volume > 0.0f) {
			Resources::Sounds::boom4->play(_pos);
			if (Common::randomInt(2))
				Resources::Sounds::whistle->play(_pos);
		}
		break;
	case EXPLODE_SUCKER:
	case EXPLODE_BIGMAMA:
		_size = 200.0f;
		break;
	case EXPLODE_SHOCKWAVE:
		_size = 300.0f;
		break;
	case EXPLODE_STRETCHER:
		_size = 400.0f;
		break;
	case EXPLODE_STARS_FROM_BOMB:
		_RGB = Particle::randomColor();
		popSphere(30, 100.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_STREAMERS_FROM_BOMB:
		_RGB = Particle::randomColor();
		popStreamers(10, 100.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
	case EXPLODE_METEORS_FROM_BOMB:
		_RGB = Particle::randomColor();
		popMeteors(10, 100.0f, _RGB);
		if (Hack::volume > 0.0f) {
			switch (Common::randomInt(4)) {
			case 0: Resources::Sounds::boom1->play(_pos); break;
			case 1: Resources::Sounds::boom2->play(_pos); break;
			case 2: Resources::Sounds::boom3->play(_pos); break;
			case 3: Resources::Sounds::boom4->play(_pos); break;
			}
		}
		break;
 	default:
 		WARN("Unknown explosion type: " << explosionType);
 		break;
 	}
}

void Explosion::update() {
	_remaining -= Common::elapsedTime;

	if (_remaining <= 0.0f || _pos.y() <= 0.0f) {
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	float life = _remaining / _lifetime;
	_brightness = life * life;

	Hack::illuminate(_pos, _RGB, _brightness, 640000.0f, 2560000.0f);
	Hack::flare(_pos, _RGB, _brightness);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Explosion::updateCameraOnly() {
	Hack::flare(_pos, _RGB, _brightness);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Explosion::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(_size, _size, _size);
		glScalef(_brightness, _brightness, _brightness);
		glMultMatrixf(Hack::cameraMat.get());
		glColor4f(_RGB.r(), _RGB.g(), _RGB.b(), _brightness);
		glCallList(Resources::DisplayLists::flares);
	glPopMatrix();
}
