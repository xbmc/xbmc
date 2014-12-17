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
#include <common.hh>

#include <spline.hh>
#include <vector.hh>

namespace Spline {
	unsigned int points;
	float step;

	std::vector<float> _phase;
	std::vector<float> _rate;
	std::vector<Vector> _moveXYZ;
	std::vector<Vector> _baseXYZ;
	std::vector<Vector> _XYZ;
	std::vector<Vector> _baseDir;
	std::vector<Vector> _dir;

	Vector interpolate(const Vector&, const Vector&, const Vector&, const Vector&, float);
};

void Spline::init(unsigned int length) {
	// 6 is the minimum number of points necessary for a tunnel to have one segment
	points = (length < 6 ? 6 : length);
	step = 0.0f;

	stdx::construct_n(_phase, points);
	stdx::construct_n(_rate, points);
	stdx::construct_n(_moveXYZ, points);
	stdx::construct_n(_baseXYZ, points);
	stdx::construct_n(_XYZ, points);
	stdx::construct_n(_baseDir, points);
	stdx::construct_n(_dir, points);

	_baseXYZ[points - 2].z() = 4.0f;

	for (unsigned int i = 0; i < points; ++i)
		makeNewPoint();
}

void Spline::makeNewPoint() {
	// shift points to rear of path
	std::rotate(_baseXYZ.begin(), _baseXYZ.begin() + 1, _baseXYZ.end());
	std::rotate(_moveXYZ.begin(), _moveXYZ.begin() + 1, _moveXYZ.end());
	std::rotate(_XYZ.begin(), _XYZ.begin() + 1, _XYZ.end());
	std::rotate(_phase.begin(), _phase.begin() + 1, _phase.end());
	std::rotate(_rate.begin(), _rate.begin() + 1, _rate.end());

	// make vector to new point
	int lastPoint = points - 1;
	float tempX = _baseXYZ[lastPoint - 1].x() - _baseXYZ[lastPoint - 2].x();
	float tempZ = _baseXYZ[lastPoint - 1].z() - _baseXYZ[lastPoint - 2].z();

	// find good angle
	float turnAngle;
	float pathAngle = std::atan2(tempX, tempZ);
	float distSquared =
		_baseXYZ[lastPoint].x() * _baseXYZ[lastPoint].x() +
		_baseXYZ[lastPoint].z() * _baseXYZ[lastPoint].z();
	if (distSquared > 10000.0f) {
		float angleToCenter = std::atan2(-_baseXYZ[lastPoint].x(), -_baseXYZ[lastPoint].z());
		turnAngle = angleToCenter - pathAngle;
		if (turnAngle > M_PI)
			turnAngle -= M_PI * 2.0f;
		if (turnAngle < -M_PI)
			turnAngle += M_PI * 2.0f;
		turnAngle = Common::clamp(turnAngle, -0.7f, 0.7f);
	} else
		turnAngle = Common::randomFloat(1.4f) - 0.7f;

	// rotate new point to some new position
	float ca = std::cos(turnAngle);
	float sa = std::cos(turnAngle);
	_baseXYZ[lastPoint].set(tempX * ca + tempZ * sa, 0.0f, tempX * -sa + tempZ * ca);

	// normalize and set length of vector
	// make it at least length 2, which is the grid size of the goo
	float lengthener = (Common::randomFloat(6.0f) + 2.0f) / _baseXYZ[lastPoint].length();
	_baseXYZ[lastPoint] *= lengthener;

	// make new movement vector proportional to base vector
	_moveXYZ[lastPoint].set(
		Common::randomFloat(0.25f) * -_baseXYZ[lastPoint].z(),
		0.3f,
		Common::randomFloat(0.25f) * -_baseXYZ[lastPoint].x()
	);

	// add vector to previous point to get new point
	_baseXYZ[lastPoint] += Vector(
		_baseXYZ[lastPoint - 1].x(),
		0.0f,
		_baseXYZ[lastPoint - 1].z()
	);

	// make new phase and movement rate
	_phase[lastPoint] = Common::randomFloat(M_PI * 2);
	_rate[lastPoint]  = Common::randomFloat(1.0f);

	// reset base direction vectors
	_baseDir.front() = _baseXYZ[1] - _baseXYZ[points - 1];
	std::transform(
		_baseXYZ.begin() + 2, _baseXYZ.end(),
		_baseXYZ.begin(), _baseDir.begin() + 1,
		std::minus<Vector>()
	);
	_baseDir.back() = _baseXYZ[0] - _baseXYZ[points - 2];
}

Vector Spline::at(unsigned int section, float where) {
	section = Common::clamp(section, 1u, points - 3);

	return interpolate(_XYZ[section - 1], _XYZ[section], _XYZ[section + 1], _XYZ[section + 2], where);
}

Vector Spline::direction(unsigned int section, float where) {
	section = Common::clamp(section, 1u, points - 3);

	Vector direction(interpolate(_dir[section - 1], _dir[section], _dir[section + 1], _dir[section + 2], where));
	direction.normalize();
	return direction;
}

// Here's a little calculus that takes 4 points
// and interpolates smoothly between the second and third
// depending on the value of where which can be 0.0 to 1.0.
// The slope at b is estimated using a and c.  The slope at c
// is estimated using b and d.
Vector Spline::interpolate(const Vector& a, const Vector& b, const Vector& c, const Vector& d, float where) {
	return
		(((b * 3.0f) + d - a - (c * 3.0f)) * (where * where * where)) * 0.5f +
		(((a * 2.0f) - (b * 5.0f) + (c * 4.0f) - d) * (where * where)) * 0.5f +
		((c - a) * where) * 0.5f +
		b;
}
