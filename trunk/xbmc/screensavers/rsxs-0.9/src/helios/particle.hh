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
#include <helios.hh>
#include <vector.hh>

#define LIGHTSIZE 64

class Node {
private:
	Vector _pos;
	Vector _oldPos;
	Vector _targetPos;
public:
	Node() : _pos(Vector(
		Common::randomFloat(1000.0f) - 500.0f,
		Common::randomFloat(1000.0f) - 500.0f,
		Common::randomFloat(1000.0f) - 500.0f
	)), _oldPos(_pos), _targetPos(_pos) {}

	const Vector& getPos() const {
		return _pos;
	}

	void setTargetPos(const Vector& target) {
		_oldPos = _pos;
		_targetPos = target;
	}

	void update(float n) {
		_pos = _oldPos * (1.0f - n) + _targetPos * n;
	}
};

class Ion {
private:
	static GLuint _texture;
	static GLuint _list;

	Vector _pos;
	float _size;
	float _speed;
	RGBColor _RGB;
public:
	static void init();

	Ion() {
		float temp = Common::randomFloat(2.0f) + 0.4f;
		_size = Hack::size * temp;
		_speed = Hack::speed * 12.0f / temp;
	}

	const RGBColor& getRGB() const {
		return _RGB;
	}

	void start(const Vector& pos, const RGBColor& RGB) {
		_pos = pos;

		float offset = Common::elapsedSecs * _speed;
		switch (Common::randomInt(14)) {
		case 0:
			_pos += Vector(offset, 0, 0);
			break;
		case 1:
			_pos += Vector(-offset, 0, 0);
			break;
		case 2:
			_pos += Vector(0, offset, 0);
			break;
		case 3:
			_pos += Vector(0, -offset, 0);
			break;
		case 4:
			_pos += Vector(0, 0, offset);
			break;
		case 5:
			_pos += Vector(0, 0, -offset);
			break;
		case 6:
			_pos += Vector(offset, offset, offset);
			break;
		case 7:
			_pos += Vector(-offset, offset, offset);
			break;
		case 8:
			_pos += Vector(offset, -offset, offset);
			break;
		case 9:
			_pos += Vector(-offset, -offset, offset);
			break;
		case 10:
			_pos += Vector(offset, offset, -offset);
			break;
		case 11:
			_pos += Vector(-offset, offset, -offset);
			break;
		case 12:
			_pos += Vector(offset, -offset, -offset);
			break;
		case 13:
			_pos += Vector(-offset, -offset, -offset);
			break;
		}

		_RGB = RGB;
	}

	void update(
		const std::vector<Node> eList, const std::vector<Node> aList,
		const RGBColor& RGB
	) {
		Vector force(0.0f, 0.0f, 0.0f);

		bool startOver = false;
		for (
			std::vector<Node>::const_iterator i = eList.begin();
			i != eList.end();
			++i
		) {
			Vector temp(_pos - i->getPos());
			float length = temp.normalize();
			startOver = startOver || (length > 11000.0f);
			if (length > 1.0f) temp /= length;
			force += temp;
		}

		float startOverDistance = Common::elapsedSecs * _speed;
		for (
			std::vector<Node>::const_iterator i = aList.begin();
			i != aList.end();
			++i
		) {
			Vector temp(i->getPos() - _pos);
			float length = temp.normalize();
			startOver = startOver || (length < startOverDistance);
			if (length > 1.0f) temp /= length;
			force += temp;
		}

		// Start this ion at an emitter if it gets too close to an attracter
		// or too far from an emitter
		if (startOver)
			start(eList[Common::randomInt(Hack::numEmitters)].getPos(), RGB);
		else {
			force.normalize();
			_pos += force * Common::elapsedSecs * _speed;
		}
	}

	void draw(const RotationMatrix& billboardMat) const {
		glColor3fv(_RGB.get());
		glPushMatrix();
			Vector transformed(billboardMat.transform(_pos));
			glTranslatef(transformed.x(), transformed.y(), transformed.z());
			glScalef(_size, _size, _size);
			glCallList(_list);
		glPopMatrix();
	}
};

#endif // _PARTICLE_HH
