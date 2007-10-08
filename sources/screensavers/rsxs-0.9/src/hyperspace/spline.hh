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
#ifndef _SPLINE_HH
#define _SPLINE_HH

#include <common.hh>

#include <vector.hh>

namespace Spline {
	extern unsigned int points;
	extern float step;

	void init(unsigned int);

	Vector at(unsigned int, float);
	Vector direction(unsigned int, float);

	void makeNewPoint();
	inline void moveAlongPath(float increment) {
		step += increment;
		while (step >= 1.0f) {
			step -= 1.0f;
			makeNewPoint();
		}
	}

	extern std::vector<float> _phase;
	extern std::vector<float> _rate;
	extern std::vector<Vector> _moveXYZ;
	extern std::vector<Vector> _baseXYZ;
	extern std::vector<Vector> _XYZ;
	extern std::vector<Vector> _baseDir;
	extern std::vector<Vector> _dir;

	inline void update(float multiplier) {
		// calculate xyz positions and direction vectors
		_phase[0] += _rate[0] * multiplier;
		_XYZ[0] = _baseXYZ[0] + _moveXYZ[0] * std::cos(_phase[0]);
		_phase[1] += _rate[1] * multiplier;
		_XYZ[1] = _baseXYZ[1] + _moveXYZ[1] * std::cos(_phase[1]);
		for (unsigned int i = 2; i < points; ++i) {
			_phase[i] += _rate[i] * multiplier;
			_XYZ[i] = _baseXYZ[i] + _moveXYZ[i] * std::cos(_phase[i]);
			_dir[i - 1] = _XYZ[i] - _XYZ[i - 2];
		}
		_dir[points - 1] = _XYZ[0] - _XYZ[points - 2];
		_dir[0] = _XYZ[1] - _XYZ[points - 1];
	}
};

#endif  // _SPLINE_HH
