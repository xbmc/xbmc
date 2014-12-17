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
#ifndef _BUG_HH
#define _BUG_HH

#include <common.hh>

#include <color.hh>
#include <vector.hh>

class Bug {
protected:
	static float _wide, _high, _deep;
	static GLuint _list;

	HSLColor _HSL;
	RGBColor _halfRGB;
	Vector _XYZ;
	Vector _speed;
	bool _right, _up, _forward;
	float _maxSpeed;
	float _accel;

	void update();
	Bug(const Vector&, bool, float, float);
public:
	static void init();
	static void fini();
	static void initBoundaries();
};

class Follower;

class Leader : public Bug {
private:
	float _craziness;	// How prone to switching direction is this leader
	float _nextChange;	// Time until this leader's next direction change

	friend class Follower;
public:
	Leader();
	void update();
};

class Follower : public Bug {
private:
	std::vector<Leader>::const_iterator _leader;
public:
	Follower(const std::vector<Leader>::const_iterator&);
	void update(const std::vector<Leader>&);
};

#endif // _BUG_HH
