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
#ifndef _CAMERA_HH
#define _CAMERA_HH

#include <common.hh>

#include <vector.hh>

namespace Camera {
	float _farPlane;
	Vector _cullVec[4];	// vectors perpendicular to viewing volume planes

	void set(const float* m, float farPlane) {
		_farPlane = farPlane;

		float temp;

		// bottom and planes' vectors
		temp = std::atan(1.0f / m[5]);
		_cullVec[0].set(0.0f, std::cos(temp), -std::sin(temp));
		_cullVec[1].set(0.0f, -std::cos(temp), -std::sin(temp));

		// left and right planes' vectors
		temp = std::atan(1.0f / m[0]);
		_cullVec[2].set(std::cos(temp), 0.0f, -std::sin(temp));
		_cullVec[3].set(-std::cos(temp), 0.0f,-std::sin(temp));
	}

	bool isVisible(const Vector& pos, float radius) {
		return
			pos.z() >= -(_farPlane + radius) &&
			Vector::dot(pos, _cullVec[0]) >= -radius &&
			Vector::dot(pos, _cullVec[1]) >= -radius &&
			Vector::dot(pos, _cullVec[2]) >= -radius &&
			Vector::dot(pos, _cullVec[3]) >= -radius;
	}
};

#endif // _CAMERA_HH
