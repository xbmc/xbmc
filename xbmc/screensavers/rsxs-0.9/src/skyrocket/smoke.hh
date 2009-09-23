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
#ifndef _SMOKE_HH
#define _SMOKE_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <vector.hh>

class Smoke : public Particle {
private:
	RGBColor _RGB;
	RGBColor _addedRGB;
	float _brightness;
	float _size;
	unsigned int _list;
	bool _frameToggle;

	static float _times[8];
	static unsigned int _timeIndex;
public:
	static void init() {
		for (unsigned int i = 0; i < 7; ++i)
			if (_times[i] > Hack::smoke)
				_times[i] = Hack::smoke;
		_times[7] = Hack::smoke;
	}

	Smoke(const Vector& pos, const Vector& vel) :
		Particle(pos, vel, 2.0f, _times[_timeIndex]), _RGB(
			0.01f * Hack::ambient,
			0.01f * Hack::ambient,
			0.01f * Hack::ambient
		), _addedRGB(0.0f, 0.0f, 0.0f), _size(0.1f),
		_list(Resources::DisplayLists::smokes + Common::randomInt(5)),
		_frameToggle(false)
	{
		_timeIndex = (_timeIndex + 1) % 8;
	}

	void update();
	void updateCameraOnly();
	void draw() const;

	void illuminate(const Vector&, const RGBColor&, float, float);
};

#endif // _SMOKE_HH
