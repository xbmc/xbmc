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
#include <fieldlines.hh>
#include <hack.hh>
#include <ion.hh>
#include <vector.hh>

namespace Hack {
	unsigned int numIons = 4;
	float stepSize = 15.0f;
	unsigned int maxSteps = 100;
	float width = 30.0f;
	float speed = 10.0f;
	bool constWidth = false;
	bool electric = false;
};

namespace Hack {
	enum Arguments {
		ARG_IONS = 1,
		ARG_SEGSIZE,
		ARG_MAXSEGS,
		ARG_SPEED,
		ARG_WIDTH,
		ARG_CONSTANT = 0x100, ARG_NO_CONSTANT,
		ARG_ELECTRIC = 0x200, ARG_NO_ELECTRIC
	};

	std::vector<Ion> _ions;

	static void drawFieldLine(const Ion&, float, float, float);

	error_t parse(int, char*, struct argp_state*);
};

static void Hack::drawFieldLine(const Ion& ion, float x, float y, float z) {
	static float brightness = 10000.0f;

	int charge = ion.getCharge();
	Vector lastV(ion.getV());
	Vector dir(x, y, z);

	// Do the first segment
	RGBColor RGB(
		std::abs(dir.z()) * brightness,
		std::abs(dir.x()) * brightness,
		std::abs(dir.y()) * brightness
	);
	RGB.clamp();
	RGBColor lastRGB(RGB);
	glColor3fv(RGB.get());

	Vector V(lastV + dir);
	if (electric)
		V += Vector(
			Common::randomFloat(stepSize * 0.3f) - (stepSize * 0.15f),
			Common::randomFloat(stepSize * 0.3f) - (stepSize * 0.15f),
			Common::randomFloat(stepSize * 0.3f) - (stepSize * 0.15f)
		);

	if (!constWidth)
		glLineWidth((V.z() + 300.0f) * 0.000333f * width);

	glBegin(GL_LINE_STRIP);
		glColor3fv(lastRGB.get());
		glVertex3fv(lastV.get());
		glColor3fv(RGB.get());
		glVertex3fv(V.get());

	Vector end;
	unsigned int i;
	for (i = 0; i < maxSteps; ++i) {
		dir.set(0.0f, 0.0f, 0.0f);
		for (std::vector<Ion>::const_iterator j = _ions.begin(); j != _ions.end(); ++j) {
			int repulsion = charge * j->getCharge();
			Vector temp(V - j->getV());
			float distSquared = temp.lengthSquared();
			float dist = std::sqrt(distSquared);
			if (dist < stepSize && i > 2) {
				end = j->getV();
				i = 10000;
			}
			temp /= dist;
			if (distSquared < 1.0f)
				distSquared = 1.0f;
			dir += temp * (repulsion / distSquared);
		}

		lastRGB = RGB;
		RGB.set(
			std::abs(dir.z()) * brightness,
			std::abs(dir.x()) * brightness,
			std::abs(dir.y()) * brightness
		);
		if (electric) {
			RGB *= 10.0f;
			if (RGB.r() > RGB.g() * 0.5f)
				RGB.r() = RGB.g() * 0.5f;
			if (RGB.g() > RGB.b() * 0.3f)
				RGB.g() = RGB.b() * 0.3f;
		}
		RGB.clamp();

		float distSquared = dir.lengthSquared();
		float distRec = stepSize / std::sqrt(distSquared);
		dir *= distRec;
		if (electric)
			dir += Vector(
				Common::randomFloat(stepSize) - (stepSize * 0.5f),
				Common::randomFloat(stepSize) - (stepSize * 0.5f),
				Common::randomFloat(stepSize) - (stepSize * 0.5f)
			);

		lastV = V;
		V += dir;

		if (!constWidth) {
			glEnd();
			float lineWidth = (V.z() + 300.0f) * 0.000333f * width;
			glLineWidth(lineWidth < 0.01f ? 0.01f : lineWidth);
			glBegin(GL_LINE_STRIP);
		}

		glColor3fv(lastRGB.get());
		glVertex3fv(lastV.get());

		if (i != 10000) {
			if (i == (maxSteps - 1))
				glColor3f(0.0f, 0.0f, 0.0f);
			else
				glColor3fv(RGB.get());
			glVertex3fv(V.get());
		}
	}

	if (i == 10001) {
		glColor3fv(RGB.get());
		glVertex3fv(end.get());
	}

	glEnd();
}

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_IONS:
		if (Common::parseArg(arg, numIons, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of ions must be between 1 and 10");
		return 0;
	case ARG_SEGSIZE:
		if (Common::parseArg(arg, stepSize, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"fieldline segment length must be between 1 and 100");
		return 0;
	case ARG_MAXSEGS:
		if (Common::parseArg(arg, maxSteps, 1u, 1000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"maximum number of fieldline segments must be between 1 and 1000");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"motion speed must be between 1 and 100");
		return 0;
	case ARG_WIDTH:
		if (Common::parseArg(arg, width, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"fieldline width factor must be between 1 and 100");
		return 0;
	case ARG_CONSTANT:
		constWidth = true;
		return 0;
	case ARG_NO_CONSTANT:
		constWidth = false;
		return 0;
	case ARG_ELECTRIC:
		electric = true;
		return 0;
	case ARG_NO_ELECTRIC:
		electric = false;
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Ion options:" },
		{ "ions", ARG_IONS, "NUM", 0, "Number of ions (1-10, default = 4)" },
		{ "speed", ARG_SPEED, "NUM", 0, "Motion speed (1-100, default = 10)" },
		{ NULL, 0, NULL, 0, "Fieldline options:" },
		{ "segsize", ARG_SEGSIZE, "NUM", 0,
			"Length of each fieldline segment (1-100, default = 15)" },
		{ "maxsegs", ARG_MAXSEGS, "NUM", 0,
			"Maximum number of segments per fieldline (1-1000, default = 100)" },
		{ "width", ARG_WIDTH, "NUM", 0,
			"Fieldline width factor (1-100, default = 30)" },
		{ "constant", ARG_CONSTANT, NULL, 0, "Enable constant-width fieldlines" },
		{ "no-constant", ARG_NO_CONSTANT, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "electric", ARG_ELECTRIC, NULL, 0, "Enable electric mode" },
		{ "no-electric", ARG_NO_ELECTRIC, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{}
	};
	static struct argp parser = {
		options, parse, NULL,
		"Simulates field lines between charged particles."
	};
	return &parser;
}

std::string Hack::getShortName() { return "fieldlines"; }
std::string Hack::getName()      { return "Field Lines"; }

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.0, Common::aspectRatio, 50, 3000);
	glTranslatef(0.0, 0.0, -(WIDE * 2));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (constWidth)
		glLineWidth(width * 0.1f);

	stdx::construct_n(_ions, numIons);
}

void Hack::tick() {
	static float s = std::sqrt(stepSize * stepSize * 0.333f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	stdx::call_all(_ions, &Ion::update);

	for (std::vector<Ion>::const_iterator i = _ions.begin(); i != _ions.end(); ++i) {
		drawFieldLine(*i, s, s, s);
		drawFieldLine(*i, s, s, -s);
		drawFieldLine(*i, s, -s, s);
		drawFieldLine(*i, s, -s, -s);
		drawFieldLine(*i, -s, s, s);
		drawFieldLine(*i, -s, s, -s);
		drawFieldLine(*i, -s, -s, s);
		drawFieldLine(*i, -s, -s, -s);
	}

	Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);
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
