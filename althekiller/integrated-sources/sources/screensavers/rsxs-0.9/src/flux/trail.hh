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
#ifndef _TRAIL_HH
#define _TRAIL_HH

#include <common.hh>
#include <flux.hh>
#include <vector.hh>

class Trail {
private:
	static GLuint _list;
	static GLuint _lightTexture;

	std::vector<Vector> _vertices;
	std::vector<float> _hues;
	std::vector<float> _sats;
	unsigned int _counter;
	Vector _offset;
public:
	static void init();

	Trail() {
		static unsigned int whichTrail = 0;

		// Offsets are somewhat like default positions for the head of each
		// particle trail. Offsets spread out the particle trails and keep
		// them from all overlapping.
		_offset.set(
			std::cos(M_PI * 2.0f * float(whichTrail) / float(Hack::numTrails)),
			float(whichTrail) / float(Hack::numTrails) - 0.5f,
			std::sin(M_PI * 2.0f * float(whichTrail) / float(Hack::numTrails))
		);
		++whichTrail;

		// Initialize memory and set initial positions out of view of the
		// camera
		_vertices.resize(Hack::trailLength, Vector(0.0f, 3.0f, 0.0f));
		_hues.resize(Hack::trailLength);
		_sats.resize(Hack::trailLength);

		_counter = 0;
	}

	void update(const float*, float, float);
};

#endif // _TRAIL_HH
