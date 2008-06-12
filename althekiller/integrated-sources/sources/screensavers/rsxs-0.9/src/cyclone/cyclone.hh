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
#ifndef _CYCLONE_HH
#define _CYCLONE_HH

#include <common.hh>

#include <color.hh>
#include <vector.hh>

#define MAX_COMPLEXITY 10u

namespace Hack {
	extern unsigned int numCyclones;
	extern unsigned int numParticles;
	extern float size;
	extern unsigned int complexity;
	extern float speed;
	extern bool stretch;
	extern bool showCurves;
	extern bool southern;
};

class Cyclone {
private:
	Vector _targetV[MAX_COMPLEXITY + 3];
	Vector _V[MAX_COMPLEXITY + 3];
	Vector _oldV[MAX_COMPLEXITY + 3];
	float _targetWidth[MAX_COMPLEXITY + 3];
	float _width[MAX_COMPLEXITY + 3];
	float _oldWidth[MAX_COMPLEXITY + 3];
	HSLColor _targetHSL;
	HSLColor _HSL;
	HSLColor _oldHSL;
	std::pair<int, int> _VChange[MAX_COMPLEXITY + 3];
	std::pair<int, int> _widthChange[MAX_COMPLEXITY + 3];
	std::pair<int, int> _HSLChange;
public:
	Cyclone();

	const HSLColor& color() const {
		return _HSL;
	}

	const Vector& v(unsigned int i) const {
		return _V[i];
	}

	float width(unsigned int i) const {
		return _width[i];
	}

	void update();
};

#endif // _CYCLONE_HH
