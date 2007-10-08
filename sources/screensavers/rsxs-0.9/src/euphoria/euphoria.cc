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

#include <euphoria.hh>
#include <hack.hh>
#include <pngimage.hh>
#include <wisp.hh>

namespace Hack {
	unsigned int numWisps = 5;
	unsigned int numBackWisps = 0;
	unsigned int density = 25;
	float visibility = 35.0f;
	float speed = 15.0f;
	float feedback = 0.0f;
	float feedbackSpeed = 1.0f;
	unsigned int feedbackSize = 8;
	std::string texture;
	bool wireframe = false;
};

namespace Hack {
	enum Arguments {
		ARG_WISPS = 1,
		ARG_BACKWISPS,
		ARG_DENSITY,
		ARG_VISIBILITY,
		ARG_SPEED,
		ARG_FEEDBACK,
		ARG_FEEDBACKSPEED,
		ARG_FEEDBACKSIZE,
		ARG_TEXTURE,
		ARG_WIREFRAME = 0x100, ARG_NO_WIREFRAME,
		ARG_NO_TEXTURE = 0x200, ARG_PLASMA_TEXTURE, ARG_STRINGY_TEXTURE,
			ARG_LINES_TEXTURE, ARG_RANDOM_TEXTURE
	};

	float _fr[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float _fv[4];
	float _f[4];

	float _lr[3] = { 0.0f, 0.0f, 0.0f };
	float _lv[3];
	float _l[3];

	GLuint _tex;
	GLuint _feedbackTex;
	unsigned int _feedbackTexSize;
	stdx::dim3<GLubyte, 3> _feedbackMap;

	std::vector<Wisp> _backWisps;
	std::vector<Wisp> _wisps;

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
retry:
	switch (key) {
	case ARG_WISPS:
		if (Common::parseArg(arg, numWisps, 0u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of wisps must be between 0 and 100");
		return 0;
	case ARG_BACKWISPS:
		if (Common::parseArg(arg, numBackWisps, 0u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of background layers must be between 0 and 100");
		return 0;
	case ARG_DENSITY:
		if (Common::parseArg(arg, density, 2u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"mesh density must be between 2 and 100");
		return 0;
	case ARG_VISIBILITY:
		if (Common::parseArg(arg, visibility, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"mesh visibility must be between 1 and 100");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"motion speed must be between 1 and 100");
		return 0;
	case ARG_FEEDBACK:
		if (Common::parseArg(arg, feedback, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"feedback intensity must be between 0 and 100");
		return 0;
	case ARG_FEEDBACKSPEED:
		if (Common::parseArg(arg, feedbackSpeed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"feedback speed must be between 1 and 100");
		return 0;
	case ARG_FEEDBACKSIZE:
		if (Common::parseArg(arg, feedbackSize, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"feedback speed must be between 1 and 10");
		return 0;
	case ARG_WIREFRAME:
		wireframe = true;
		return 0;
	case ARG_NO_WIREFRAME:
		wireframe = false;
		return 0;
	case ARG_TEXTURE:
		texture = arg;
		return 0;
	case ARG_NO_TEXTURE:
		texture = "";
		return 0;
	case ARG_PLASMA_TEXTURE:
		texture = "plasma.png";
		return 0;
	case ARG_STRINGY_TEXTURE:
		texture = "stringy.png";
		return 0;
	case ARG_LINES_TEXTURE:
		texture = "lines.png";
		return 0;
	case ARG_RANDOM_TEXTURE:
		key = Common::randomInt(3) + ARG_PLASMA_TEXTURE;
		goto retry;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Global options:" },
		{ "wisps", ARG_WISPS, "NUM", 0, "Number of wisps (0-100, default = 5)" },
		{ "background", ARG_BACKWISPS, "NUM", 0,
			"Number of background layers (0-100, default = 0)" },
		{ NULL, 0, NULL, 0, "Wisp mesh options:" },
		{ "density", ARG_DENSITY, "NUM", 0,
			"Mesh density (2-100, default = 25)" },
		{ "speed", ARG_SPEED, "NUM", 0,
			"Motion speed (1-100, default = 15)" },
		{ "visibility", ARG_VISIBILITY, "NUM", 0,
			"Mesh visibility (1-100, default = 35)" },
		{ "wireframe", ARG_WIREFRAME, NULL, 0,
			"Enable wireframe mesh" },
		{ "no-wireframe", ARG_NO_WIREFRAME, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "texture", ARG_TEXTURE, "FILE", 0,
			"Wisp texture (default = no texture)" },
		{ NULL, 0, NULL, 0, "Predefined textures:" },
		{ "plain", ARG_NO_TEXTURE, NULL, 0 },
		{ "plasma", ARG_PLASMA_TEXTURE, NULL, 0 },
		{ "stringy", ARG_STRINGY_TEXTURE, NULL, 0 },
		{ "lines", ARG_LINES_TEXTURE, NULL, 0 },
		{ "random", ARG_RANDOM_TEXTURE, NULL, 0 },
		{ NULL, 0, NULL, 0, "Feedback options:" },
		{ "feedback", ARG_FEEDBACK, "NUM", 0,
			"Feedback intensity (0-100, default = 0)" },
		{ "feedbackspeed", ARG_FEEDBACKSPEED, "NUM", 0,
			"Feedback speed (1-100, default = 1)" },
		{ "feedbacksize", ARG_FEEDBACKSIZE, "NUM", 0,
			"Feedback size (1-10, default = 3)" },
		{}
	};
	static struct argp parser = {
		options, parse, NULL,
		"Draws patterned wisps, with optional psychadelic feedback."
	};
	return &parser;
}

std::string Hack::getShortName() { return "euphoria"; }
std::string Hack::getName()      { return "Euphoria"; }

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);

	// setup regular drawing area just in case feedback isn't used
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(20.0, Common::aspectRatio, 0.01, 20);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -5.0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glLineWidth(2.0f);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	_tex = 0;
	if (texture.length()) {
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		_tex = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			PNG(texture)
		);
	}

	if (feedback > 0.0f) {
		_feedbackTexSize = 1 << feedbackSize;
		while (
			(_feedbackTexSize > Common::width) ||
			(_feedbackTexSize > Common::height)
		)
			_feedbackTexSize >>= 1;

		// feedback texture setup
		glEnable(GL_TEXTURE_2D);
		_feedbackMap.resize(_feedbackTexSize, _feedbackTexSize);
		_feedbackTex = Common::resources->genTexture(
			GL_LINEAR, GL_LINEAR, GL_CLAMP, GL_CLAMP,
			3, _feedbackTexSize, _feedbackTexSize,
			GL_RGB, GL_UNSIGNED_BYTE, &_feedbackMap.front(), false
		);

		// feedback velocity variable setup
		_fv[0] = feedbackSpeed * (Common::randomFloat(0.025f) + 0.025f);
		_fv[1] = feedbackSpeed * (Common::randomFloat(0.05f) + 0.05f);
		_fv[2] = feedbackSpeed * (Common::randomFloat(0.05f) + 0.05f);
		_fv[3] = feedbackSpeed * (Common::randomFloat(0.1f) + 0.1f);
		_lv[0] = feedbackSpeed * (Common::randomFloat(0.0025f) + 0.0025f);
		_lv[1] = feedbackSpeed * (Common::randomFloat(0.0025f) + 0.0025f);
		_lv[2] = feedbackSpeed * (Common::randomFloat(0.0025f) + 0.0025f);
	}

	// Initialize wisps
	stdx::construct_n(_wisps, numWisps);
	stdx::construct_n(_backWisps, numBackWisps);
}

void Hack::tick() {
	// Update wisps
	stdx::call_all(_wisps, &Wisp::update);
	stdx::call_all(_backWisps, &Wisp::update);

	if (feedback > 0.0f) {
		static float feedbackIntensity = feedback / 101.0f;

		// update feedback variables
		for (unsigned int i = 0; i < 4; ++i) {
			_fr[i] += Common::elapsedSecs * _fv[i];
			if (_fr[i] > M_PI * 2.0f)
				_fr[i] -= M_PI * 2.0f;
		}
		_f[0] = 30.0f * std::cos(_fr[0]);
		_f[1] = 0.2f * std::cos(_fr[1]);
		_f[2] = 0.2f * std::cos(_fr[2]);
		_f[3] = 0.8f * std::cos(_fr[3]);
		for (unsigned int i = 0; i < 3; ++i) {
			_lr[i] += Common::elapsedSecs * _lv[i];
			if (_lr[i] > M_PI * 2.0f)
				_lr[i] -= M_PI * 2.0f;
			_l[i] = std::cos(_lr[i]);
			_l[i] = _l[i] * _l[i];
		}

		// Create drawing area for feedback texture
		glViewport(0, 0, _feedbackTexSize, _feedbackTexSize);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(30.0, Common::aspectRatio, 0.01f, 20.0f);
		glMatrixMode(GL_MODELVIEW);

		// Draw
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(feedbackIntensity, feedbackIntensity, feedbackIntensity);
		glBindTexture(GL_TEXTURE_2D, _feedbackTex);
		glPushMatrix();
		glTranslatef(_f[1] * _l[1], _f[2] * _l[1], _f[3] * _l[2]);
		glRotatef(_f[0] * _l[0], 0, 0, 1);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(-0.5f, -0.5f);
			glVertex3f(-Common::aspectRatio * 2.0f, -2.0f, 1.25f);
			glTexCoord2f(1.5f, -0.5f);
			glVertex3f(Common::aspectRatio * 2.0f, -2.0f, 1.25f);
			glTexCoord2f(-0.5f, 1.5f);
			glVertex3f(-Common::aspectRatio * 2.0f, 2.0f, 1.25f);
			glTexCoord2f(1.5f, 1.5f);
			glVertex3f(Common::aspectRatio * 2.0f, 2.0f, 1.25f);
		glEnd();
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, _tex);
		stdx::call_all(_backWisps, &Wisp::drawAsBackground);
		stdx::call_all(_wisps, &Wisp::draw);

		// readback feedback texture
		glReadBuffer(GL_BACK);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, _feedbackTexSize);
		glBindTexture(GL_TEXTURE_2D, _feedbackTex);
		glReadPixels(0, 0, _feedbackTexSize, _feedbackTexSize, GL_RGB,
			GL_UNSIGNED_BYTE, &_feedbackMap.front());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _feedbackTexSize, _feedbackTexSize,
			GL_RGB, GL_UNSIGNED_BYTE, &_feedbackMap.front());

		// create regular drawing area
		glViewport(0, 0, Common::width, Common::height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(20.0, Common::aspectRatio, 0.01f, 20.0f);
		glMatrixMode(GL_MODELVIEW);

		// Draw again
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(feedbackIntensity, feedbackIntensity, feedbackIntensity);
		glPushMatrix();
		glTranslatef(_f[1] * _l[1], _f[2] * _l[1], _f[3] * _l[2]);
		glRotatef(_f[0] * _l[0], 0, 0, 1);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(-0.5f, -0.5f);
			glVertex3f(-Common::aspectRatio * 2.0f, -2.0f, 1.25f);
			glTexCoord2f(1.5f, -0.5f);
			glVertex3f(Common::aspectRatio * 2.0f, -2.0f, 1.25f);
			glTexCoord2f(-0.5f, 1.5f);
			glVertex3f(-Common::aspectRatio * 2.0f, 2.0f, 1.25f);
			glTexCoord2f(1.5f, 1.5f);
			glVertex3f(Common::aspectRatio * 2.0f, 2.0f, 1.25f);
		glEnd();
		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, _tex);
	} else
		glClear(GL_COLOR_BUFFER_BIT);

	stdx::call_all(_backWisps, &Wisp::drawAsBackground);
	stdx::call_all(_wisps, &Wisp::draw);

	Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);

	// setup regular drawing area just in case feedback isn't used
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(20.0, Common::aspectRatio, 0.01, 20);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -5.0);
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
