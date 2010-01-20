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
#ifndef _WISP_HH
#define _WISP_HH

#include <common.hh>

#include <color.hh>
#include <vector.hh>

#define NUMCONSTS 9

class Wisp {
private:
	stdx::dim2<Vector> _vertex;
	stdx::dim2<Vector> _gridPos;
	stdx::dim2<float> _intensity;

	float _c[NUMCONSTS];	// constants
	float _cr[NUMCONSTS];	// constants' radial position
	float _cv[NUMCONSTS];	// constants' change velocities
	HSLColor _HSL;
	RGBColor _RGB;
	float _hueSpeed;
	float _saturationSpeed;
public:
	Wisp();

	void update();
	void draw() const;
	void drawAsBackground() const;
};

#endif // _WISP_HH
