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
 * Copyright (C) 2005 Terence M. Welsh, available from www.reallyslick.com
 */
#ifndef _PARTICLE_H
#define _PARTICLE_H

#include <common.hh>

#include <color.hh>
#include <vector.hh>

class StretchedParticle : public ResourceManager::Resource<void> {
private:
	Vector _XYZ, _lastXYZ;
	float _radius;
	RGBColor _color;
	float _fov;

	double _lastScreenPos[2];
	bool _moved;
public:
	StretchedParticle(const Vector&, float, const RGBColor&, float);
	void operator()() const {}

	void update();
	void draw();

	const Vector& getPos() const { return _XYZ; }
	void setPos(const Vector& XYZ) {
		_XYZ = _lastXYZ = XYZ; _moved = true; 
	}
	void offsetPos(const Vector& XYZ) { _XYZ += XYZ; }

	void setColor(const RGBColor& RGB) { _color = RGB; }
};

#endif // _PARTICLE_H
