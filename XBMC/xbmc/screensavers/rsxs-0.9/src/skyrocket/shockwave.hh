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
#ifndef _SHOCKWAVE_HH
#define _SHOCKWAVE_HH

#include <common.hh>

#include <particle.hh>
#include <vector.hh>

#define WAVESTEPS 40

class Shockwave : public Particle {
private:
	static Vector _geom[7][WAVESTEPS + 1];

	float _size;
	float _life;
public:
	Shockwave(const Vector&, const Vector&);

	static void init();

	void update();
	void updateCameraOnly();
	void draw() const;
	void drawShockwave(float, float) const;
	void shock(const Vector&, float);
};

#endif // _SHOCKWAVE_HH
