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
#ifndef _EXPLOSION_HH
#define _EXPLOSION_HH

#include <common.hh>

#include <color.hh>
#include <particle.hh>
#include <vector.hh>

class Explosion : public Particle {
public:
	enum Type {
		EXPLODE_NONE = -1,
		EXPLODE_SPHERE,
		EXPLODE_SPLIT_SPHERE,
		EXPLODE_MULTICOLORED_SPHERE,
		EXPLODE_RING,
		EXPLODE_DOUBLE_SPHERE,
		EXPLODE_SPHERE_INSIDE_RING,
		EXPLODE_STREAMERS,
		EXPLODE_METEORS,
		EXPLODE_STARS_INSIDE_STREAMERS,
		EXPLODE_STARS_INSIDE_METEORS,
		EXPLODE_STREAMERS_INSIDE_STARS,
		EXPLODE_METEORS_INSIDE_STARS,
		EXPLODE_STAR_BOMBS,
		EXPLODE_STREAMER_BOMBS,
		EXPLODE_METEOR_BOMBS,
		EXPLODE_CRACKER_BOMBS,
		EXPLODE_BEES,
		EXPLODE_FLASH,
		EXPLODE_SPINNER,
		EXPLODE_SUCKER,
		EXPLODE_SHOCKWAVE,
		EXPLODE_STRETCHER,
		EXPLODE_BIGMAMA,
		EXPLODE_STARS_FROM_BOMB,
		EXPLODE_STREAMERS_FROM_BOMB,
		EXPLODE_METEORS_FROM_BOMB
	};
private:
	RGBColor _RGB;
	float _size;
	float _brightness;

	void popSphere(unsigned int, float, const RGBColor&) const;
	void popSplitSphere(unsigned int, float, const RGBColor&, const RGBColor&) const;
	void popMultiColorSphere(unsigned int, float, const RGBColor[3]) const;
	void popRing(unsigned int, float, const RGBColor&) const;
	void popStreamers(unsigned int, float, const RGBColor&) const;
	void popMeteors(unsigned int, float, const RGBColor&) const;
	void popStarBombs(unsigned int, float, const RGBColor&) const;
	void popStreamerBombs(unsigned int, float, const RGBColor&) const;
	void popMeteorBombs(unsigned int, float, const RGBColor&) const;
	void popCrackerBombs(unsigned int, float) const;
	void popBees(unsigned int, float, const RGBColor&) const;
public:
	Explosion(
		const Vector&, const Vector&, Type,
		const RGBColor& = RGBColor(), float lifetime = 0.5f
	);

	void update();
	void updateCameraOnly();
	void draw() const;
};

#endif // _EXPLOSION_HH
