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
#include <hack.hh>
#include <solarwinds.hh>
#include <vector.hh>
#include <wind.hh>

namespace Hack {
	unsigned int numWinds = 1;
	unsigned int numEmitters = 30;
	unsigned int numParticles = 2000;
	GeometryType geometry = LIGHTS_GEOMETRY;
	float size = 50.0f;
	float windSpeed = 20.0f;
	float emitterSpeed = 15.0f;
	float particleSpeed = 10.0f;
	float blur = 40.0f;
};

namespace Hack {
	enum Arguments {
		ARG_WINDS = 1,
		ARG_PARTICLES,
		ARG_EMITTERS,
		ARG_SIZE,
		ARG_SPEED,
		ARG_EMITTERSPEED,
		ARG_WINDSPEED,
		ARG_BLUR,
		ARG_LIGHTS_GEOMETRY = 0x100, ARG_POINTS_GEOMETRY, ARG_LINES_GEOMETRY
	};

	std::vector<Wind> _winds;

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_WINDS:
		if (Common::parseArg(arg, numWinds, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of solar winds must be between 1 and 10");
		return 0;
	case ARG_PARTICLES:
		if (Common::parseArg(arg, numParticles, 1u, 10000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"particles per wind must be between 1 and 10000");
		return 0;
	case ARG_EMITTERS:
		if (Common::parseArg(arg, numEmitters, 1u, 1000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"emitters per wind must be between 1 and 1000");
		return 0;
	case ARG_SIZE:
		if (Common::parseArg(arg, size, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"particle size must be between 1 and 100");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, particleSpeed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"particle speed must be between 1 and 100");
		return 0;
	case ARG_EMITTERSPEED:
		if (Common::parseArg(arg, emitterSpeed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"emitter speed must be between 1 and 100");
		return 0;
	case ARG_WINDSPEED:
		if (Common::parseArg(arg, windSpeed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"wind speed must be between 1 and 100");
		return 0;
	case ARG_BLUR:
		if (Common::parseArg(arg, blur, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"motion blur must be between 1 and 100");
		return 0;
	case ARG_LIGHTS_GEOMETRY:
		geometry = LIGHTS_GEOMETRY;
		return 0;
	case ARG_POINTS_GEOMETRY:
		geometry = POINTS_GEOMETRY;
		return 0;
	case ARG_LINES_GEOMETRY:
		geometry = LINES_GEOMETRY;
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Wind options:" },
		{ "winds", ARG_WINDS, "NUM", 0, "Number of solar winds (1-10, default = 1)" },
		{ "particles", ARG_PARTICLES, "NUM", 0,
			"Particles per wind (1-10000, default = 2000)" },
		{ "emitters", ARG_EMITTERS, "NUM", 0,
			"Emitters per wind (1-1000, default = 30)" },
		{ "windspeed", ARG_WINDSPEED, "NUM", 0, "Wind speed (1-100, default = 20)" },
		{ NULL, 0, NULL, 0, "Particle options:" },
		{ "size", ARG_SIZE, "NUM", 0, "Particle size (1-100, default = 50)" },
		{ "lights", ARG_LIGHTS_GEOMETRY, NULL, 0,
			"Particle geometry (default = lights)" },
		{ "points", ARG_POINTS_GEOMETRY, NULL, OPTION_ALIAS },
		{ "lines", ARG_LINES_GEOMETRY, NULL, OPTION_ALIAS },
		{ "speed", ARG_SPEED, "NUM", 0, "Particle speed (1-100, default = 10)" },
		{ NULL, 0, NULL, 0, "Emitter options:" },
		{ "emitterspeed", ARG_EMITTERSPEED, "NUM", 0,
			"Emitter speed (1-100, default = 15)" },
		{ NULL, 0, NULL, 0, "Other options:" },
		{ "blur", ARG_BLUR, "NUM", 0, "Motion blur (0-100, default = 40)" },
		{}
	};
	static struct argp parser = {
		options, parse, NULL, "Another color and movement particle effect."
	};
	return &parser;
}

std::string Hack::getShortName() { return "solarwinds"; }
std::string Hack::getName()      { return "Solar Winds"; }

void Hack::start() {
/*
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, Common::aspectRatio, 1.0, 10000);
	glTranslatef(0.0, 0.0, -15.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
*/

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (geometry == LIGHTS_GEOMETRY)
		glBlendFunc(GL_ONE, GL_ONE);
	else
		// Necessary for point and line smoothing (I don't know why)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	Wind::init();

	// Initialize surfaces
	_winds.resize(numWinds);

        // Clear the GL error 
        glGetError();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, Common::aspectRatio, 1.0, 10000);
	glTranslatef(0.0, 0.0, -15.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Hack::tick() {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(90.0, Common::aspectRatio, 1.0, 10000);
        glTranslatef(0.0, 0.0, -15.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (!blur) {
		glClear(GL_COLOR_BUFFER_BIT);
	} else {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f - (blur * 0.0049f));
		glBegin(GL_QUADS);
			glVertex3f(-40.0f, -17.0f, 0.0f);
			glVertex3f(40.0f, -17.0f, 0.0f);
			glVertex3f(40.0f, 17.0f, 0.0f);
			glVertex3f(-40.0f, 17.0f, 0.0f);
		glEnd();
		if (geometry == LIGHTS_GEOMETRY)
			glBlendFunc(GL_ONE, GL_ONE);
		else
			// Necessary for point and line smoothing (I don't know why)
			// Maybe it's just my video card...
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}

	// Update surfaces
	stdx::call_all(_winds, &Wind::update);

	// Common::flush();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        // Clear the GL error 
        glGetError();
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
  Common::resources = new ResourceManager;
  Hack::start();
}

void Render()
{
  Hack::tick();
}

void Stop()
{
  Hack::stop();
  delete Common::resources;
}

}

