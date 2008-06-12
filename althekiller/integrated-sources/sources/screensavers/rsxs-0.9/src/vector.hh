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
#ifndef _VECTOR_HH
#define _VECTOR_HH

#include <common.hh>

#define R2D (180.0f / M_PI)
#define D2R (M_PI / 180.0f)

template <typename T>
class VectorBase {
protected:
	T _v[3];
public:
	VectorBase(const T* v) {
		set(v[0], v[1], v[2]);
	}

	VectorBase(const T& x, const T& y, const T& z) {
		set(x, y, z);
	}

	void set(const T& x, const T& y, const T& z) {
		_v[0] = x;
		_v[1] = y;
		_v[2] = z;
	}

	const T& x() const { return _v[0]; }
	T& x() { return _v[0]; }
	const T& y() const { return _v[1]; }
	T& y() { return _v[1]; }
	const T& z() const { return _v[2]; }
	T& z() { return _v[2]; }

	const T* get() const {
		return _v;
	}

	T lengthSquared() const {
		return _v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2];
	}
};

class Vector : public VectorBase<float> {
public:
	Vector(const float* v) : VectorBase<float>(v) {}

	Vector(float x = 0.0f, float y = 0.0f, float z = 0.0f)
		: VectorBase<float>(x, y, z) {}

	float length() const {
		return std::sqrt(lengthSquared());
	}

	float normalize() {
		float l = length();
		if (l == 0.0f)
			return 0.0f;
		_v[0] /= l;
		_v[1] /= l;
		_v[2] /= l;
		return l;
	}

	Vector operator+(const Vector& v) const {
		return Vector(_v[0] + v._v[0], _v[1] + v._v[1], _v[2] + v._v[2]);
	}

	Vector operator-(const Vector& v) const {
		return Vector(_v[0] - v._v[0], _v[1] - v._v[1], _v[2] - v._v[2]);
	}

	Vector operator*(float f) const {
		return Vector(_v[0] * f, _v[1] * f, _v[2] * f);
	}

	Vector operator/(float f) const {
		return Vector(_v[0] / f, _v[1] / f, _v[2] / f);
	}

	Vector& operator+=(const Vector& v) {
		_v[0] += v._v[0];
		_v[1] += v._v[1];
		_v[2] += v._v[2];
		return *this;
	}

	Vector& operator-=(const Vector& v) {
		_v[0] -= v._v[0];
		_v[1] -= v._v[1];
		_v[2] -= v._v[2];
		return *this;
	}

	Vector& operator*=(float f) {
		_v[0] *= f;
		_v[1] *= f;
		_v[2] *= f;
		return *this;
	}

	Vector& operator/=(float f) {
		_v[0] /= f;
		_v[1] /= f;
		_v[2] /= f;
		return *this;
	}

	static float dot(const Vector& a, const Vector& b) {
		return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
	}

	static Vector cross(const Vector& a, const Vector& b) {
		return Vector(
			a.y() * b.z() - b.y() * a.z(),
			a.z() * b.x() - b.z() * a.x(),
			a.x() * b.y() - b.x() * a.y()
		);
	};
};

class UVector : public VectorBase<unsigned int> {
public:
	UVector(const unsigned int* v) : VectorBase<unsigned int>(v) {}

	UVector(unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
		: VectorBase<unsigned int>(x, y, z) {}

	UVector operator+(const UVector& v) const {
		return UVector(_v[0] + v._v[0], _v[1] + v._v[1], _v[2] + v._v[2]);
	}

	UVector operator-(const UVector& v) const {
		return UVector(_v[0] - v._v[0], _v[1] - v._v[1], _v[2] - v._v[2]);
	}

	UVector& operator+=(const UVector& v) {
		_v[0] += v._v[0];
		_v[1] += v._v[1];
		_v[2] += v._v[2];
		return *this;
	}

	UVector& operator-=(const UVector& v) {
		_v[0] -= v._v[0];
		_v[1] -= v._v[1];
		_v[2] -= v._v[2];
		return *this;
	}
};

class UnitVector : public Vector {
protected:
	using Vector::set;
	using Vector::length;
	using Vector::lengthSquared;
	using Vector::normalize;

	UnitVector(const float* v) {
		set(v[0], v[1], v[2]);
	}

	UnitVector(float x, float y, float z) {
		set(x, y, z);
	}

	friend class Vector;
	friend class RotationMatrix;
	friend class UnitQuat;
public:
	UnitVector(const Vector& v) {
		float l = v.length();
		if (l == 0.0f)
			set(1.0f, 0.0f, 0.0f);
		else
			set(v.x() / l, v.y() / l, v.z() / l);
	}

	UnitVector() {
		set(1.0f, 0.0f, 0.0f);
	}

	static UnitVector of(const Vector& v) {
		float l = v.length();
		if (l == 0.0f)
			return UnitVector();
		return UnitVector(
			v.x() / l,
			v.y() / l,
			v.z() / l
		);
	}
};

class Matrix {
protected:
	float _m[16];
public:
	Matrix(
		float m0 = 1.0f, float m1 = 0.0f, float m2 = 0.0f, float m3 = 0.0f,
		float m4 = 0.0f, float m5 = 1.0f, float m6 = 0.0f, float m7 = 0.0f,
		float m8 = 0.0f, float m9 = 0.0f, float m10 = 1.0f, float m11 = 0.0f,
		float m12 = 0.0f, float m13 = 0.0f, float m14 = 0.0f, float m15 = 1.0f
	) {
		_m[0] = m0; _m[1] = m1; _m[2] = m2; _m[3] = m3;
		_m[4] = m4; _m[5] = m5; _m[6] = m6; _m[7] = m7;
		_m[8] = m8; _m[9] = m9; _m[10] = m10; _m[11] = m11;
		_m[12] = m12; _m[13] = m13; _m[14] = m14; _m[15] = m15;
	}

	float get(int i) const {
		return _m[i];
	}

	const float* get() const {
		return _m;
	}

	Matrix transposed() const {
		return Matrix(
			_m[0], _m[4], _m[8], _m[12],
			_m[1], _m[5], _m[9], _m[13],
			_m[2], _m[6], _m[10], _m[14],
			_m[3], _m[7], _m[11], _m[15]
		);
	}

	Vector transform(const Vector& v) const {
		float w = _m[3] * v.x() + _m[7] * v.y() + _m[11] * v.z() + _m[15];
		return Vector(
			(_m[0] * v.x() + _m[4] * v.y() + _m[8] * v.z() + _m[12]) / w,
			(_m[1] * v.x() + _m[5] * v.y() + _m[9] * v.z() + _m[13]) / w,
			(_m[2] * v.x() + _m[6] * v.y() + _m[10] * v.z() + _m[14]) / w
		);
	}

	Matrix operator*(const Matrix& m) const {
		return Matrix(
			_m[0] * m._m[0] + _m[1] * m._m[4] +
				_m[2] * m._m[8] + _m[3] * m._m[12],
			_m[0] * m._m[1] + _m[1] * m._m[5] +
				_m[2] * m._m[9] + _m[3] * m._m[13],
			_m[0] * m._m[2] + _m[1] * m._m[6] +
				_m[2] * m._m[10] + _m[3] * m._m[14],
			_m[0] * m._m[3] + _m[1] * m._m[7] +
				_m[2] * m._m[11] + _m[3] * m._m[15]
		,
			_m[4] * m._m[0] + _m[5] * m._m[4] +
				_m[6] * m._m[8] + _m[7] * m._m[12],
			_m[4] * m._m[1] + _m[5] * m._m[5] +
				_m[6] * m._m[9] + _m[7] * m._m[13],
			_m[4] * m._m[2] + _m[5] * m._m[6] +
				_m[6] * m._m[10] + _m[7] * m._m[14],
			_m[4] * m._m[3] + _m[5] * m._m[7] +
				_m[6] * m._m[11] + _m[7] * m._m[15]
		,
			_m[8] * m._m[0] + _m[9] * m._m[4] +
				_m[10] * m._m[8] + _m[11] * m._m[12],
			_m[8] * m._m[1] + _m[9] * m._m[5] +
				_m[10] * m._m[9] + _m[11] * m._m[13],
			_m[8] * m._m[2] + _m[9] * m._m[6] +
				_m[10] * m._m[10] + _m[11] * m._m[14],
			_m[8] * m._m[3] + _m[9] * m._m[7] +
				_m[10] * m._m[11] + _m[11] * m._m[15]
		,
			_m[12] * m._m[0] + _m[13] * m._m[4] +
				_m[14] * m._m[8] + _m[15] * m._m[12],
			_m[12] * m._m[1] + _m[13] * m._m[5] +
				_m[14] * m._m[9] + _m[15] * m._m[13],
			_m[12] * m._m[2] + _m[13] * m._m[6] +
				_m[14] * m._m[10] + _m[15] * m._m[14],
			_m[12] * m._m[3] + _m[13] * m._m[7] +
				_m[14] * m._m[11] + _m[15] * m._m[15]
		);
	}
};

class TranslationMatrix : public Matrix {
protected:
	float _dx, _dy, _dz;
public:
	TranslationMatrix(float dx = 0.0f, float dy = 0.0f, float dz = 0.0f) :
		Matrix(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			dz, dy, dz, 1.0f
		), _dx(dx), _dy(dy), _dz(dz) {}

	TranslationMatrix inverted() const {
		return TranslationMatrix(-_dx, -_dy, -_dz);
	}

	Vector transform(const Vector& v) const {
		return v + Vector(_dx, _dy, _dz);
	}

	TranslationMatrix operator*(const TranslationMatrix& m) const {
		return TranslationMatrix(_dx + m._dx, _dy + m._dy, _dz + m._dz);
	}
};

class ScalingMatrix : public Matrix {
protected:
	float _sx, _sy, _sz;
public:
	ScalingMatrix(float sx = 0.0f, float sy = 0.0f, float sz = 0.0f) :
		Matrix(
			sx, 0.0f, 0.0f, 0.0f,
			0.0f, sy, 0.0f, 0.0f,
			0.0f, 0.0f, sz, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		), _sx(sx), _sy(sy), _sz(sz) {}

	ScalingMatrix inverted() const {
		return ScalingMatrix(-_sx, -_sy, -_sz);
	}

	ScalingMatrix transposed() const {
		return ScalingMatrix(*this);
	}

	Vector transform(const Vector& v) const {
		return Vector(_sx * v.x(), _sy * v.y(), _sz * v.z());
	}

	ScalingMatrix operator*(const ScalingMatrix& m) const {
		return ScalingMatrix(_sx + m._sx, _sy + m._sy, _sz + m._sz);
	}
};

class ShearingMatrix : public Matrix {
protected:
	float _sxy, _sxz, _syx, _syz, _szx, _szy;
public:
	ShearingMatrix(
		float sxy = 0.0f, float sxz = 0.0f,
		float syx = 0.0f, float syz = 0.0f,
		float szx = 0.0f, float szy = 0.0f
	) :
		Matrix(
			1.0f, sxy, sxz, 0.0f,
			syx, 1.0f, syz, 0.0f,
			szx, szy, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		),
		_sxy(sxy), _sxz(sxz), _syx(syx),
		_syz(syz), _szx(szx), _szy(szy) {}

	ShearingMatrix transposed() const {
		return ShearingMatrix(_syx, _szx, _sxy, _szy, _sxz, _syz);
	}

	Vector transform(const Vector& v) const {
		return Vector(
			v.x() + _syx * v.y() + _szx * v.z(),
			v.y() + _sxy * v.x() + _szy * v.z(),
			v.z() + _sxz * v.x() + _syz * v.y()
		);
	}
};

class RotationMatrix : public Matrix {
protected:
	RotationMatrix(
		float m0, float m1, float m2,
		float m4, float m5, float m6,
		float m8, float m9, float m10
	) : Matrix(
		m0, m1, m2, 0.0f,
		m4, m5, m6, 0.0f,
		m8, m9, m10, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	) {}

	RotationMatrix(const Vector& x, const Vector& y, const Vector& z) :
		Matrix(
			x.x(), x.y(), x.z(), 0.0f,
			y.x(), y.y(), y.z(), 0.0f,
			z.x(), z.y(), z.z(), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		) {}

	friend class UnitQuat;
public:
	RotationMatrix() :
		Matrix(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		) {}

	// Emulate gluLookAt
	static RotationMatrix lookAt(
		const Vector& eye, const Vector& center, const Vector& up
	) {
		Vector f(eye - center);
		f.normalize();
		Vector x(Vector::cross(up, f));
		x.normalize();
		return RotationMatrix(x, Vector::cross(f, x), f);
	}

	static RotationMatrix heading(float a) {
		float sa = std::sin(a);
		float ca = std::cos(a);

		return RotationMatrix(
			1.0f, 0.0f, 0.0f,
			0.0f, ca, -sa,
			0.0f, sa, ca
		);
	}

	static RotationMatrix pitch(float a) {
		float sa = std::sin(a);
		float ca = std::cos(a);

		return RotationMatrix(
			ca, 0.0f, -sa,
			0.0f, 1.0f, 0.0f,
			sa, 0.0f, ca
		);
	}

	static RotationMatrix roll(float a) {
		float sa = std::sin(a);
		float ca = std::cos(a);

		return RotationMatrix(
			ca, -sa, 0.0f,
			sa, ca, 0.0f,
			0.0f, 0.0f, 1.0f
		);
	}

	RotationMatrix inverted() const {
		return RotationMatrix(
			_m[0], _m[4], _m[8],
			_m[1], _m[5], _m[9],
			_m[2], _m[6], _m[10]
		);
	}

	RotationMatrix transposed() const {
		return RotationMatrix(
			_m[0], _m[4], _m[8],
			_m[1], _m[5], _m[9],
			_m[2], _m[6], _m[10]
		);
	}

	Vector transform(const Vector& v) const {
		return Vector(
			_m[0] * v.x() + _m[4] * v.y() + _m[8] * v.z(),
			_m[1] * v.x() + _m[5] * v.y() + _m[9] * v.z(),
			_m[2] * v.x() + _m[6] * v.y() + _m[10] * v.z()
		);
	}

	UnitVector transform(const UnitVector& v) const {
		return UnitVector(
			_m[0] * v.x() + _m[4] * v.y() + _m[8] * v.z(),
			_m[1] * v.x() + _m[5] * v.y() + _m[9] * v.z(),
			_m[2] * v.x() + _m[6] * v.y() + _m[10] * v.z()
		);
	}

	RotationMatrix operator*(const RotationMatrix& m) const {
		return RotationMatrix(
			_m[0] * m._m[0] + _m[1] * m._m[4] + _m[2] * m._m[8],
			_m[0] * m._m[1] + _m[1] * m._m[5] + _m[2] * m._m[9],
			_m[0] * m._m[2] + _m[1] * m._m[6] + _m[2] * m._m[10]
		,
			_m[4] * m._m[0] + _m[5] * m._m[4] + _m[6] * m._m[8],
			_m[4] * m._m[1] + _m[5] * m._m[5] + _m[6] * m._m[9],
			_m[4] * m._m[2] + _m[5] * m._m[6] + _m[6] * m._m[10]
		,
			_m[8] * m._m[0] + _m[9] * m._m[4] + _m[10] * m._m[8],
			_m[8] * m._m[1] + _m[9] * m._m[5] + _m[10] * m._m[9],
			_m[8] * m._m[2] + _m[9] * m._m[6] + _m[10] * m._m[10]
		);
	}
};

class UnitQuat {
protected:
	float _q[4];
	unsigned int _renormalizeCount;

	// These two are internal, since they do not guarantee length == 1
	UnitQuat(float x, float y, float z, float w) : _renormalizeCount(0) {
		_q[0] = x;
		_q[1] = y;
		_q[2] = z;
		_q[3] = w;
	}

	void set(float x, float y, float z, float w) {
		_q[0] = x;
		_q[1] = y;
		_q[2] = z;
		_q[3] = w;
	}
public:
	UnitQuat() : _renormalizeCount(0) {
		_q[0] = _q[1] = _q[2] = 0.0f;
		_q[3] = 1.0f;
	}

	UnitQuat(float a, const UnitVector& axis) : _renormalizeCount(0) {
		a *= 0.5f;
		float sinA = std::sin(a);
		set(
			sinA * axis.x(),
			sinA * axis.y(),
			sinA * axis.z(),
			std::cos(a)
		);
	}

	UnitQuat(const RotationMatrix& r) : _renormalizeCount(0) {
		float trace = r.get(0) + r.get(5) + r.get(10) + 1.0f;
		if (trace > 0) {
			float s = 2.0f * std::sqrt(trace);
			set(
				(r.get(9) - r.get(6)) / s,
				(r.get(2) - r.get(8)) / s,
				(r.get(4) - r.get(1)) / s,
				s / 4.0f
			);
		} else {
			if (r.get(0) > r.get(5)) {
				if (r.get(0) > r.get(10)) {
					// r.get(0) is largest
					float s = 2.0f *
						std::sqrt(1.0f + r.get(0) - r.get(5) - r.get(10));
					set(
						s / 4.0f,
						(r.get(1) + r.get(4)) / s,
						(r.get(2) + r.get(8)) / s,
						(r.get(9) - r.get(6)) / s
					);
				} else {
					// r.get(10) is largest
					float s = 2.0f *
						std::sqrt(1.0f + r.get(10) - r.get(0) - r.get(5));
					set(
						(r.get(2) + r.get(8)) / s,
						(r.get(6) + r.get(9)) / s,
						s / 4,
						(r.get(4) - r.get(1)) / s
					);
				}
			} else {
				if (r.get(5) > r.get(10)) {
					// r.get(5) is largest
					float s = 2.0f *
						std::sqrt(1.0f + r.get(5) - r.get(0) - r.get(10));
					set(
						(r.get(1) + r.get(4)) / s,
						s / 4,
						(r.get(6) + r.get(9)) / s,
						(r.get(2) - r.get(8)) / s
					);
				} else {
					// r.get(10) is largest
					float s = 2.0f *
						std::sqrt(1.0f + r.get(10) - r.get(0) - r.get(5));
					set(
						(r.get(2) + r.get(8)) / s,
						(r.get(6) + r.get(9)) / s,
						s / 4,
						(r.get(4) - r.get(1)) / s
					);
				}
			}
		}
	}

	operator RotationMatrix() const {
		float wx = _q[3] * _q[0] * 2.0f;
		float wy = _q[3] * _q[1] * 2.0f;
		float wz = _q[3] * _q[2] * 2.0f;
		float xx = _q[0] * _q[0] * 2.0f;
		float xy = _q[0] * _q[1] * 2.0f;
		float xz = _q[0] * _q[2] * 2.0f;
		float yy = _q[1] * _q[1] * 2.0f;
		float yz = _q[1] * _q[2] * 2.0f;
		float zz = _q[2] * _q[2] * 2.0f;

		return RotationMatrix(
			1.0f - yy - zz, xy + wz, xz - wy,
			xy - wz, 1.0f - xx - zz, yz + wx,
			xz + wy, yz - wx, 1.0f - xx - yy
		);
	}

	void toAngleAxis(float* a, UnitVector* axis) const {
		*a = 2.0f * std::acos(_q[3]);
		float sinA = std::sqrt(1.0f - _q[3] * _q[3]);
		if (std::abs(sinA) < 0.00001f)
			axis->set(1.0f, 0.0f, 0.0f);
		else
			axis->set(
				_q[0] / sinA,
				_q[1] / sinA,
				_q[2] / sinA
			);
	}

	UnitVector forward() const {
		float wx = _q[3] * _q[0] * 2.0f;
		float wy = _q[3] * _q[1] * 2.0f;
		float xx = _q[0] * _q[0] * 2.0f;
		float xz = _q[0] * _q[2] * 2.0f;
		float yy = _q[1] * _q[1] * 2.0f;
		float yz = _q[1] * _q[2] * 2.0f;

		return UnitVector(-xz - wy, -yz + wx, -1.0f + xx + yy);
	}

	UnitVector backward() const {
		float wx = _q[3] * _q[0] * 2.0f;
		float wy = _q[3] * _q[1] * 2.0f;
		float xx = _q[0] * _q[0] * 2.0f;
		float xz = _q[0] * _q[2] * 2.0f;
		float yy = _q[1] * _q[1] * 2.0f;
		float yz = _q[1] * _q[2] * 2.0f;

		return UnitVector(xz + wy, yz - wx, 1.0f - xx - yy);
	}

	UnitVector left() const {
		float wy = _q[3] * _q[1] * 2.0f;
		float wz = _q[3] * _q[2] * 2.0f;
		float xy = _q[0] * _q[1] * 2.0f;
		float xz = _q[0] * _q[2] * 2.0f;
		float yy = _q[1] * _q[1] * 2.0f;
		float zz = _q[2] * _q[2] * 2.0f;

		return UnitVector(-1.0f + yy + zz, -xy - wz, -xz + wy);
	}

	UnitVector right() const {
		float wy = _q[3] * _q[1] * 2.0f;
		float wz = _q[3] * _q[2] * 2.0f;
		float xy = _q[0] * _q[1] * 2.0f;
		float xz = _q[0] * _q[2] * 2.0f;
		float yy = _q[1] * _q[1] * 2.0f;
		float zz = _q[2] * _q[2] * 2.0f;

		return UnitVector(1.0f - yy - zz, xy + wz, xz - wy);
	}

	UnitVector up() const {
		float wx = _q[3] * _q[0] * 2.0f;
		float wz = _q[3] * _q[2] * 2.0f;
		float xx = _q[0] * _q[0] * 2.0f;
		float xy = _q[0] * _q[1] * 2.0f;
		float yz = _q[1] * _q[2] * 2.0f;
		float zz = _q[2] * _q[2] * 2.0f;

		return UnitVector(xy - wz, 1.0f - xx - zz, yz + wx);
	}

	UnitVector down() const {
		float wx = _q[3] * _q[0] * 2.0f;
		float wz = _q[3] * _q[2] * 2.0f;
		float xx = _q[0] * _q[0] * 2.0f;
		float xy = _q[0] * _q[1] * 2.0f;
		float yz = _q[1] * _q[2] * 2.0f;
		float zz = _q[2] * _q[2] * 2.0f;

		return UnitVector(-xy + wz, -1.0f + xx + zz, -yz + wx);
	}

	float getX() const {
		return _q[0];
	}

	float getY() const {
		return _q[1];
	}

	float getZ() const {
		return _q[2];
	}

	float getW() const {
		return _q[3];
	}

	void multiplyBy(const UnitQuat& q) {
		float tempX = _q[0];
		float tempY = _q[1];
		float tempZ = _q[2];
		float tempW = _q[3];

		_q[0] = tempW * q._q[0] + q._q[3] * tempX
			+ tempY * q._q[2] - q._q[1] * tempZ;
		_q[1] = tempW * q._q[1] + q._q[3] * tempY
			+ tempZ * q._q[0] - q._q[2] * tempX;
		_q[2] = tempW * q._q[2] + q._q[3] * tempZ
			+ tempX * q._q[1] - q._q[0] * tempY;
		_q[3] = tempW * q._q[3]
			- tempX * q._q[0]
			- tempY * q._q[1]
			- tempZ * q._q[2];

		if (++_renormalizeCount == 5) {
			float length = std::sqrt(
				_q[0] * _q[0] +
				_q[1] * _q[1] +
				_q[2] * _q[2] +
				_q[3] * _q[3]
			);
			_q[0] /= length;
			_q[1] /= length;
			_q[2] /= length;
			_q[3] /= length;
			_renormalizeCount = 0;
		}
	}

	void preMultiplyBy(const UnitQuat& q) {
		float tempX = _q[0];
		float tempY = _q[1];
		float tempZ = _q[2];
		float tempW = _q[3];

		_q[0] = q._q[3] * tempX + tempW * q._q[0]
			+ q._q[1] * tempZ - tempY * q._q[2];
		_q[1] = q._q[3] * tempY + tempW * q._q[1]
			+ q._q[2] * tempX - tempZ * q._q[0];
		_q[2] = q._q[3] * tempZ + tempW * q._q[2]
			+ q._q[0] * tempY - tempX * q._q[1];
		_q[3] = q._q[3] * tempW
			- q._q[0] * tempX
			- q._q[1] * tempY
			- q._q[2] * tempZ;

		if (++_renormalizeCount == 5) {
			float length = std::sqrt(
				_q[0] * _q[0] +
				_q[1] * _q[1] +
				_q[2] * _q[2] +
				_q[3] * _q[3]
			);
			_q[0] /= length;
			_q[1] /= length;
			_q[2] /= length;
			_q[3] /= length;
			_renormalizeCount = 0;
		}
	}

	static UnitQuat pitch(float a) {
		a *= 0.5f;
		return UnitQuat(std::sin(a), 0.0f, 0.0f, std::cos(a));
	}

	static UnitQuat heading(float a) {
		a *= 0.5f;
		return UnitQuat(0.0f, std::sin(a), 0.0f, std::cos(a));
	}

	static UnitQuat roll(float a) {
		a *= 0.5f;
		return UnitQuat(0.0f, 0.0f, std::sin(a), std::cos(a));
	}
};

#endif // _VECTOR_HH
