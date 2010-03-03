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

#include <explosion.hh>
#include <flares.hh>
#include <fountain.hh>
#include <overlay.hh>
#include <resources.hh>
#include <rocket.hh>
#include <shockwave.hh>
#include <skyrocket.hh>
#include <smoke.hh>
#include <world.hh>
#include <vector.hh>

namespace Hack {
	bool drawMoon = true;
	bool drawClouds = true;
	bool drawEarth = true;
	bool drawIllumination = true;
	bool drawSunset;

	unsigned int maxRockets = 8;

	unsigned int starDensity = 20;
	float moonGlow = 20.0f;
	float ambient = 10.0f;
	float flares = 20.0f;

	float smoke = 5.0f;
	unsigned int explosionSmoke = 0;

	float wind = 20.0f;

#if HAVE_SOUND
	float volume = 100.0f;
#else
	float volume = 0.0f;
#endif
	std::string openalSpec("");
	float speedOfSound = 1130.0f;
};

namespace Hack {
	enum Arguments {
		ARG_ROCKETS = 1,
		ARG_SMOKE,
		ARG_EXPLOSIONSMOKE,
		ARG_STARS,
		ARG_HALO,
		ARG_AMBIENT,
		ARG_WIND,
		ARG_FLARES,
		ARG_VOLUME,
		ARG_OPENAL_SPEC,
		ARG_SPEED_OF_SOUND,
		ARG_MOON = 0x100, ARG_NO_MOON,
		ARG_CLOUDS = 0x200, ARG_NO_CLOUDS,
		ARG_EARTH = 0x300, ARG_NO_EARTH,
		ARG_GLOW = 0x400, ARG_NO_GLOW
	};

	bool _action;

	enum CameraMode {
		CAMERA_FIXED,
		CAMERA_AUTO,
		CAMERA_MANUAL
	} _cameraMode;
	float _cameraTimeTotal;
	float _cameraTimeElapsed;

	Vector _cameraStartPos;
	RotationMatrix _cameraStartDir;
	Vector _cameraEndPos;
	RotationMatrix _cameraEndDir;

	Vector _cameraTransPos;
	UnitQuat _cameraTransDir;
	float _cameraTransAngle;
	UnitVector _cameraTransAxis;

	Vector cameraPos;
	Vector cameraVel;
	RotationMatrix cameraMat;
	RotationMatrix _cameraMatInv;
	UnitQuat cameraDir;
	float _cameraSpeed;

	int _viewport[4];
	double _projectionMat[16];
	double _modelMat[16];

	int _mouseX, _mouseY;
	bool _leftButton, _middleButton, _rightButton;
	bool _mouseInWindow;
	float _mouseIdleTime;

	Explosion::Type _userDefinedExplosion;

	std::vector<Particle*> _particles;
	std::vector<Particle*> pending;
	struct _ParticleSorter :
		public std::binary_function<const Particle*, const Particle*, bool>
	{
		bool operator()(const Particle* a, const Particle* b) {
			return *a < *b;
		}
	} ParticleSorter;
	unsigned int numDead;
	unsigned int numRockets;

	std::list<_Illumination> _illuminationList;
	std::list<_Flare> _flareList;
	std::list<_Flare> _superFlareList;
	std::list<_Influence> _suckList;
	std::list<_Influence> _shockList;
	std::list<_Influence> _stretchList;

	bool frameToggle;

	void newCameraTarget();
	void firstCamera();
	void newCamera();
	void chainCamera();
	void crashChainCamera();

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_ROCKETS:
		if (Common::parseArg(arg, maxRockets, 1u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"maximum number of rockets must be between 1 and 100");
		return 0;
	case ARG_SMOKE:
		if (Common::parseArg(arg, smoke, 0.0f, 60.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"smoke lifetime must be between 0 and 60");
		return 0;
	case ARG_EXPLOSIONSMOKE:
		if (Common::parseArg(arg, explosionSmoke, 0u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"amount of explosion smoke must be between 0 and 100");
		return 0;
	case ARG_STARS:
		if (Common::parseArg(arg, starDensity, 0u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"star field density must be between 0 and 100");
		return 0;
	case ARG_HALO:
		if (Common::parseArg(arg, moonGlow, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"moon halo intensity must be between 0 and 100");
		return 0;
	case ARG_AMBIENT:
		if (Common::parseArg(arg, ambient, 0.0f, 50.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"ambient illumination must be between 0 and 50");
		return 0;
	case ARG_WIND:
		if (Common::parseArg(arg, ambient, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"wind speed must be between 0 and 100");
		return 0;
	case ARG_FLARES:
		if (Common::parseArg(arg, flares, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"lens flare brightness must be between 0 and 100");
		return 0;
	case ARG_VOLUME:
		if (Common::parseArg(arg, volume, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"audio volume must be between 0 and 100");
		return 0;
	case ARG_OPENAL_SPEC:
 		openalSpec = arg;
		return 0;
	case ARG_SPEED_OF_SOUND:
		if (Common::parseArg(arg, speedOfSound, 500.0f, 40000.0f, 0.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"speed of sound must be 0, or between 500 and 40000");
		return 0;
	case ARG_MOON:
		drawMoon = true;
		return 0;
	case ARG_NO_MOON:
		drawMoon = false;
		return 0;
	case ARG_CLOUDS:
		drawClouds = true;
		return 0;
	case ARG_NO_CLOUDS:
		drawClouds = false;
		return 0;
	case ARG_EARTH:
		drawEarth = true;
		return 0;
	case ARG_NO_EARTH:
		drawEarth = false;
		return 0;
	case ARG_GLOW:
		drawIllumination = true;
		return 0;
	case ARG_NO_GLOW:
		drawIllumination = false;
		return 0;
	case ARGP_KEY_FINI:
		drawSunset = Common::randomInt(4) == 0;
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Rocket options:" },
		{ "rockets", ARG_ROCKETS, "NUM", 0,
			"Maximum number of rockets (1-100, default = 8)" },
		{ NULL, 0, NULL, 0, "" },
		{ "smoke", ARG_SMOKE, "NUM", 0, "Smoke lifetime (0-60, default = 5)" },
		{ "explosionsmoke", ARG_EXPLOSIONSMOKE, "NUM", 0,
			"Amount of smoke from explosions (0-100, default = 0)" },
		{ NULL, 0, NULL, 0, "Environment options:" },
		{ "no-moon", ARG_NO_MOON, NULL, 0, "Do not draw the moon" },
		{ "moon", ARG_MOON, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "no-clouds", ARG_NO_CLOUDS, NULL, 0, "Do not draw clouds" },
		{ "clouds", ARG_CLOUDS, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "no-earth", ARG_NO_EARTH, NULL, 0, "Do not draw the earth" },
		{ "earth", ARG_EARTH, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "no-glow", ARG_NO_GLOW, NULL, 0,
			"Disable illumination of smoke and clouds by rockets and explosions" },
		{ "glow", ARG_GLOW, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ NULL, 0, NULL, 0, "" },
		{ "stars", ARG_STARS, "NUM", 0,
			"Density of star field (0-100, default = 20)" },
		{ "halo", ARG_HALO, "NUM", 0,
			"Intensity of moon halo (0-100, default = 20)" },
		{ "ambient", ARG_AMBIENT, "NUM", 0,
			"Ambient illumination (0-50, default = 10)" },
		{ "wind", ARG_WIND, "NUM", 0, "Wind speed (0-100, default = 20)" },
		{ NULL, 0, NULL, 0, "Other options:" },
		{ "flares", ARG_FLARES, "NUM", 0,
			"Lens flare brightness (0-100, default = 20)" },
#if HAVE_SOUND
		{ "volume", ARG_VOLUME, "NUM", 0, "Audio volume (0-100, default = 100)" },
		{ "openal-spec", ARG_OPENAL_SPEC, "SPEC", OPTION_HIDDEN },
		{ "speed-of-sound", ARG_SPEED_OF_SOUND, "FPS", OPTION_HIDDEN },
#else
		{ "volume", ARG_VOLUME, "NUM", OPTION_HIDDEN },
		{ "openal-spec", ARG_OPENAL_SPEC, "SPEC", OPTION_HIDDEN },
		{ "speed-of-sound", ARG_SPEED_OF_SOUND, "FPS", OPTION_HIDDEN },
#endif
		{}
	};
	static struct argp parser = {
		options, parse, NULL, "The mother of all fireworks screensavers."
	};
	return &parser;
}

std::string Hack::getShortName() { return "skyrocket"; }
std::string Hack::getName()      { return "Skyrocket"; }

void Hack::newCameraTarget() {
	_cameraEndPos.set(
		Common::randomFloat(6000.0f) - 3000.0f,
		Common::randomFloat(1700.0f) + 5.0f,
		Common::randomFloat(6000.0f) - 3000.0f
	);
	_cameraTransPos = _cameraEndPos - _cameraStartPos;
	_cameraEndDir = RotationMatrix::lookAt(
		_cameraEndPos,
		Vector(
			Common::randomFloat(1000.0f) - 500.0f,
			Common::randomFloat(1100.0f) + 200.0f,
			Common::randomFloat(1000.0f) - 500.0f
		),
		Vector(0.0f, 1.0f, 0.0f)
	).inverted();
	_cameraTransDir =
		UnitQuat(RotationMatrix(_cameraStartDir).transposed() * _cameraEndDir);
	_cameraTransDir.toAngleAxis(&_cameraTransAngle, &_cameraTransAxis);
}

void Hack::firstCamera() {
	_cameraTimeTotal = 20.0f;
	_cameraTimeElapsed = 0.0f;

	{
		_cameraStartPos.set(
			Common::randomFloat(1000.0f) + 5000.0f,
			5.0f,
			Common::randomFloat(4000.0f) - 2000.0f
		);
		_cameraStartDir = RotationMatrix::lookAt(
			_cameraStartPos,
			Vector(0.0f, 1200.0f, 0.0f),
			Vector(0.0f, 1.0f, 0.0f)
		).inverted();
	}

	{
		_cameraEndPos.set(
			Common::randomFloat(3000.0f),
			400.0f,
			Common::randomFloat(3000.0f) - 1500.0f
		);
		_cameraTransPos = _cameraEndPos - _cameraStartPos;
		_cameraEndDir =	RotationMatrix::lookAt(
			_cameraEndPos,
			Vector(0.0f, 1200.0f, 0.0f),
			Vector(0.0f, 1.0f, 0.0f)
		).inverted();
		_cameraTransDir = UnitQuat(
			RotationMatrix(_cameraStartDir).transposed() * _cameraEndDir);
		_cameraTransDir.toAngleAxis(&_cameraTransAngle, &_cameraTransAxis);
	}
}

void Hack::newCamera() {
	_cameraTimeTotal = Common::randomFloat(25.0f) + 5.0f;
	_cameraTimeElapsed = 0.0f;

	_cameraStartPos.set(
		Common::randomFloat(6000.0f) - 3000.0f,
		Common::randomFloat(1700.0f) + 5.0f,
		Common::randomFloat(6000.0f) - 3000.0f
	);
	_cameraStartDir = RotationMatrix::lookAt(
		_cameraStartPos,
		Vector(
			Common::randomFloat(1000.0f) - 500.0f,
			Common::randomFloat(1100.0f) + 200.0f,
			Common::randomFloat(1000.0f) - 500.0f
		),
		Vector(0.0f, 1.0f, 0.0f)
	).inverted();
	newCameraTarget();
}

void Hack::chainCamera() {
	_cameraTimeTotal = Common::randomFloat(25.0f) + 5.0f;
	_cameraTimeElapsed = 0.0f;

	_cameraStartPos = _cameraEndPos;
	_cameraStartDir = _cameraEndDir;
	newCameraTarget();
}

void Hack::crashChainCamera() {
	_cameraTimeTotal = Common::randomFloat(2.0f) + 2.0f;
	_cameraTimeElapsed = 0.0f;

	_cameraStartPos = cameraPos;
	_cameraStartDir = _cameraMatInv;
	_cameraEndPos.set(
		_cameraStartPos.x() + Common::randomFloat(800.0f) - 400.0f,
		Common::randomFloat(400.0f) + 600.0f,
		_cameraStartPos.z() + Common::randomFloat(800.0f) - 400.0f
	);
	_cameraTransPos = _cameraEndPos - _cameraStartPos;
	_cameraEndDir = RotationMatrix::lookAt(
		_cameraEndPos,
		_cameraStartPos,
		Vector(0.0f, 1.0f, 0.0f)
	).inverted();
	_cameraTransDir =
		UnitQuat(RotationMatrix(_cameraStartDir).transposed() * _cameraEndDir);
	_cameraTransDir.toAngleAxis(&_cameraTransAngle, &_cameraTransAxis);
}

void Hack::start() {
	_action = true;
	_cameraMode = CAMERA_AUTO;
	_userDefinedExplosion = Explosion::EXPLODE_NONE;

	firstCamera();

	_mouseX = Common::centerX;
	_mouseY = Common::centerY;
	_leftButton = _middleButton = _rightButton = _mouseInWindow = false;
	_mouseInWindow = false;
	_mouseIdleTime = 0.0f;

	glViewport(0, 0, Common::width, Common::height);
	glGetIntegerv(GL_VIEWPORT, _viewport);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, Common::aspectRatio, 1.0, 10000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	Resources::init();
	Shockwave::init();
	Smoke::init();
	World::init();
	Overlay::init();
}

void Hack::reshape() {}

void Hack::tick() {
	// build viewing matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, Common::aspectRatio, 1.0f, 40000.0f);
	glGetDoublev(GL_PROJECTION_MATRIX, _projectionMat);

	// Build modelview matrix
	// Don't use gluLookAt() because it's easier to find the billboard matrix
	// if we know the heading and pitch
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	switch (_cameraMode) {
	case CAMERA_FIXED:
		cameraVel.set(0.0f, 0.0f, 0.0f);
		break;
	case CAMERA_AUTO:
		{
			_cameraTimeElapsed += Common::elapsedSecs;
			if (_cameraTimeElapsed >= _cameraTimeTotal) {
				if (!Common::randomInt(30))
					newCamera();
				else
					chainCamera();
			}

			// change camera position and angle
			float cameraStep = 0.5f *
				(1.0f - std::cos(_cameraTimeElapsed / _cameraTimeTotal * M_PI));
			Vector lastPos(cameraPos);
			cameraPos = _cameraStartPos + _cameraTransPos * cameraStep;
			cameraVel = cameraPos - lastPos;
			cameraDir = _cameraStartDir;
			cameraDir.multiplyBy(
				UnitQuat(_cameraTransAngle * cameraStep, _cameraTransAxis)
			);
			cameraMat = RotationMatrix(cameraDir);
			_cameraMatInv = cameraMat.inverted();
		}
		break;
	case CAMERA_MANUAL:
		{
			float dx = float(
				int(Common::centerX) - int(_mouseX)) /
				float(Common::width);
			dx *= (dx < 0.0f) ? -dx : dx;
			float dy = float(
				int(Common::centerY) - int(_mouseY)) /
				float(Common::height);
			dy *= (dy < 0.0f) ? -dy : dy;
			if (_leftButton)
				_cameraSpeed += Common::elapsedSecs * 400.0f;
			if (_rightButton)
				_cameraSpeed -= Common::elapsedSecs * 400.0f;
			if (_middleButton)
				_cameraSpeed = 0.0f;
			_cameraSpeed = Common::clamp(_cameraSpeed, -1500.0f, 1500.0f);
			cameraDir.multiplyBy(
				UnitQuat::heading(2.0f * Common::elapsedSecs * Common::aspectRatio * dx)
			);
			cameraDir.multiplyBy(
				UnitQuat::roll(5.0f * Common::elapsedSecs * Common::aspectRatio * dx)
			);
			cameraDir.multiplyBy(
				UnitQuat::pitch(5.0f * Common::elapsedSecs * dy)
			);
			cameraMat = RotationMatrix(cameraDir);
			_cameraMatInv = cameraMat.inverted();
			cameraVel = cameraDir.forward() * _cameraSpeed * Common::elapsedSecs;
			cameraPos += cameraVel;
			if (drawEarth && cameraPos.y() < 5.0f) {
				_particles.push_back(new Explosion(
					Vector(cameraPos.x(), 50.0f, cameraPos.z()),
					Vector(0.0f, 200.0f, 0.0f),
					Explosion::EXPLODE_SPHERE)
				);
				_particles.push_back(new Explosion(
					Vector(cameraPos.x(), 1.0f, cameraPos.z()),
					Vector(0.0f, 20.0f, 0.0f),
					Explosion::EXPLODE_FLASH)
				);
				_cameraMode = CAMERA_AUTO;
				Overlay::set("Automatic camera");
				crashChainCamera();
			}
		}
		break;
	}

	glMultMatrixf(_cameraMatInv.get());
	glTranslatef(-cameraPos.x(), -cameraPos.y(), -cameraPos.z());
	glGetDoublev(GL_MODELVIEW_MATRIX, _modelMat);

	glClear(GL_COLOR_BUFFER_BIT);

	if (_action) {
		World::update();

		static float rocketSpacing = 10.0f / float(maxRockets);
		static float changeRocketTimer = 20.0f;
		changeRocketTimer -= Common::elapsedTime;
		if (changeRocketTimer <= 0.0f) {
			float temp = Common::randomFloat(4.0f);
			rocketSpacing = (temp * temp) + (10.0f / float(maxRockets));
			changeRocketTimer = Common::randomFloat(30.0f) + 10.0f;
		}

		static float rocketTimer = 0.0f;
		rocketTimer -= Common::elapsedTime;
		if (
			(rocketTimer <= 0.0f) ||
			(_userDefinedExplosion != Explosion::EXPLODE_NONE)
		) {
			if (numRockets < maxRockets) {
				if (_userDefinedExplosion != Explosion::EXPLODE_NONE)
					_particles.push_back(new Rocket(_userDefinedExplosion));
				else if (Common::randomInt(30)) {
					unsigned int dist = Common::randomInt(5602);
					_particles.push_back(new Rocket(
						(dist < 1200) ? Explosion::EXPLODE_SPHERE :
						(dist < 2400) ? Explosion::EXPLODE_SPLIT_SPHERE :
						(dist < 2680) ? Explosion::EXPLODE_MULTICOLORED_SPHERE :
						(dist < 2980) ? Explosion::EXPLODE_RING :
						(dist < 3280) ? Explosion::EXPLODE_DOUBLE_SPHERE :
						(dist < 3580) ? Explosion::EXPLODE_SPHERE_INSIDE_RING :
						(dist < 3820) ? Explosion::EXPLODE_STREAMERS :
						(dist < 4060) ? Explosion::EXPLODE_METEORS :
						(dist < 4170) ? Explosion::EXPLODE_STARS_INSIDE_STREAMERS :
						(dist < 4280) ? Explosion::EXPLODE_STARS_INSIDE_METEORS :
						(dist < 4390) ? Explosion::EXPLODE_STREAMERS_INSIDE_STARS :
						(dist < 4500) ? Explosion::EXPLODE_METEORS_INSIDE_STARS :
						(dist < 4720) ? Explosion::EXPLODE_STAR_BOMBS :
						(dist < 4940) ? Explosion::EXPLODE_STREAMER_BOMBS :
						(dist < 5160) ? Explosion::EXPLODE_METEOR_BOMBS :
						(dist < 5270) ? Explosion::EXPLODE_CRACKER_BOMBS :
						(dist < 5380) ? Explosion::EXPLODE_BEES :
						(dist < 5490) ? Explosion::EXPLODE_FLASH :
						(dist < 5600) ? Explosion::EXPLODE_SPINNER :
						(dist < 5601) ? Explosion::EXPLODE_SUCKER :
						                Explosion::EXPLODE_STRETCHER
					));
				} else {
					_particles.push_back(new Fountain());
					if (Common::randomInt(2))
						_particles.push_back(new Fountain());
				}
			}
			if (maxRockets > 0)
				rocketTimer = Common::randomFloat(rocketSpacing);
			else
				rocketTimer = 60.0f;	// arbitrary number since no rockets ever fire
			if (_userDefinedExplosion != Explosion::EXPLODE_NONE) {
				_userDefinedExplosion = Explosion::EXPLODE_NONE;
				rocketTimer = 20.0f;
			}
		}

		_particles.insert(_particles.end(), pending.begin(), pending.end());
		pending.clear();

		numDead = 0;
		stdx::call_all_ptr(_particles, &Particle::update);
		std::sort(_particles.begin(), _particles.end(), ParticleSorter);

		std::vector<Particle*>::iterator afterAlive(_particles.end() - numDead);
		stdx::destroy_each_ptr(afterAlive, _particles.end());
		_particles.erase(afterAlive, _particles.end());

		std::vector<Particle*>::iterator begin1(_particles.begin());
		std::vector<Particle*>::iterator end1(_particles.end());
		std::list<_Illumination>::iterator begin2(_illuminationList.begin());
		std::list<_Illumination>::iterator end2(_illuminationList.end());
		std::list<_Influence>::iterator begin3(_suckList.begin());
		std::list<_Influence>::iterator end3(_suckList.end());
		std::list<_Influence>::iterator begin4(_shockList.begin());
		std::list<_Influence>::iterator end4(_shockList.end());
		std::list<_Influence>::iterator begin5(_stretchList.begin());
		std::list<_Influence>::iterator end5(_stretchList.end());
		for (std::vector<Particle*>::iterator it1 = begin1; it1 != end1; ++it1) {
			for (std::list<_Illumination>::iterator it2 = begin2; it2 != end2; ++it2)
				(*it1)->illuminate(it2->pos, it2->RGB,
					it2->brightness, it2->smokeDistanceSquared);
			for (std::list<_Influence>::iterator it3 = begin3; it3 != end3; ++it3)
				(*it1)->suck(it3->pos, it3->factor);
			for (std::list<_Influence>::iterator it4 = begin4; it4 != end4; ++it4)
				(*it1)->shock(it4->pos, it4->factor);
			for (std::list<_Influence>::iterator it5 = begin5; it5 != end5; ++it5)
				(*it1)->stretch(it5->pos, it5->factor);
		}
		_illuminationList.clear();
		_suckList.clear();
		_shockList.clear();
		_stretchList.clear();
	} else {
		stdx::call_all_ptr(_particles, &Particle::updateCameraOnly);
		std::sort(_particles.begin(), _particles.end(), ParticleSorter);
	}
	World::draw();

	glEnable(GL_BLEND);
	stdx::call_all_ptr(_particles, &Particle::draw);

	if (flares > 0.0f) {
		std::list<_Flare>::iterator begin(_flareList.begin());
		std::list<_Flare>::iterator end(_flareList.end());
		for (std::list<_Flare>::iterator it = begin; it != end; ++it)
			Flares::draw(it->x, it->y, it->RGB, it->alpha);
		begin = _superFlareList.begin();
		end = _superFlareList.end();
		for (std::list<_Flare>::iterator it = begin; it != end; ++it)
			Flares::drawSuper(it->x, it->y, it->RGB, it->alpha);
	}
	_flareList.clear();
	_superFlareList.clear();

	if (volume) Sound::update();

	Overlay::update();
	Overlay::draw();

	Common::flush();

	frameToggle = !frameToggle;
}

void Hack::stop() {
	stdx::destroy_all_ptr(pending);
	stdx::destroy_all_ptr(_particles);
}

void Hack::keyPress(char c, const KeySym&) {
	switch (c) {
	case 3: case 27:
		Common::running = false;
		break;
	case 'a': case 'A':
		_action = !_action;
		if (_action) {
			Common::speed = 1.0f;
			Overlay::set("Action continuing");
		} else {
			Common::speed = 0.0f;
			Overlay::set("Action paused");
		}
		break;
	case 'c': case 'C':
		_cameraMode = (_cameraMode == CAMERA_FIXED) ? CAMERA_AUTO : CAMERA_FIXED;
		if (_cameraMode == CAMERA_AUTO)
			Overlay::set("Automatic camera");
		else
			Overlay::set("Fixed camera");
		break;
	case 'm': case 'M':
		_cameraMode = (_cameraMode == CAMERA_MANUAL) ? CAMERA_AUTO : CAMERA_MANUAL;
		if (_cameraMode == CAMERA_AUTO) {
			Overlay::set("Automatic camera");
			_cameraEndPos = cameraPos;
			_cameraEndDir = _cameraMatInv;
			chainCamera();
		} else {
			Overlay::set("Mouse-controlled camera");
			_cameraSpeed = 100.0f;
		}
		break;
	case 'n': case 'N':
		Overlay::set("New camera");
		newCamera();
		break;
	case 's': case 'S':
		{
			static bool slowMotion = false;
			slowMotion = !slowMotion;
			if (slowMotion) {
				Common::speed = 0.125;
				Overlay::set("Slow-motion");
			} else {
				Common::speed = 1.0f;
				Overlay::set("Normal speed");
			}
		}
		break;
	case '1':
		_userDefinedExplosion = Explosion::EXPLODE_SPHERE;
		Overlay::set("Sphere of stars");
		break;
	case '2':
		_userDefinedExplosion = Explosion::EXPLODE_SPLIT_SPHERE;
		Overlay::set("Split sphere of stars");
		break;
	case '3':
		_userDefinedExplosion = Explosion::EXPLODE_MULTICOLORED_SPHERE;
		Overlay::set("Multicolored sphere of stars");
		break;
	case '4':
		_userDefinedExplosion = Explosion::EXPLODE_RING;
		Overlay::set("Ring of stars");
		break;
	case '5':
		_userDefinedExplosion = Explosion::EXPLODE_DOUBLE_SPHERE;
		Overlay::set("Double-sphere of stars");
		break;
	case '6':
		_userDefinedExplosion = Explosion::EXPLODE_SPHERE_INSIDE_RING;
		Overlay::set("Sphere of stars inside ring of stars");
		break;
	case '7':
		_userDefinedExplosion = Explosion::EXPLODE_STREAMERS;
		Overlay::set("Sphere of streamers");
		break;
	case '8':
		_userDefinedExplosion = Explosion::EXPLODE_METEORS;
		Overlay::set("Sphere of meteors");
		break;
	case '9':
		_userDefinedExplosion = Explosion::EXPLODE_STARS_INSIDE_STREAMERS;
		Overlay::set("Sphere of stars inside sphere of streamers");
		break;
	case '0':
		_userDefinedExplosion = Explosion::EXPLODE_STARS_INSIDE_METEORS;
		Overlay::set("Sphere of stars inside sphere of meteors");
		break;
	case 'q': case 'Q':
		_userDefinedExplosion = Explosion::EXPLODE_STREAMERS_INSIDE_STARS;
		Overlay::set("Sphere of streamers inside sphere of stars");
		break;
	case 'w': case 'W':
		_userDefinedExplosion = Explosion::EXPLODE_METEORS_INSIDE_STARS;
		Overlay::set("Sphere of meteors inside sphere of stars");
		break;
	case 'e': case 'E':
		_userDefinedExplosion = Explosion::EXPLODE_STAR_BOMBS;
		Overlay::set("Star bombs");
		break;
	case 'r': case 'R':
		_userDefinedExplosion = Explosion::EXPLODE_STREAMER_BOMBS;
		Overlay::set("Streamer bombs");
		break;
	case 't': case 'T':
		_userDefinedExplosion = Explosion::EXPLODE_METEOR_BOMBS;
		Overlay::set("Meteor bombs");
		break;
	case 'y': case 'Y':
		_userDefinedExplosion = Explosion::EXPLODE_CRACKER_BOMBS;
		Overlay::set("Cracker bombs");
		break;
	case 'u': case 'U':
		_userDefinedExplosion = Explosion::EXPLODE_BEES;
		Overlay::set("Bees");
		break;
	case 'i': case 'I':
		_userDefinedExplosion = Explosion::EXPLODE_FLASH;
		Overlay::set("Flash-bang");
		break;
	case 'o': case 'O':
		_userDefinedExplosion = Explosion::EXPLODE_SPINNER;
		Overlay::set("Spinner");
		break;
	case '{':
		_userDefinedExplosion = Explosion::EXPLODE_SUCKER;
		Overlay::set("Sucker and shockwave");
		break;
	case '}':
		_userDefinedExplosion = Explosion::EXPLODE_STRETCHER;
		Overlay::set("Stretcher and Big Mama");
		break;
	case '/':
		TRACE("Particles: " << _particles.size());
		break;
	}
}

void Hack::keyRelease(char, const KeySym&) {}

void Hack::pointerMotion(int x, int y) {
	_mouseX = x;
	_mouseY = y;
	_mouseIdleTime = 0.0f;
}

void Hack::buttonPress(unsigned int button) {
	switch (button) {
	case Button1:
		_leftButton = true;
		break;
	case Button2:
		_middleButton = true;
		break;
	case Button3:
		_rightButton = true;
		break;
	}
	_mouseIdleTime = 0.0f;
}

void Hack::buttonRelease(unsigned int button) {
	switch (button) {
	case Button1:
		_leftButton = false;
		break;
	case Button2:
		_middleButton = false;
		break;
	case Button3:
		_rightButton = false;
		break;
	}
	_mouseIdleTime = 0.0f;
}

void Hack::pointerEnter() {
	_mouseInWindow = true;
	_mouseIdleTime = 0.0f;
}

void Hack::pointerLeave() {
	_mouseInWindow = false;
	_mouseIdleTime = 0.0f;
}

#define _LINUX
#include "../../../addons/include/xbmc_scr_dll.h"

extern "C" {

ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

  Common::width = scrprops->width;
  Common::height = scrprops->height;
  Common::aspectRatio = float(Common::width) / float(Common::height);

  return STATUS_OK;
}

void Start()
{
  Hack::start();
}

void Render()
{
  Hack::tick();
}

void Stop()
{
  Hack::stop();
}

void Destroy()
{
}

ADDON_STATUS GetStatus()
{
  return STATUS_OK;
}

bool HasSettings()
{
  return false;
}

unsigned int GetSettings(StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS SetSetting(const char *settingName, const void *settingValue)
{
  return STATUS_OK;
}

void Remove()
{
}

void FreeSettings()
{
}

void GetInfo(SCR_INFO *info)
{
}

}
