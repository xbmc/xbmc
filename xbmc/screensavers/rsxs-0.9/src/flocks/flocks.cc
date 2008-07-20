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

#include <bug.hh>
#include <color.hh>
#include <flocks.hh>
#include <hack.hh>
#include <vector.hh>

namespace Hack {
	unsigned int numLeaders = 4;
	unsigned int numFollowers = 400;
	bool blobs = true;
	float size = 10.0f;
	unsigned int complexity = 1;
	float speed = 15.0f;
	float stretch = 20.0f;
	float colorFadeSpeed = 15.0f;
	bool chromatek = false;
	bool connections = false;
};

namespace Hack {
	enum Arguments {
		ARG_LEADERS = 1,
		ARG_FOLLOWERS,
		ARG_SIZE,
		ARG_STRETCH,
		ARG_SPEED,
		ARG_COMPLEXITY,
		ARG_COLORSPEED,
		ARG_CHROMATEK = 0x100, ARG_NO_CHROMATEK,
		ARG_CONNECTIONS = 0x200, ARG_NO_CONNECTIONS
	};

	std::vector<Leader> _leaders;
	std::vector<Follower> _followers;

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_LEADERS:
		if (Common::parseArg(arg, numLeaders, 1u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of leaders must be between 1 and 100");
		return 0;
	case ARG_FOLLOWERS:
		if (Common::parseArg(arg, numFollowers, 0u, 10000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of followers must be between 0 and 10000");
		return 0;
	case ARG_SIZE:
		if (Common::parseArg(arg, size, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"bug size must be between 1 and 100");
		return 0;
	case ARG_STRETCH:
		if (Common::parseArg(arg, stretch, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"bug stretchability must be between 0 and 100");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"bug speed must be between 1 and 100");
		return 0;
	case ARG_COMPLEXITY:
		if (Common::parseArg(arg, complexity, 0u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"camera speed must be between 0 and 10");
		blobs = complexity != 0;
		return 0;
	case ARG_COLORSPEED:
		if (Common::parseArg(arg, colorFadeSpeed, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"color fade speed must be between 0 and 100");
		return 0;
	case ARG_CHROMATEK:
		chromatek = true;
		return 0;
	case ARG_NO_CHROMATEK:
		chromatek = false;
		return 0;
	case ARG_CONNECTIONS:
		connections = true;
		return 0;
	case ARG_NO_CONNECTIONS:
		connections = false;
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Flock options:" },
		{ "leaders", ARG_LEADERS, "NUM", 0,
			"Number of leaders (1-100, default = 4)" },
		{ "followers", ARG_FOLLOWERS, "NUM", 0,
			"Number of followers (0-10000, default = 400)" },
		{ NULL, 0, NULL, 0, "Bug options:" },
		{ "size", ARG_SIZE, "NUM", 0, "Bug size (1-100, default = 10)" },
		{ "stretch", ARG_STRETCH, "NUM", 0,
			"Bug stretchability (0-100, default = 20)" },
		{ "speed", ARG_SPEED, "NUM", 0, "Bug speed (1-100, default = 15)" },
		{ "complexity", ARG_COMPLEXITY, "NUM", 0,
			"Bug complexity (1-10, 0 for lines, default = 1)" },
		{ "colorspeed", ARG_COLORSPEED, "NUM", 0,
			"Color fade speed (0-100, default = 15)" },
		{ NULL, 0, NULL, 0, "Other options:" },
		{ "chromadepth", ARG_CHROMATEK, NULL, 0, "Enable ChromaDepth mode" },
		{ "no-chromadepth", ARG_NO_CHROMATEK, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "connections", ARG_CONNECTIONS, NULL, 0, "Draw connection lines" },
		{ "no-connections", ARG_NO_CONNECTIONS, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{}
	};
	static struct argp parser =
		{ options, parse, NULL, "Draws 3D swarms of little bugs." };
	return &parser;
}

std::string Hack::getShortName() { return "flocks"; }
std::string Hack::getName()      { return "Flocks"; }

void Hack::start() {
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.0, Common::aspectRatio, 50, 2000);

	Bug::initBoundaries();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Bug::init();

	stdx::construct_n(_leaders, numLeaders);
	stdx::construct_n(_followers, numFollowers, _leaders.begin());

	colorFadeSpeed *= 0.01f;
}

void Hack::tick() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	stdx::call_all(_leaders, &Leader::update);
	stdx::call_all(_followers, &Follower::update, _leaders);

	Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.0, Common::aspectRatio, 50, 2000);

	Bug::initBoundaries();

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
