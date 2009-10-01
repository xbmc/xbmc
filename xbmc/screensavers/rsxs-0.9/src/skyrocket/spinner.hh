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
#ifndef _SPINNER_HH
#define _SPINNER_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <vector.hh>

class Spinner : public Particle {
private:
	RGBColor _RGB;
	float _brightness;

	float _radialVelocity;
	float _radialPosition;
	UnitVector _spinAxis;

	float _starTimer;
public:
	Spinner(const Vector& pos, const Vector& vel) :
		Particle(pos, vel, 0.612f,	// terminal velocity = 20 ft/s
			Common::randomFloat(2.0f) + 6.0f
		), _RGB(Particle::randomColor()), _brightness(0.0f),
		_radialVelocity(Common::randomFloat(3.0f) + 12.0f),
		_radialPosition(0.0f),
		_spinAxis(UnitVector::of(
			Vector(
				Common::randomFloat(2.0f) - 1.0f,
				Common::randomFloat(2.0f) - 1.0f,
				Common::randomFloat(2.0f) - 1.0f
			)
		)), _starTimer(0.0f)
	{
		if (Hack::volume > 0.0f) {
			if (Common::randomInt(2))
				Resources::Sounds::launch1->play(_pos);
			else
				Resources::Sounds::launch2->play(_pos);
		}
	}

	void update();
	void updateCameraOnly();
	void draw() const;
};

#endif // _SPINNER_HH
