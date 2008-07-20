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
#ifndef _BEE_HH
#define _BEE_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <vector.hh>

class Bee : public Particle {
private:
	RGBColor _RGB;
	float _brightness;

	Vector _accel;
	Vector _accelSpeed;

	Vector _sparkPos;
public:
	Bee(const Vector& pos, const Vector& vel, const RGBColor& RGB) :
		Particle(pos, vel, 0.3f, Common::randomFloat(1.0f) + 2.5f),
		_RGB(RGB), _accel(
			Common::randomFloat(M_PI * 2),
			Common::randomFloat(M_PI * 2),
			Common::randomFloat(M_PI * 2)
		), _accelSpeed(
			Common::randomFloat(M_PI * 2),
			Common::randomFloat(M_PI * 2),
			Common::randomFloat(M_PI * 2)
		), _sparkPos(_pos) {}

	void update();
	void updateCameraOnly();
	void draw() const;
};

#endif // _BEE_HH
