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
#ifndef _FLUX_HH
#define _FLUX_HH

#include <common.hh>

#define NUMCONSTS 8

namespace Hack {
	enum GeometryType {
		POINTS_GEOMETRY,
		SPHERES_GEOMETRY,
		LIGHTS_GEOMETRY
	};

	extern unsigned int numFluxes;
	extern unsigned int numTrails;
	extern unsigned int trailLength;
	extern GeometryType geometry;
	extern float size;
	extern unsigned int complexity;
	extern unsigned int randomize;
	extern float expansion;
	extern float rotation;
	extern float wind;
	extern float instability;
	extern float blur;
};

class Trail;

class Flux {
private:
	std::vector<Trail> _trails;
	unsigned int _randomize;
	float _c[NUMCONSTS];	// constants
	float _cv[NUMCONSTS];	// constants' change velocities
	float _oldDistance;
public:
	Flux();

	void update(float, float);
};

#endif // _FLUX_HH
