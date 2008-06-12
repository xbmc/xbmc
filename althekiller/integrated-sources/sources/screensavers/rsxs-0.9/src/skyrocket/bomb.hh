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
#ifndef _BOMB_HH
#define _BOMB_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <vector.hh>

class Bomb : public Particle {
public:
	enum Type {
		BOMB_STARS,
		BOMB_STREAMERS,
		BOMB_METEORS,
		BOMB_CRACKER
	};
private:
	RGBColor _RGB;

	Type _bombType;
public:
	Bomb(const Vector& pos, const Vector& vel, Type bombType,
		const RGBColor& RGB = RGBColor()) :
			Particle(pos, vel, 0.4f,
				(bombType == BOMB_CRACKER) ?
					4.0f * (0.5f - std::sin(Common::randomFloat(M_PI))) + 4.5f :
					Common::randomFloat(1.5f) + 3.0f
			), _RGB(RGB), _bombType(bombType) {}

	void update();
	void updateCameraOnly();
	void draw() const;
};

#endif // _BOMB_HH
