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
#ifndef _WIND_HH
#define _WIND_HH

#include <common.hh>

#include <color.hh>
#include <vector.hh>

#define NUMCONSTS 9

class Wind {
private:
	static GLuint _list;
	static GLuint _texture;

	std::vector<Vector> _emitters;
	std::vector<Vector> _particlesXYZ;
	std::vector<RGBColor> _particlesRGB;
	std::vector<std::pair<unsigned int, unsigned int> > _lineList;
	std::vector<unsigned int> _lastParticle;
	unsigned int _whichParticle;
	float _c[NUMCONSTS];
	float _ct[NUMCONSTS];
	float _cv[NUMCONSTS];
public:
	static void init();

	Wind();

	void update();
};

#endif // _WIND_HH
