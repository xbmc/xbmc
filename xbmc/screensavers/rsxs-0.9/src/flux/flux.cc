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
#include <flux.hh>
#include <hack.hh>
#include <trail.hh>
#include <vector.hh>

namespace Hack {
	unsigned int numFluxes = 1;
	unsigned int numTrails = 20;
	unsigned int trailLength = 40;
	GeometryType geometry = LIGHTS_GEOMETRY;
	float size = 15.0f;
	unsigned int complexity = 3;
	unsigned int randomize = 0;
	float expansion = 40.0f;
	float rotation = 30.0f;
	float wind = 20.0f;
	float instability = 20.0f;
	float blur = 0.0f;
};

namespace Hack {
	enum Arguments {
		ARG_FLUXES = 1,
		ARG_TRAILS,
		ARG_LENGTH,
		ARG_SIZE,
		ARG_COMPLEXITY,
		ARG_SPEED,
		ARG_RANDOMIZATION,
		ARG_ROTATION,
		ARG_CROSSWIND,
		ARG_INSTABILITY,
		ARG_BLUR,
		ARG_POINTS_GEOMETRY = 0x100, ARG_SPHERES_GEOMETRY, ARG_LIGHTS_GEOMETRY
	};

	std::vector<Flux> _fluxes;

	error_t parse(int, char*, struct argp_state*);
};

Flux::Flux() {
	stdx::construct_n(_trails, Hack::numTrails);
	_randomize = 1;

	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_c[i] = Common::randomFloat(2.0f) - 1.0f;
		_cv[i] = Common::randomFloat(0.000005f *
			Hack::instability * Hack::instability) +
			0.000001f * Hack::instability * Hack::instability;
	}

	_oldDistance = 0.0f;
}

void Flux::update(float cosCameraAngle, float sinCameraAngle) {
	// randomize constants
	if (Hack::randomize) {
		if (!--_randomize) {
			for (unsigned int i = 0; i < NUMCONSTS; ++i)
				_c[i] = Common::randomFloat(2.0f) - 1.0f;
			unsigned int temp = 101 - Hack::randomize;
			temp = temp * temp;
			_randomize = temp + Common::randomInt(temp);
		}
	}

	// update constants
	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_c[i] += _cv[i];
		if (_c[i] >= 1.0f) {
			_c[i] = 1.0f;
			_cv[i] = -_cv[i];
		}
		if (_c[i] <= -1.0f) {
			_c[i] = -1.0f;
			_cv[i] = -_cv[i];
		}
	}

	// update all particles in this flux field
	std::vector<Trail>::iterator j = _trails.end();
	for (std::vector<Trail>::iterator i = _trails.begin(); i != j; ++i)
		i->update(_c, cosCameraAngle, sinCameraAngle);
}

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_FLUXES:
		if (Common::parseArg(arg, numFluxes, 1u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of flux fields must be between 1 and 100");
		return 0;
	case ARG_TRAILS:
		if (Common::parseArg(arg, numTrails, 1u, 1000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"particles per flux field must be between 1 and 1000");
		return 0;
	case ARG_LENGTH:
		if (Common::parseArg(arg, trailLength, 3u, 10000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"particle trail length must be between 3 and 10000");
		return 0;
	case ARG_SIZE:
		if (Common::parseArg(arg, size, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"particle size must be between 1 and 100");
		return 0;
	case ARG_POINTS_GEOMETRY:
		geometry = POINTS_GEOMETRY;
		return 0;
	case ARG_SPHERES_GEOMETRY:
		geometry = SPHERES_GEOMETRY;
		return 0;
	case ARG_LIGHTS_GEOMETRY:
		geometry = LIGHTS_GEOMETRY;
		return 0;
	case ARG_COMPLEXITY:
		if (Common::parseArg(arg, complexity, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"sphere complexity must be between 1 and 10");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, expansion, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"expansion rate must be between 0 and 100");
		return 0;
	case ARG_RANDOMIZATION:
		if (Common::parseArg(arg, randomize, 0u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"randomization frequency must be between 0 and 100");
		return 0;
	case ARG_ROTATION:
		if (Common::parseArg(arg, rotation, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"rotation rate must be between 0 and 100");
		return 0;
	case ARG_CROSSWIND:
		if (Common::parseArg(arg, wind, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"crosswind speed must be between 0 and 100");
		return 0;
	case ARG_INSTABILITY:
		if (Common::parseArg(arg, instability, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"instability must be between 1 and 100");
		return 0;
	case ARG_BLUR:
		if (Common::parseArg(arg, blur, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"motion blur must be between 0 and 100");
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Flux options:" },
		{ "fluxes", ARG_FLUXES, "NUM", 0,
			"Number of flux fields (1-100, default = 1)" },
		{ "particles", ARG_TRAILS, "NUM", 0,
			"Particles per flux field (1-1000, default = 20)" },
		{ NULL, 0, NULL, 0, "Particle options:" },
		{ "length", ARG_LENGTH, "NUM", 0,
			"Particle trail length (3-10000, default = 40)" },
		{ "size", ARG_SIZE, "NUM", 0, "Particle size (1-100, default = 15)" },
		{ NULL, 0, NULL, 0, "Geometry options:" },
		{ "points", ARG_POINTS_GEOMETRY, NULL, 0,
			"Particle geometry (default = lights)" },
		{ "spheres", ARG_SPHERES_GEOMETRY, NULL, OPTION_ALIAS },
		{ "lights", ARG_LIGHTS_GEOMETRY, NULL, OPTION_ALIAS },
		{ "complexity", ARG_COMPLEXITY, "NUM", 0,
			"Sphere complexity (1-10, default = 3)" },
		{ NULL, 0, NULL, 0, "Other options:" },
		{ "speed", ARG_SPEED, "NUM", 0, "Expansion rate (0-100, default = 40)" },
		{ "randomness", ARG_RANDOMIZATION, "NUM", 0,
			"Randomization frequency (0-100, default = 0)" },
		{ "rotation", ARG_ROTATION, "NUM", 0, "Rotation rate (0-100, default = 30)" },
		{ "wind", ARG_CROSSWIND, "NUM", 0,
			"Crosswind speed (0-100, default = 20)" },
		{ "instability", ARG_INSTABILITY, "NUM", 0,
			"Instability (1-100, default = 20)" },
		{ "blur", ARG_BLUR, "NUM", 0, "Motion blur (0-100, default = 0)" },
		{}
	};
	static struct argp parser = {
		options, parse, NULL,
		"Draws a particle system based on strange attractor equations."
	};
	return &parser;
}

std::string Hack::getShortName() { return "flux"; }
std::string Hack::getName()      { return "Flux"; }

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(100.0, Common::aspectRatio, 0.01, 200);
	glTranslatef(0.0, 0.0, -2.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (geometry == POINTS_GEOMETRY)
		glEnable(GL_POINT_SMOOTH);

	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Trail::init();

	// Initialize flux fields
	stdx::construct_n(_fluxes, numFluxes);
}

void Hack::tick() {
	// clear the screen
	glLoadIdentity();
	if (blur) {	// partially
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f -
			float(std::sqrt(std::sqrt(blur))) * 0.15495f);
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(-5.0f, -4.0f, 0.0f);
			glVertex3f(5.0f, -4.0f, 0.0f);
			glVertex3f(-5.0f, 4.0f, 0.0f);
			glVertex3f(5.0f, 4.0f, 0.0f);
		glEnd();
	} else	// completely
		glClear(GL_COLOR_BUFFER_BIT);

	static float cameraAngle = 0.0f;

	cameraAngle += 0.01f * rotation;
	if (cameraAngle >= 360.0f)
		cameraAngle -= 360.0f;

	float cosCameraAngle = 0.0f, sinCameraAngle = 0.0f;
	// set up blend modes for rendering particles
	switch (geometry) {
	case POINTS_GEOMETRY:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glEnable(GL_BLEND);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		cosCameraAngle = std::cos(cameraAngle * D2R);
		sinCameraAngle = std::sin(cameraAngle * D2R);
		break;
	case SPHERES_GEOMETRY:
		glRotatef(cameraAngle, 0.0f, 1.0f, 0.0f);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		break;
	case LIGHTS_GEOMETRY:
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		cosCameraAngle = std::cos(cameraAngle * D2R);
		sinCameraAngle = std::sin(cameraAngle * D2R);
		break;
	}

	// Update particles
	std::vector<Flux>::iterator j = _fluxes.end();
	for (std::vector<Flux>::iterator i = _fluxes.begin(); i != j; ++i)
		i->update(cosCameraAngle, sinCameraAngle);

	Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(100.0, Common::aspectRatio, 0.01, 200);
	glTranslatef(0.0, 0.0, -2.5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Hack::stop() {}

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
