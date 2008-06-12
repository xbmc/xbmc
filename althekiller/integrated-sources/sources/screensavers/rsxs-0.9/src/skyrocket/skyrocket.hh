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
#ifndef _SKYROCKET_HH
#define _SKYROCKET_HH

#include <common.hh>

#include <color.hh>
#include <hack.hh>
#include <vector.hh>

// User-supplied parameters
namespace Hack {
	extern bool drawMoon;
	extern bool drawClouds;
	extern bool drawEarth;
	extern bool drawIllumination;
	extern bool drawSunset;

	extern unsigned int maxRockets;

	extern unsigned int starDensity;
	extern float moonGlow;
	extern float ambient;
	extern float flares;

	extern float smoke;
	extern unsigned int explosionSmoke;

	extern float wind;

	extern float volume;
	extern std::string openalSpec;
	extern float speedOfSound;
};

#include <particle.hh>
#include <smoke.hh>
#include <world.hh>

// Public stuff
namespace Hack {
	extern std::vector<Particle*> pending;
	extern unsigned int numDead;
	extern unsigned int numRockets;

	extern Vector cameraPos;
	extern Vector cameraVel;
	extern UnitQuat cameraDir;
	extern RotationMatrix cameraMat;

	extern bool frameToggle;

	inline void illuminate(
		const Vector&, const RGBColor&,
		float, float, float
	);
	inline void flare(const Vector&, const RGBColor&, float);
	inline void superFlare(const Vector&, const RGBColor&, float);
	inline void suck(const Vector&, float);
	inline void shock(const Vector&, float);
	inline void stretch(const Vector&, float);
};

// Private stuff
namespace Hack {
	extern int _viewport[4];
	extern double _projectionMat[16];
	extern double _modelMat[16];

	struct _Illumination {
		Vector pos;
		RGBColor RGB;
		float brightness;
		float smokeDistanceSquared;
	};
	extern std::list<_Illumination> _illuminationList;

	struct _Flare {
		float x;
		float y;
		RGBColor RGB;
		float alpha;
	};
	extern std::list<_Flare> _flareList;
	extern std::list<_Flare> _superFlareList;

	struct _Influence {
		Vector pos;
		float factor;
	};
	extern std::list<_Influence> _suckList;
	extern std::list<_Influence> _shockList;
	extern std::list<_Influence> _stretchList;
};

inline void Hack::illuminate(
	const Vector& pos, const RGBColor& RGB,
	float brightness, float smokeDistanceSquared,
	float cloudDistanceSquared
) {
	if (!drawIllumination)
		return;

	RGBColor newRGB(RGB * 0.6f + 0.4f);

	if (smokeDistanceSquared > 0.0f) {
		_Illumination ill = { pos, RGB, brightness, smokeDistanceSquared };
		_illuminationList.push_back(ill);
	}

	if (drawClouds && cloudDistanceSquared > 0.0f)
		World::illuminateClouds(
			pos, newRGB, brightness, cloudDistanceSquared
		);
}

inline void Hack::flare(
	const Vector& pos, const RGBColor& RGB, float brightness
) {
	Vector diff(pos - cameraPos);
	UnitVector forwards(cameraDir.forward());
	if (Vector::dot(diff, forwards) <= 1.0f)
		return;

	double winX, winY, winZ;
	gluProject(
		pos.x(), pos.y(), pos.z(),
		_modelMat, _projectionMat, _viewport,
		&winX, &winY, &winZ
	);

	float attenuation = 1.0f - diff.length() * 0.0001f;
	if (attenuation < 0.0f)
		attenuation = 0.0f;
	_Flare flare = {
		winX * Common::aspectRatio / Common::width,
		winY / Common::height,
		RGB,
		brightness * flares * 0.01f * attenuation
	};
	_flareList.push_back(flare);
}

inline void Hack::superFlare(
	const Vector& pos, const RGBColor& RGB, float brightness
) {
	Vector diff(pos - cameraPos);
	UnitVector forwards(cameraDir.forward());
	if (Vector::dot(diff, forwards) <= 1.0f)
		return;

	double winX, winY, winZ;
	gluProject(
		pos.x(), pos.y(), pos.z(),
		_modelMat, _projectionMat, _viewport,
		&winX, &winY, &winZ
	);

	float attenuation = 1.0f - diff.length() * 0.00005f;
	if (attenuation < 0.0f)
		attenuation = 0.0f;
	float temp = 1.0f - (brightness - 0.5f) * flares * 0.02f;
	_Flare flare = {
		winX * Common::aspectRatio / Common::width,
		winY / Common::height,
		RGB,
		(1.0f - temp * temp * temp * temp) * attenuation
	};
	_flareList.push_back(flare);
}

inline void Hack::suck(const Vector& pos, float factor) {
	_Influence suck = { pos, factor };
	_suckList.push_back(suck);
}

inline void Hack::shock(const Vector& pos, float factor) {
	_Influence shock = { pos, factor };
	_shockList.push_back(shock);
}

inline void Hack::stretch(const Vector& pos, float factor) {
	_Influence stretch = { pos, factor };
	_stretchList.push_back(stretch);
}

#endif // _SKYROCKET_HH
