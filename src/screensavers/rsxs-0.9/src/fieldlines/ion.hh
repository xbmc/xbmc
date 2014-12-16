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
#ifndef _ION_HH
#define _ION_HH

#include <common.hh>

#include <fieldlines.hh>
#include <vector.hh>

class Ion {
private:
	int _charge;
	Vector _V;
	Vector _vel;
public:
	Ion() : _charge(Common::randomInt(2) ? 1 : -1), _V(
		Common::randomFloat(2.0f * WIDE) - WIDE,
		Common::randomFloat(2.0f * HIGH) - HIGH,
		Common::randomFloat(2.0f * WIDE) - WIDE
	), _vel(
		Common::randomFloat(Hack::speed * 0.1f) - (Hack::speed * 0.05f),
		Common::randomFloat(Hack::speed * 0.1f) - (Hack::speed * 0.05f),
		Common::randomFloat(Hack::speed * 0.1f) - (Hack::speed * 0.05f)
	) {}

	int getCharge() const {
		return _charge;
	}

	const Vector& getV() const {
		return _V;
	}

	void update() {
		_V += _vel;
		if (_V.x() > WIDE)
			_vel.x() = _vel.x() - 0.001f * Hack::speed;
		if (_V.x() < -WIDE)
			_vel.x() = _vel.x() + 0.001f * Hack::speed;
		if (_V.y() > HIGH)
			_vel.y() = _vel.y() - 0.001f * Hack::speed;
		if (_V.y() < -HIGH)
			_vel.y() = _vel.y() + 0.001f * Hack::speed;
		if (_V.z() > WIDE)
			_vel.z() = _vel.z() - 0.001f * Hack::speed;
		if (_V.z() < -WIDE)
			_vel.z() = _vel.z() + 0.001f * Hack::speed;
	}
};

#endif // _ION_HH
