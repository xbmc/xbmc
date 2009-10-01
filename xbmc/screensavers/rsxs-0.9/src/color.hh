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
#ifndef _COLOR_HH
#define _COLOR_HH

#include <common.hh>

#include <vector.hh>

class HSLColor;

class RGBColor : protected Vector {
public:
	RGBColor(const float*& v) : Vector(v) {}
	RGBColor(
		float r = 0.0f, float g = 0.0f, float b = 0.0f
	) : Vector(r, g, b) {}

	operator HSLColor() const;

	using Vector::set;
	using Vector::get;

	RGBColor operator+(const RGBColor& v) const {
		return RGBColor(
			_v[0] + v._v[0], _v[1] + v._v[1], _v[2] + v._v[2]
		);
	}

	RGBColor operator-(const RGBColor& v) const {
		return RGBColor(
			_v[0] - v._v[0], _v[1] - v._v[1], _v[2] - v._v[2]
		);
	}

	RGBColor operator*(float f) const {
		return RGBColor(_v[0] * f, _v[1] * f, _v[2] * f);
	}

	RGBColor operator/(float f) const {
		return RGBColor(_v[0] / f, _v[1] / f, _v[2] / f);
	}

	RGBColor& operator+=(const RGBColor& v) {
		_v[0] += v._v[0];
		_v[1] += v._v[1];
		_v[2] += v._v[2];
		return *this;
	}

	RGBColor& operator*=(float f) {
		_v[0] *= f;
		_v[1] *= f;
		_v[2] *= f;
		return *this;
	}

	RGBColor& operator/=(float f) {
		_v[0] /= f;
		_v[1] /= f;
		_v[2] /= f;
		return *this;
	}

	const float& r() const { return x(); }
	float& r() { return x(); }
	const float& g() const { return y(); }
	float& g() { return y(); }
	const float& b() const { return z(); }
	float& b() { return z(); }

	void clamp() {
		if (_v[0] > 1)
			_v[0] = 1;
		if (_v[0] < 0)
			_v[0] = 0;
		if (_v[1] > 1)
			_v[1] = 1;
		if (_v[1] < 0)
			_v[1] = 0;
		if (_v[2] > 1)
			_v[2] = 1;
		if (_v[2] < 0)
			_v[2] = 0;
	}

	static RGBColor tween(
		const RGBColor&, const RGBColor&, float tween, bool direction
	);
};

class HSLColor : protected Vector {
public:
	HSLColor(const float*& v) : Vector(v) {}
	HSLColor(
		float h = 0.0f, float s = 0.0f, float l = 0.0f
	) : Vector(h, s, l) {}

	operator RGBColor() const;

	using Vector::set;
	using Vector::get;
	using Vector::operator+;
	using Vector::operator-;
	using Vector::operator*;
	using Vector::operator/;
	using Vector::operator+=;
	using Vector::operator*=;
	using Vector::operator/=;

	const float& h() const { return x(); }
	float& h() { return x(); }
	const float& s() const { return y(); }
	float& s() { return y(); }
	const float& l() const { return z(); }
	float& l() { return z(); }

	void clamp() {
		while (_v[0] >= 1)
			_v[0] -= 1;
		while (_v[0] < 0)
			_v[0] += 1;
		if (_v[1] > 1)
			_v[1] = 1;
		if (_v[1] < 0)
			_v[1] = 0;
		if (_v[2] > 1)
			_v[2] = 1;
		if (_v[2] < 0)
			_v[2] = 0;
	}

	static HSLColor tween(
		const HSLColor&, const HSLColor&, float tween, bool direction
	);
};

#endif // _COLOR_HH
