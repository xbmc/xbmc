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

#include <blend.hh>
#include <color.hh>
#include <hack.hh>
#include <particle.hh>
#include <vector.hh>

#define WIDE 200.0f
#define HIGH 200.0f

namespace Hack {
	unsigned int numCyclones = 1;
	unsigned int numParticles = 200;
	float size = 7.0f;
	unsigned int complexity = 3;
	float speed = 10.0f;
	bool stretch = true;
	bool showCurves = false;
	bool southern = false;
};

namespace Hack {
	enum Arguments {
		ARG_CYCLONES = 1,
		ARG_PARTICLES,
		ARG_SIZE,
		ARG_COMPLEXITY,
		ARG_SPEED,
		ARG_STRETCH = 0x100, ARG_NO_STRETCH,
		ARG_CURVES = 0x200, ARG_NO_CURVES,
		ARG_SOUTH = 0x300, ARG_NORTH
	};

	std::vector<Cyclone> _cyclones;
	std::vector<Particle> _particles;

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_CYCLONES:
		if (Common::parseArg(arg, numCyclones, 1u, MAX_COMPLEXITY))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of cyclones must be between 1 and %d", MAX_COMPLEXITY);
		return 0;
	case ARG_PARTICLES:
		if (Common::parseArg(arg, numParticles, 1u, 10000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"particles per cyclones must be between 1 and 10000");
		return 0;
	case ARG_SIZE:
		if (Common::parseArg(arg, size, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"particle size must be between 1 and 100");
		return 0;
	case ARG_COMPLEXITY:
		if (Common::parseArg(arg, complexity, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"cyclone complexity must be between 1 and 10");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"particle speed must be between 1 and 100");
		return 0;
	case ARG_STRETCH:
		stretch = true;
		return 0;
	case ARG_NO_STRETCH:
		stretch = false;
		return 0;
	case ARG_CURVES:
		showCurves = true;
		return 0;
	case ARG_NO_CURVES:
		showCurves = false;
		return 0;
	case ARG_SOUTH:
		southern = true;
		return 0;
	case ARG_NORTH:
		southern = false;
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Cyclone options:" },
		{ "cyclones", ARG_CYCLONES, "NUM", 0,
			"Number of cyclones (1-10, default = 1)" },
		{ "complexity", ARG_COMPLEXITY, "NUM", 0,
			"Cyclone complexity (1-10, default = 3)" },
		{ "particles", ARG_PARTICLES, "NUM", 0,
			"Particles per cyclone (1-10000, default = 200)" },
		{ "curves", ARG_CURVES, NULL, 0,
			"Enable cyclone curves"},
		{ "no-curves", ARG_NO_CURVES, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "south", ARG_SOUTH, NULL, 0,
			"Use southern/northern hemisphere cyclones (default = north)" },
		{ "north", ARG_NORTH, NULL, OPTION_ALIAS },
		{ NULL, 0, NULL, 0, "Particle options:" },
		{ "size", ARG_SIZE, "NUM", 0,
			"Particle size (1-100, default = 7)" },
		{ "speed", ARG_SPEED, "NUM", 0,
			"Particle speed (1-100, default = 10)" },
		{ "stretch", ARG_STRETCH, NULL, OPTION_HIDDEN,
			"Disable particle stretching" },
		{ "no-stretch", ARG_NO_STRETCH, NULL, OPTION_ALIAS },
		{}
	};
	static struct argp parser =
		{ options, parse, NULL, "Simulates tornadoes on your computer screen." };
	return &parser;
}

std::string Hack::getShortName() { return "cyclone"; }
std::string Hack::getName()      { return "Cyclone"; }

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);

	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.0, Common::aspectRatio, 50, 3000);
	if (Common::randomInt(500) == 0) {
		TRACE("Easter egg view!");
		glRotatef(90, 1, 0, 0);
		glTranslatef(0.0f, WIDE * -2.0f, 0.0f);
	} else
		glTranslatef(0.0f, 0.0f, WIDE * -2.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Particle::init();

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	float ambient[4] = {0.25f, 0.25f, 0.25f, 0.0f};
	float diffuse[4] = {1.0f, 1.0f, 1.0f, 0.0f};
	float specular[4] = {1.0f, 1.0f, 1.0f, 0.0f};
	float position[4] = {WIDE * 2.0f, -HIGH, WIDE * 2.0f, 0.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glEnable(GL_COLOR_MATERIAL);
	glMaterialf(GL_FRONT, GL_SHININESS, 20.0f);
	glColorMaterial(GL_FRONT, GL_SPECULAR);
	glColor3f(0.7f, 0.7f, 0.7f);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// Initialize cyclones and their particles
	Blend::init();
	stdx::construct_n(_cyclones, numCyclones);
	_particles.reserve(numCyclones * numParticles);
	for (
		std::vector<Cyclone>::const_iterator c = _cyclones.begin();
		c != _cyclones.end();
		++c
	) stdx::construct_n(_particles, numParticles, c);
}

void Hack::tick() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	stdx::call_all(_cyclones, &Cyclone::update);
	stdx::call_all(_particles, &Particle::update);

	//Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);
}

void Hack::stop() {}

Cyclone::Cyclone() {
	// Initialize position stuff
	_V[Hack::complexity + 2].set(
		Common::randomFloat(WIDE * 2.0f - WIDE),
		HIGH,
		Common::randomFloat(WIDE * 2.0f - WIDE)
	);
	_V[Hack::complexity + 1].set(
		_V[Hack::complexity + 2].x(),
		Common::randomFloat(HIGH / 3.0f) + HIGH / 4.0f,
		_V[Hack::complexity + 2].z()
	);
	for (unsigned int i = Hack::complexity; i > 1; --i)
		_V[i].set(
			_V[i + 1].x() + Common::randomFloat(WIDE) - WIDE / 2.0f,
			Common::randomFloat(HIGH * 2.0f) - HIGH,
			_V[i + 1].z() + Common::randomFloat(WIDE) - WIDE / 2.0f
		);
	_V[1].set(
		_V[2].x() + Common::randomFloat(WIDE / 2.0f) - WIDE / 4.0f,
		-Common::randomFloat(HIGH / 2.0f) - HIGH / 4.0f,
		_V[2].z() + Common::randomFloat(WIDE / 2.0f) - WIDE / 4.0f
	);
	_V[0].set(
		_V[1].x() + Common::randomFloat(WIDE / 8.0f) - WIDE / 16.0f,
		-HIGH,
		_V[1].z() + Common::randomFloat(WIDE / 8.0f) - WIDE / 16.0f
	);

	// Initialize width stuff
	_width[Hack::complexity + 2] = Common::randomFloat(175.0f) + 75.0f;
	_width[Hack::complexity + 1] = Common::randomFloat(60.0f) + 15.0f;
	for (unsigned int i = Hack::complexity; i > 1; --i)
		_width[i] = Common::randomFloat(25.0f) + 15.0f;
	_width[1] = Common::randomFloat(25.0f) + 5.0f;
	_width[0] = Common::randomFloat(15.0f) + 5.0f;

	// Initialize transition stuff
	for (unsigned int i = 0; i < Hack::complexity + 3; ++i) {
		_VChange[i].first = _VChange[i].second = 0;
		_widthChange[i].first = _widthChange[i].second = 0;
	}

	// Initialize color stuff
	_oldHSL.set(
		Common::randomFloat(1.0f),
		Common::randomFloat(1.0f),
		0.0f
	);
	_targetHSL.set(
		Common::randomFloat(1.0f),
		Common::randomFloat(1.0f),
		1.0f
	);
	_HSLChange.first = 0;
	_HSLChange.second = 300;
}

void Cyclone::update() {
	static int speed10000 = int(10000 / Hack::speed);
	static int speed7000 = int(7000 / Hack::speed);
	static int speed5000 = int(5000 / Hack::speed);
	static int speed4000 = int(4000 / Hack::speed);
	static int speed3000 = int(3000 / Hack::speed);
	static int speed2000 = int(2000 / Hack::speed);
	static int speed1500 = int(1500 / Hack::speed);

	// update cyclone's path
	unsigned int temp = Hack::complexity + 2;
	if (_VChange[temp].first == _VChange[temp].second) {
		_oldV[temp] = _V[temp];
		_targetV[temp].set(
			Common::randomFloat(WIDE * 2.0f) - WIDE,
			HIGH,
			Common::randomFloat(WIDE * 2.0f) - WIDE
		);
		_VChange[temp].first = 0;
		_VChange[temp].second = Common::randomInt(speed10000) + speed5000;
	}

	temp = Hack::complexity + 1;
	if (_VChange[temp].first == _VChange[temp].second) {
		_oldV[temp] = _V[temp];
		_targetV[temp].set(
			_V[temp + 1].x(),
			Common::randomFloat(HIGH / 3.0f) + HIGH / 4.0f,
			_V[temp + 1].z()
		);
		_VChange[temp].first = 0;
		_VChange[temp].second = Common::randomInt(speed7000) + speed5000;
	}

	for (unsigned int i = Hack::complexity; i > 1; --i) {
		if (_VChange[i].first == _VChange[i].second) {
			_oldV[i] = _V[i];
			_targetV[i].set(
				_targetV[i + 1].x() +
					(_targetV[i + 1].x() - _targetV[i + 2].x()) / 2.0f +
					Common::randomFloat(WIDE / 2.0f) - WIDE / 4.0f,
				(_targetV[i + 1].y() + _targetV[i - 1].y()) / 2.0f +
					Common::randomFloat(HIGH / 8.0f) - HIGH / 16.0f,
				_targetV[i + 1].z() +
					(_targetV[i + 1].z() - _targetV[i + 2].z()) / 2.0f +
					Common::randomFloat(WIDE / 2.0f) - WIDE / 4.0f
			);
			_targetV[i].y() = Common::clamp(_targetV[i].y(), -HIGH, HIGH);
			_VChange[i].first = 0;
			_VChange[i].second = Common::randomInt(speed5000) + speed3000;
		}
	}
	if (_VChange[1].first == _VChange[1].second) {
		_oldV[1] = _V[1];
		_targetV[1].set(
			_targetV[2].x() + Common::randomFloat(WIDE / 2.0f) - WIDE / 4.0f,
			-Common::randomFloat(HIGH / 2.0f) - HIGH / 4.0f,
			_targetV[2].z() + Common::randomFloat(WIDE / 2.0f) - WIDE / 4.0f
		);
		_VChange[1].first = 0;
		_VChange[1].second = Common::randomInt(speed4000) + speed2000;
	}
	if (_VChange[0].first == _VChange[0].second) {
		_oldV[0] = _V[0];
		// XXX Should the following be _targetV not _V ?
		_targetV[0].set(
			_V[1].x() + Common::randomFloat(WIDE / 8.0f) - WIDE / 16.0f,
			-HIGH,
			_V[1].z() + Common::randomFloat(WIDE / 8.0f) - WIDE / 16.0f
		);
		_VChange[0].first = 0;
		_VChange[0].second = Common::randomInt(speed3000) + speed1500;
	}
	for (unsigned int i = 0; i < Hack::complexity + 3; ++i) {
		float between = float(_VChange[i].first) / float(_VChange[i].second) * M_PI * 2.0f;
		between = (1.0f - std::cos(between)) / 2.0f;
		_V[i].set(
			((_targetV[i].x() - _oldV[i].x()) * between) + _oldV[i].x(),
			((_targetV[i].y() - _oldV[i].y()) * between) + _oldV[i].y(),
			((_targetV[i].z() - _oldV[i].z()) * between) + _oldV[i].z()
		);
		++_VChange[i].first;
	}

	// Update cyclone's widths
	temp = Hack::complexity + 2;
	if (_widthChange[temp].first == _widthChange[temp].second) {
		_oldWidth[temp] = _width[temp];
		_targetWidth[temp] = Common::randomFloat(225.0f) + 75.0f;
		_widthChange[temp].first = 0;
		_widthChange[temp].second = Common::randomInt(speed5000) + speed5000;
	}
	temp = Hack::complexity + 1;
	if (_widthChange[temp].first == _widthChange[temp].second) {
		_oldWidth[temp] = _width[temp];
		_targetWidth[temp] = Common::randomFloat(80.0f) + 15.0f;
		_widthChange[temp].first = 0;
		_widthChange[temp].second = Common::randomInt(speed5000) + speed5000;
	}
	for (unsigned int i = Hack::complexity; i > 1; --i) {
		if (_widthChange[i].first == _widthChange[i].second) {
			_oldWidth[i] = _width[i];
			_targetWidth[i] = Common::randomFloat(25.0f) + 15.0f;
			_widthChange[i].first = 0;
			_widthChange[i].second = Common::randomInt(speed5000) + speed4000;
		}
	}
	if (_widthChange[1].first == _widthChange[1].second) {
		_oldWidth[1] = _width[1];
		_targetWidth[1] = Common::randomFloat(25.0f) + 5.0f;
		_widthChange[1].first = 0;
		_widthChange[1].second = Common::randomInt(speed5000) + speed3000;
	}
	if (_widthChange[0].first == _widthChange[0].second) {
		_oldWidth[0] = _width[0];
		_targetWidth[0] = Common::randomFloat(15.0f) + 5.0f;
		_widthChange[0].first = 0;
		_widthChange[0].second = Common::randomInt(speed5000) + speed2000;
	}
	for (unsigned int i = 0; i < Hack::complexity + 3; ++i) {
		float between = float(_widthChange[i].first) / float(_widthChange[i].second);
		_width[i] = ((_targetWidth[i] - _oldWidth[i]) * between) + _oldWidth[i];
		++_widthChange[i].first;
	}

	// Update cyclones color
	if (_HSLChange.first == _HSLChange.second) {
		_oldHSL = _HSL;
		_targetHSL.set(
			Common::randomFloat(1.0f),
			Common::randomFloat(1.0f),
			Common::randomFloat(1.0f) + 0.5f
		);
		_targetHSL.clamp();
		_HSLChange.first = 0;
		_HSLChange.second = Common::randomInt(1900) + 100;
	}
	float between = float(_HSLChange.first) / float(_HSLChange.second);
	float diff = _targetHSL.h() - _oldHSL.h();
	bool direction = (
		(_targetHSL.h() > _oldHSL.h() && diff > 0.5f) ||
		(_targetHSL.h() < _oldHSL.h() && diff < -0.5f)
	) && (diff > 0.5f);
	_HSL = HSLColor::tween(_oldHSL, _targetHSL, between, direction);
	++_HSLChange.first;

	if (Hack::showCurves) {
		glDisable(GL_LIGHTING);
		glColor3f(0.0f, 1.0f, 0.0f);
		glBegin(GL_LINE_STRIP);
		for (float step = 0.0; step < 1.0; step += 0.02f) {
			Vector point;
			for (unsigned int i = 0; i < Hack::complexity + 3; ++i)
				point += _V[i] * Blend::blend(i, step);
			glVertex3fv(point.get());
		}
		glEnd();
		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_LINE_STRIP);
		for (unsigned int i = 0; i < Hack::complexity + 3; ++i)
			glVertex3fv(_V[i].get());
		glEnd();
		glEnable(GL_LIGHTING);
	}
}

void Hack::keyPress(char c, const KeySym&) {
	switch (c) {
	case 3: case 27:
	case 'q': case 'Q':
		Common::running = false;
		break;
	}
}

void Hack::keyRelease(char, const KeySym&) {}
void Hack::pointerMotion(int, int) {}
void Hack::buttonPress(unsigned int) {}
void Hack::buttonRelease(unsigned int) {}
void Hack::pointerEnter() {}
void Hack::pointerLeave() {}

#define _LINUX
#include "../../../xbmc_scr.h"

extern "C" {

void Create(void* pd3dDevice, int iWidth, int iHeight, const char * szScreensaver, float pixelRatio)
{
  Common::width = iWidth;
  Common::height = iHeight;
  Common::aspectRatio = float(Common::width) / float(Common::height);
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

}
