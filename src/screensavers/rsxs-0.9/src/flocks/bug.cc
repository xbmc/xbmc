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
#include <common.hh>

#include <bug.hh>
#include <color.hh>
#include <flocks.hh>
#include <resource.hh>
#include <vector.hh>

float Bug::_wide, Bug::_high, Bug::_deep;
GLuint Bug::_list;

void Bug::init() {
	if (Hack::blobs) {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		float ambient[4] =  {   0.25f,  0.25f,  0.25f, 0.0f };
		float diffuse[4] =  {   1.0f,   1.0f,   1.0f,  0.0f };
		float specular[4] = {   1.0f,   1.0f,   1.0f,  0.0f };
		float position[4] = { 500.0f, 500.0f, 500.0f,  0.0f };
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		glEnable(GL_COLOR_MATERIAL);
		glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);
		glColorMaterial(GL_FRONT, GL_SPECULAR);
		glColor3f(0.7f, 0.7f, 0.7f);
		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

		_list = Common::resources->genLists(1);
		glNewList(_list, GL_COMPILE);
			GLUquadricObj* qObj = gluNewQuadric();
			gluSphere(qObj, Hack::size * 0.5f, Hack::complexity + 2,
				Hack::complexity + 1);
			gluDeleteQuadric(qObj);
		glEndList();
	}
}

void Bug::initBoundaries() {
	if (Common::aspectRatio > 1.0f) {
		_high = _deep = 160.0f;
		_wide = _high * Common::aspectRatio;
		glTranslatef(0.0f, 0.0f, -_wide * 2.0f);
	} else {
		_wide = _deep = 160.0f;
		_high = _wide * Common::aspectRatio;
		glTranslatef(0.0f, 0.0f, -_high * 2.0f);
	}
}

Bug::Bug(const Vector& XYZ, bool dir, float maxSpeed, float accel) :
	_HSL(Common::randomFloat(1.0f), 1.0f, 1.0f), _XYZ(XYZ), _right(dir),
	_up(dir), _forward(dir), _maxSpeed(maxSpeed), _accel(accel) {}

void Bug::update() {
	_speed.x() = Common::clamp(_speed.x(), -_maxSpeed, _maxSpeed);
	_speed.y() = Common::clamp(_speed.y(), -_maxSpeed, _maxSpeed);
	_speed.z() = Common::clamp(_speed.z(), -_maxSpeed, _maxSpeed);

	_XYZ += _speed * Common::elapsedSecs;

	RGBColor RGB(_HSL);
	_halfRGB = RGB * 0.5f;

	glColor3fv(RGB.get());
	if (Hack::blobs) {
		glPushMatrix();
			glTranslatef(_XYZ.x(), _XYZ.y(), _XYZ.z());
			if (Hack::stretch > 0.0f) {
				float scale = _speed.length() * Hack::stretch * 0.002;
				if (scale < 1.0f) scale = 1.0f;
				Vector rotator(_speed);
				rotator.normalize();
				glRotatef(
					float(std::atan2(-rotator.x(), -rotator.z())) * R2D,
					0.0f, 1.0f, 0.0f
				);
				glRotatef(
					float(std::asin(rotator.y())) * R2D,
					1.0f, 0.0f, 0.0f
				);
				glScalef(1.0f, 1.0f, scale);
			}
			glCallList(_list);
		glPopMatrix();
	} else {
		if (Hack::stretch) {
			glLineWidth(Hack::size * float(700 - _XYZ.z()) * 0.0002f);
			Vector scaler(_speed);
			scaler.normalize();
			scaler *= Hack::stretch;
			glBegin(GL_LINES);
				glVertex3fv((_XYZ - scaler).get());
				glVertex3fv((_XYZ + scaler).get());
			glEnd();
		} else {
			glPointSize(Hack::size * float(700 - _XYZ.z()) * 0.001f);
			glBegin(GL_POINTS);
				glVertex3fv(_XYZ.get());
			glEnd();
		}
	}
}

Leader::Leader() : Bug(
	Vector(
		Common::randomFloat(_wide * 2.0f) - _wide,
		Common::randomFloat(_high * 2.0f) - _high,
		Common::randomFloat(_deep * 2.0f) + _deep * 2.0f
	), true, 8.0f * Hack::speed, 13.0f * Hack::speed),
	_craziness(Common::randomFloat(4.0f) + 0.05f), _nextChange(1.0f) {}

void Leader::update() {
	_nextChange -= Common::elapsedSecs;
	if (_nextChange <= 0.0f) {
		if (Common::randomInt(2))
			_right = !_right;
		if (Common::randomInt(2))
			_up = !_up;
		if (Common::randomInt(2))
			_forward = !_forward;
		_nextChange = Common::randomFloat(_craziness);
	}
	_speed += Vector(
		_right   ? _accel  : -_accel,
		_up      ? _accel  : -_accel,
		_forward ? -_accel : _accel
	) * Common::elapsedSecs;
	if (_XYZ.x() < -_wide) _right = true;
	if (_XYZ.x() > _wide)  _right = false;
	if (_XYZ.y() < -_high) _up = true;
	if (_XYZ.y() > _high)  _up = false;
	if (_XYZ.z() < -_deep) _forward = false;
	if (_XYZ.z() > _deep)  _forward = true;
	if (Hack::chromatek) {
		float h = Common::clamp(
			0.666667f * ((_wide - _XYZ.z()) / (2.0f * _wide)),
			0.0f, 0.666667f
		);
		_HSL.h() = h;
	}

	Bug::update();
}

Follower::Follower(const std::vector<Leader>::const_iterator& leader) : Bug(
	Vector(
		Common::randomFloat(_wide * 2.0f) - _wide,
		Common::randomFloat(_high * 2.0f) - _high,
		Common::randomFloat(_deep * 5.0f) + _deep * 2.5f
	), false, (Common::randomFloat(6.0f) + 4.0f) * Hack::speed,
	(Common::randomFloat(4.0f) + 9.0f) * Hack::speed),
	_leader(leader) {}

void Follower::update(const std::vector<Leader>& leaders) {
	if (!Common::randomInt(10)) {
		float oldDistance = 10000000.0f;
		for (
			std::vector<Leader>::const_iterator l = leaders.begin();
			l != leaders.end();
			++l
		) {
			float newDistance = (l->_XYZ - _XYZ).lengthSquared();
			if (newDistance < oldDistance) {
				oldDistance = newDistance;
				_leader = l;
			}
		}
	}
	_speed += Vector(
		(_leader->_XYZ.x() - _XYZ.x()) > 0.0f ?
			_accel : -_accel,
		(_leader->_XYZ.y() - _XYZ.y()) > 0.0f ?
			_accel : -_accel,
		(_leader->_XYZ.z() - _XYZ.z()) > 0.0f ?
			_accel : -_accel
	) * Common::elapsedSecs;
	if (Hack::chromatek) {
		_HSL.h() = Common::clamp(
			0.666667f * ((_wide - _XYZ.z()) / (2.0f * _wide)),
			0.0f, 0.666667f
		);
	} else {
		float h = _HSL.h();
		float leaderH = _leader->_HSL.h();
		if (std::abs(h - leaderH) < (Hack::colorFadeSpeed * Common::elapsedSecs)) {
			_HSL.h() = leaderH;
		} else {
			if (std::abs(h - leaderH) < 0.5f) {
				if (h > leaderH)
					h -= Hack::colorFadeSpeed * Common::elapsedSecs;
				else
					h += Hack::colorFadeSpeed * Common::elapsedSecs;
			} else {
				if (h > leaderH)
					h += Hack::colorFadeSpeed * Common::elapsedSecs;
				else
					h -= Hack::colorFadeSpeed * Common::elapsedSecs;
				if (h > 1.0f) h -= 1.0f;
				if (h < 0.0f) h += 1.0f;
			}
			_HSL.h() = h;
		}
	}

	Bug::update();

	if (Hack::connections) {
		glLineWidth(1.0f);
		glBegin(GL_LINES);
			glColor3fv(_halfRGB.get());
			glVertex3fv(_XYZ.get());
			glColor3fv(_leader->_halfRGB.get());
			glVertex3fv(_leader->_XYZ.get());
		glEnd();
	}
}
