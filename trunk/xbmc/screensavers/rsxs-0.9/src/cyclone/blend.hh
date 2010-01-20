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
#ifndef _BLEND_HH
#define _BLEND_HH

#include <common.hh>

#include <cyclone.hh>

namespace Blend {
	extern float _fact[13];

	void init();

	static inline float blend(unsigned int i, float step) {
		return _fact[Hack::complexity + 2] / (_fact[i]
			* _fact[Hack::complexity + 2 - i]) * std::pow(step, float(i))
			* std::pow(1.0f - step, float(Hack::complexity + 2 - i));
	}
};

#endif // _BLEND_HH
