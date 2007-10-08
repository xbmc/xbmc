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

#include <color.hh>

/*
 * This library converts between colors defined with RGB values and HSL
 * values. It also finds in-between colors by moving linearly through
 * HSL space.
 * All functions take values for r, g, b, h, s, and l between 0.0 and 1.0
 * (RGB = red, green, blue; HSL = hue, saturation, luminosity)
 */

/*
 * For the 'tween functions, a tween value of 0.0 will output the first
 * color while a tween value of 1.0 will output the second color.
 * A value of false for direction indicates a positive progression around
 * the color wheel (i.e. red -> yellow -> green -> cyan...). A value of
 * true does the opposite.
 */

RGBColor::operator HSLColor() const {
	unsigned int huezone = 0;
	float rr, gg, bb, h, s, l;

	// find huezone
	if (_v[0] >= _v[1]) {
		huezone = 0;
		if (_v[2] > _v[0])
			huezone = 4;
		else {
			if (_v[2] > _v[1])
				huezone = 5;
		}
	} else {
		huezone = 1;
		if (_v[2] > _v[1])
			huezone = 2;
		else {
			if (_v[2] > _v[0])
				huezone = 3;
		}
	}

	// luminosity
	switch (huezone) {
	case 0:
	case 5:
		l = _v[0];
		rr = 1.0f;
		gg = _v[1] / l;
		bb = _v[2] / l;
		break;
	case 1:
	case 2:
		l = _v[1];
		gg = 1.0f;
		rr = _v[0] / l;
		bb = _v[2] / l;
		break;
	default:
		l = _v[2];
		bb = 1.0f;
		rr = _v[0] / l;
		gg = _v[1] / l;
	}

	if (l == 0.0)
		return HSLColor(0, 1, 0);

	// saturation
	switch (huezone) {
	case 0:
	case 1:
		s = 1.0f - _v[2];
		bb = 0.0f;
		rr = 1.0f - ((1.0f - rr) / s);
		gg = 1.0f - ((1.0f - gg) / s);
		break;
	case 2:
	case 3:
		s = 1.0f - _v[0];
		rr = 0.0f;
		gg = 1.0f - ((1.0f - gg) / s);
		bb = 1.0f - ((1.0f - bb) / s);
		break;
	default:
		s = 1.0f - _v[1];
		gg = 0.0f;
		rr = 1.0f - ((1.0f - rr) / s);
		bb = 1.0f - ((1.0f - bb) / s);
	}

	// hue
	switch (huezone) {
	case 0:
		h = _v[1] / 6.0f;
		break;
	case 1:
		h = ((1.0f - _v[0]) / 6.0f) + 0.166667f;
		break;
	case 2:
		h = (_v[2] / 6.0f) + 0.333333f;
		break;
	case 3:
		h = ((1.0f - _v[1]) / 6.0f) + 0.5f;
		break;
	case 4:
		h = (_v[0] / 6.0f) + 0.666667f;
		break;
	default:
		h = ((1.0f - _v[2]) / 6.0f) + 0.833333f;
	}

	return HSLColor(h, s, l);
}

RGBColor RGBColor::tween(const RGBColor& a, const RGBColor& b,
		float tween, bool direction) {
	return HSLColor::tween(HSLColor(a), HSLColor(b), tween, direction);
}

HSLColor::operator RGBColor() const {
	float r, g, b;

	// hue influence
	if (_v[0] < 0.166667) { // full red, some green
		r = 1.0;
		g = _v[0] * 6.0f;
		b = 0.0;
	} else {
		if (_v[0] < 0.5) { // full green
			g = 1.0;
			if (_v[0] < 0.333333) { // some red
				r = 1.0f - ((_v[0] - 0.166667f) * 6.0f);
				b = 0.0;
			} else { // some blue
				b = (_v[0] - 0.333333f) * 6.0f;
				r = 0.0;
			}
		} else {
			if (_v[0] < 0.833333) { // full blue
				b = 1.0;
				if (_v[0] < 0.666667) { // some green
					g = 1.0f - ((_v[0] - 0.5f) * 6.0f);
					r = 0.0;
				} else { // some red
					r = (_v[0] - 0.666667f) * 6.0f;
					g = 0.0;
				}
			} else { // full red, some blue
				r = 1.0;
				b = 1.0f - ((_v[0] - 0.833333f) * 6.0f);
				g = 0.0;
			}
		}
	}

	// saturation influence
	r = 1.0f - (_v[1] * (1.0f - r));
	g = 1.0f - (_v[1] * (1.0f - g));
	b = 1.0f - (_v[1] * (1.0f - b));

	// luminosity influence
	return RGBColor(r * _v[2], g * _v[2], b * _v[2]);
}

HSLColor HSLColor::tween(const HSLColor& a, const HSLColor& b,
		float tween, bool direction) {
	float h, s, l;

	// hue
	if (!direction) { // forward around color wheel
		if (b.h() >= a.h())
			h = a.h() + (tween * (b.h() - a.h()));
		else {
			h = a.h() + (tween * (1.0f - (a.h() - b.h())));
			if (h > 1.0)
				h -= 1.0;
		}
	} else { // backward around color wheel
		if (a.h() >= b.h())
			h = a.h() - (tween * (a.h() - b.h()));
		else {
			h = a.h() - (tween * (1.0f - (b.h() - a.h())));
			if (h < 0.0)
				h += 1.0;
		}
	}

	// saturation
	s = a.s() + (tween * (b.s() - a.s()));

	// luminosity
	l = a.l() + (tween * (b.l() - a.l()));

	return HSLColor(h, s, l);
}
