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
#ifndef _FOUNTAIN_HH
#define _FOUNTAIN_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <vector.hh>

class Fountain : public Particle {
private:
	RGBColor _RGB;
	float _brightness;

	float _starTimer;
public:
	Fountain() : Particle(
		Vector(
			Common::randomFloat(300.0f) - 150.0f,
			5.0f,
			Common::randomFloat(300.0f) - 150.0f
		), Vector(), 0.0f, Common::randomFloat(5.0f) + 10.0f
	), _RGB(Particle::randomColor()), _brightness(0.0f), _starTimer(0.0f) {
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

#endif // _FOUNTAIN_HH
