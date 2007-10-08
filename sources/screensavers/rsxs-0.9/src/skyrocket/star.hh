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
#ifndef _STAR_HH
#define _STAR_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <resources.hh>
#include <vector.hh>

class Star : public Particle {
private:
	RGBColor _RGB;
	float _size;
	float _brightness;

	bool _smoker;
	Vector _smokePos;

	unsigned int _list;
public:
	Star(
		const Vector& pos, const Vector& vel, float drag, float lifetime,
		const RGBColor& RGB, float size, bool smoker = false,
		unsigned int list = Resources::DisplayLists::flares
	) :
		Particle(pos, vel, drag, lifetime), _RGB(RGB), _size(size),
		_smoker(smoker), _smokePos(_pos), _list(list) {}

	void update();
	void updateCameraOnly();
	void draw() const;
};

#endif // _STAR_HH
