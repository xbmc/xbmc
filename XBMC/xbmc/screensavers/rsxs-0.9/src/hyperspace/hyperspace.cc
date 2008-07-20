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
 * Copyright (C) 2005 Terence M. Welsh, available from www.reallyslick.com
 */
#include <common.hh>

#include <extensions.hh>
#include <flares.hh>
#include <goo.hh>
#include <hack.hh>
#include <nebula.hh>
#include <particle.hh>
#include <spline.hh>
#include <starburst.hh>
#include <tunnel.hh>

namespace Hack {
	unsigned int numStars = 1000;
	float starSize = 10.0f;
	unsigned int depth = 5;
	float fov = 50.0f;
	float speed = 10.0f;
	unsigned int resolution = 10;
	bool shaders = true;
};

namespace Hack {
	unsigned int frames;
	unsigned int current;

	float fogDepth;
	Vector camera, dir;
	float unroll;
	float lerp;

	int viewport[4];
	double projMat[16];
	double modelMat[16];
};

namespace Hack {
	enum Arguments {
		ARG_STARS = 1,
		ARG_SIZE,
		ARG_DEPTH,
		ARG_FOV,
		ARG_SPEED,
		ARG_RESOLUTION,
		ARG_SHADERS = 0x100, ARG_NO_SHADERS
	};

	std::vector<StretchedParticle> _stars;
	StretchedParticle* _sun;

	float gooFunction(const Vector&);
	float nextFrame();

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_STARS:
		if (Common::parseArg(arg, numStars, 0u, 10000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"stars must be between 0 and 10000");
		return 0;
	case ARG_SIZE:
		if (Common::parseArg(arg, starSize, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"star size must be between 1 and 100");
		return 0;
	case ARG_DEPTH:
		if (Common::parseArg(arg, depth, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"depth must be between 1 and 10");
		return 0;
	case ARG_FOV:
		if (Common::parseArg(arg, fov, 10.0f, 150.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"field of view must be between 10 and 150");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"speed must be between 1 and 100");
		return 0;
	case ARG_RESOLUTION:
		if (Common::parseArg(arg, resolution, 4u, 20u))
			argp_failure(state, EXIT_FAILURE, 0,
				"resolution must be between 4 and 20");
		return 0;
	case ARG_SHADERS:
		shaders = true;
		return 0;
	case ARG_NO_SHADERS:
		shaders = false;
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Hyperspace options:" },
		{ "stars", ARG_STARS, "NUM", 0,
			"Number of stars (0-10000, default = 1000)" },
		{ "size", ARG_SIZE, "NUM", 0,
			"Star size (1-100, default = 10)" },
		{ "depth", ARG_DEPTH, "NUM", 0, "Depth (1-10, default = 5)" },
		{ "fov", ARG_FOV, "NUM", 0, "Field of view (10-150, default = 90)" },
		{ "speed", ARG_SPEED, "NUM", 0, "Speed (1-100, default = 10)" },
		{ "resolution", ARG_RESOLUTION, "NUM", 0,
			"Resolution (4-20, default = 10)" },
		{ "shaders", ARG_SHADERS, NULL, OPTION_HIDDEN,
			"Disable shaders" },
		{ "no-shaders", ARG_NO_SHADERS, NULL, OPTION_ALIAS },
		{}
	};
	static struct argp parser =
		{ options, parse, NULL, "Soar through wormholes in liquid spacetime." };
	return &parser;
}

std::string Hack::getShortName() { return "hyperspace"; }
std::string Hack::getName()      { return "Hyperspace"; }

float Hack::nextFrame() {
	static float time = 0.0f;
	time += Common::elapsedTime;

	// loop frames every 2 seconds
	static float frameTime = 2.0f / float(frames);

	while (time > frameTime) {
		time -= frameTime;
		current++;
		if (current == frames) current = 0;
	}

	return time / frameTime;
}

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = Common::width;
	viewport[3] = Common::height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, Common::aspectRatio, 0.001f, 200.0f);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glMatrixMode(GL_MODELVIEW);

	try {
		Shaders::init();
		shaders = true;
	} catch (...) {
		shaders = false;
	}
	Hack::frames = shaders ? 20 : 60;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	Flares::init();
	Spline::init(depth * 2 + 6);
	Tunnel::init();

	fogDepth = depth * 2.0f - 2.0f / float(resolution);
	Goo::init();

	_stars.reserve(numStars);
	for (unsigned int i = 0; i < numStars; ++i) {
		RGBColor color(
			0.5f + Common::randomFloat(0.5f),
			0.5f + Common::randomFloat(0.5f),
			0.5f + Common::randomFloat(0.5f)
		);
		switch (Common::randomInt(3)) {
		case 0: color.r() = 1.0f; break;
		case 1: color.g() = 1.0f; break;
		case 2: color.b() = 1.0f; break;
		}
		_stars.push_back(StretchedParticle(
			Vector(
				Common::randomFloat(2.0f * fogDepth) - fogDepth,
				Common::randomFloat(4.0f) - 2.0f,
				Common::randomFloat(2.0f * fogDepth) - fogDepth
			),
			Common::randomFloat(starSize * 0.001f) + starSize * 0.001f,
			color, fov
		));
	}

	_sun = new StretchedParticle(
		Vector(0.0f, 2.0f, 0.0f), starSize * 0.004f,
		RGBColor(1.0f, 1.0f, 1.0f), fov
	);
	Common::resources->manage(_sun);

	StarBurst::init();
	Nebula::init();
	current = 0;

	glEnable(GL_FOG);
	float fog_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glFogfv(GL_FOG_COLOR, fog_color);
	glFogf(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, fogDepth * 0.7f);
	glFogf(GL_FOG_END, fogDepth);
}

void Hack::tick() {
	glMatrixMode(GL_MODELVIEW);

	// Camera movements
	static float heading[3] = { 0.0f, 0.0f, 0.0f }; // current, target, and last
	static bool headingChange = true;
	static float headingChangeTime[2] = { 20.0f, 0.0f }; // total, elapsed
	static float roll[3] = { 0.0f, 0.0f, 0.0f }; // current, target, and last
	static bool rollChange = true;
	static float rollChangeTime[2] = { 1.0f, 0.0f }; // total, elapsed
	headingChangeTime[1] += Common::elapsedTime;
	if( headingChangeTime[1] >= headingChangeTime[0]) { // Choose new direction
		headingChangeTime[0] = Common::randomFloat(15.0f) + 5.0f;
		headingChangeTime[1] = 0.0f;
		heading[2] = heading[1]; // last = target
		if (headingChange) {
			// face forward most of the time
			if (Common::randomInt(6))
				heading[1] = 0.0f;
			// face backward the rest of the time
			else if (Common::randomInt(2))
				heading[1] = M_PI;
			else
				heading[1] = -M_PI;
			headingChange = false;
		} else
			headingChange = true;
	}
	float t = headingChangeTime[1] / headingChangeTime[0];
	t = 0.5f * (1.0f - std::cos(M_PI * t));
	heading[0] = heading[1] * t + heading[2] * (1.0f - t);
	rollChangeTime[1] += Common::elapsedTime;
	if (rollChangeTime[1] >= rollChangeTime[0]) { // Choose new roll angle
		rollChangeTime[0] = Common::randomFloat(5.0f) + 10.0f;
		rollChangeTime[1] = 0.0f;
		roll[2] = roll[1]; // last = target
		if (rollChange) {
			roll[1] = Common::randomFloat(M_PI * 4.0f) - M_PI * 2.0f;
			rollChange = false;
		} else
			rollChange = true;
	}
	t = rollChangeTime[1] / rollChangeTime[0];
	t = 0.5f * (1.0f - std::cos(M_PI * t));
	roll[0] = roll[1] * t + roll[2] * (1.0f - t);

	Spline::moveAlongPath(speed * Common::elapsedTime * 0.03f);
	Spline::update(Common::elapsedTime);
	camera = Spline::at(depth + 2, Spline::step);
	dir = Spline::direction(depth + 2, Spline::step);
	float pathAngle = std::atan2(-dir.x(), -dir.z());

	glLoadIdentity();
	glRotatef(-roll[0] * R2D, 0, 0, 1);
	glRotatef((-pathAngle - heading[0]) * R2D, 0, 1, 0);
	glTranslatef(-camera.x(), -camera.y(), -camera.z());
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
	unroll = roll[0] * R2D;

	// calculate diagonal fov
	float diagFov = 0.5f * fov * D2R;
	diagFov = std::tan(diagFov);
	diagFov = std::sqrt(diagFov * diagFov + (diagFov * Common::aspectRatio * diagFov * Common::aspectRatio));
	diagFov = 2.0f * std::atan(diagFov);

	// clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// pick animated texture frame
	lerp = Hack::nextFrame();

	// draw stars
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, Flares::blob);
	for (
		std::vector<StretchedParticle>::iterator i = _stars.begin();
		i < _stars.end();
		++i
	) {
		i->update();
		i->draw();
	}

	// draw goo
	Goo::update(pathAngle + heading[0], diagFov);
	Goo::draw();

	// update starburst
	StarBurst::update();
	StarBurst::draw();

	// draw tunnel
	Tunnel::draw();

	// draw sun with lens flare
	glDisable(GL_FOG);
	static Vector flarePos(0.0f, 2.0f, 0.0f);
	glBindTexture(GL_TEXTURE_2D, Flares::blob);
	_sun->draw();
	float alpha = 0.5f - 0.005f * (flarePos - camera).length();
	if (alpha > 0.0f)
		Flares::draw(flarePos, RGBColor(1.0f, 1.0f, 1.0f), alpha);
	glEnable(GL_FOG);

	Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = Common::width;
	viewport[3] = Common::height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, Common::aspectRatio, 0.001f, 200.0f);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glMatrixMode(GL_MODELVIEW);
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
