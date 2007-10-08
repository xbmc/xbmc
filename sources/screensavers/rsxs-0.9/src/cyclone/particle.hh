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
#ifndef _PARTICLE_HH
#define _PARTICLE_HH

#include <common.hh>

#include <color.hh>
#include <cyclone.hh>
#include <vector.hh>

class Particle {
private:
	static GLuint _list;

	std::vector<Cyclone>::const_iterator _cy;

	RGBColor _RGB;
	Vector _V, _lastV;
	float _width;
	float _step;
	float _spinAngle;
public:
	static void init() {
		_list = Common::resources->genLists(1);
		glNewList(_list, GL_COMPILE);
			GLUquadricObj* qobj = gluNewQuadric();
			gluSphere(qobj, Hack::size / 4.0f, 3, 2);
			gluDeleteQuadric(qobj);
		glEndList();
	}

	Particle(const std::vector<Cyclone>::const_iterator& cy) : _cy(cy) {
		setup();
	}

	void setup() {
		_width = Common::randomFloat(0.8f) + 0.2f;
		_step = 0.0f;
		_spinAngle = Common::randomFloat(360);
		_RGB = _cy->color();
	}

	void update();
};

#endif // _PARTICLE_HH
