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
#include <plasma.hh>

#define NUMCONSTS 18
#define MAXTEXSIZE 1024

namespace Hack {
	float zoom = 10.0f;
	float focus = 30.0f;
	float speed = 20.0f;
	unsigned int resolution = 20;
};

namespace Hack {
	enum Arguments {
		ARG_ZOOM = 1,
		ARG_FOCUS,
		ARG_SPEED,
		ARG_RESOLUTION
	};

	float _wide;
	float _high;
	float _c[NUMCONSTS];	// constant
	float _ct[NUMCONSTS];	// temporary value of constant
	float _cv[NUMCONSTS];	// velocity of constant
	stdx::dim2<std::pair<float, float> > _position;
	stdx::dim2<RGBColor> _plasma;
	unsigned int _texSize;
	unsigned int _plasmaWidth;
	unsigned int _plasmaHeight;
	stdx::dim3<float, 3> _plasmaMap;

	float _maxDiff;
	float _texRight;
	float _texTop;

	static inline float absTrunc(float f) {
		if (f >= 0.0f)
			return f <= 1.0f ? f : 1.0f;
		else
			return f >= -1.0f ? -f : 1.0f;
	}

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_ZOOM:
		if (Common::parseArg(arg, zoom, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"magnification must be between 1 and 100");
		return 0;
	case ARG_FOCUS:
		if (Common::parseArg(arg, focus, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"plasma focus must be between 1 and 100");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"plasma speed must be between 1 and 100");
		return 0;
	case ARG_RESOLUTION:
		if (Common::parseArg(arg, resolution, 1u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"plama resolution must be between 1 and 100");
		return 0;
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Plasma options:" },
		{ "zoom", ARG_ZOOM, "NUM", 0, "Magnification (1-100, default = 10)" },
		{ "focus", ARG_FOCUS, "NUM", 0, "Plasma focus (1-100, default = 30)" },
		{ "speed", ARG_SPEED, "NUM", 0, "Plasma speed (1-100, default = 20)" },
		{ "resolution", ARG_RESOLUTION, "NUM", 0,
			"Plasma resolution (1-100, default = 20)" },
		{}
	};
	static struct argp parser =
		{ options, parse, NULL, "Draws gooey colorful plasma stuff." };
	return &parser;
}

std::string Hack::getShortName() { return "plasma"; }
std::string Hack::getName()      { return "Plasma"; }

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);

	if (Common::aspectRatio >= 1.0f) {
		_wide = 30.0f / zoom;
		_high = _wide / Common::aspectRatio;
	} else {
		_high = 30.0f / zoom;
		_wide = _high * Common::aspectRatio;
	}
/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
*/
	// Set resolution of plasma
	if (Common::aspectRatio >= 1.0f)
		_plasmaHeight = (resolution * MAXTEXSIZE) / 100;
	else
		_plasmaHeight = (unsigned int)(
			float(resolution * MAXTEXSIZE) *
			Common::aspectRatio * 0.01f
		);

	_plasmaWidth = (unsigned int)(float(_plasmaHeight) / Common::aspectRatio);

	// Set resolution of texture
	_texSize = 16;
	if (Common::aspectRatio >= 1.0f)
		while (_plasmaHeight > _texSize) _texSize *= 2;
	else
		while (_plasmaWidth > _texSize) _texSize *= 2;

	// Initialize memory and positions
	_position.resize(_plasmaWidth, _plasmaHeight);
	_plasma.resize(_plasmaWidth, _plasmaHeight);
	_plasmaMap.resize(_texSize, _texSize);
	for (unsigned int i = 0; i < _plasmaHeight; ++i)
		for (unsigned int j = 0; j < _plasmaWidth; ++j)
			_position(i, j) =
				std::make_pair(
					(i * _wide) / float(_plasmaHeight - 1) - (_wide * 0.5f),
					(j * _high) /
						(float(_plasmaHeight) / Common::aspectRatio - 1.0f) -
						(_high * 0.5f)
				);

	// Initialize constants
	for (unsigned int i = 0; i < NUMCONSTS; ++i) {
		_c[i] = 0.0f;
		_ct[i] = Common::randomFloat(M_PI * 2.0f);
		_cv[i] = Common::randomFloat(0.005f * speed) + 0.0001f;
	}

	// Make texture
	glBindTexture(GL_TEXTURE_2D, 1000);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, _texSize, _texSize, 0,
		GL_RGB, GL_FLOAT, &_plasmaMap.front());
	//glEnable(GL_TEXTURE_2D);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, _texSize);

	focus = focus / 50.0f + 0.3f;
	_maxDiff = 0.004f * speed;

	// The "- 1" cuts off right and top edges to get rid of blending to black
	_texRight = float(_plasmaHeight - 1) / float(_texSize);
	_texTop = _texRight / Common::aspectRatio;

        // Clear the error flag
        glGetError();
}

void Hack::tick() {
        int origRowLength;

	glViewport(0, 0, Common::width, Common::height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, 1000);
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &origRowLength);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, _texSize);

	// Update constants
	for(unsigned int i = 0; i < NUMCONSTS; ++i) {
		_ct[i] += _cv[i];
		if (_ct[i] > M_PI * 2.0f)
			_ct[i] -= M_PI * 2.0f;
		_c[i] = std::sin(_ct[i]) * focus;
	}

	// Update colors
	for (unsigned int i = 0; i < _plasmaHeight; ++i) {
		for (unsigned int j = 0; j < _plasmaWidth; ++j) {
			RGBColor& plasma(_plasma(i, j));
			RGBColor RGB(plasma);
			std::pair<float, float>& pos(_position(i, j));

			// Calculate vertex colors
			plasma.set(
				0.7f
					* (_c[0] * pos.first + _c[1] * pos.second
					+ _c[2] * (pos.first * pos.first + 1.0f)
					+ _c[3] * pos.first * pos.second
					+ _c[4] * RGB.g() + _c[5] * RGB.b()),
				0.7f
					* (_c[6] * pos.first + _c[7] * pos.second
					+ _c[8] * pos.first * pos.first
					+ _c[9] * (pos.second * pos.second - 1.0f)
					+ _c[10] * RGB.r() + _c[11] * RGB.b()),
				0.7f
					* (_c[12] * pos.first + _c[13] * pos.second
					+ _c[14] * (1.0f - pos.first * pos.second)
					+ _c[15] * pos.second * pos.second
					+ _c[16] * RGB.r() + _c[17] * RGB.g())
			);

			// Don't let the colors change too much
			float temp = plasma.r() - RGB.r();
			if (temp > _maxDiff)
				plasma.r() = RGB.r() + _maxDiff;
			if (temp < -_maxDiff)
				plasma.r() = RGB.r() - _maxDiff;
			temp = plasma.g() - RGB.g();
			if (temp > _maxDiff)
				plasma.g() = RGB.g() + _maxDiff;
			if (temp < -_maxDiff)
				plasma.g() = RGB.g() - _maxDiff;
			temp = plasma.b() - RGB.b();
			if (temp > _maxDiff)
				plasma.b() = RGB.b() + _maxDiff;
			if (temp < -_maxDiff)
				plasma.b() = RGB.b() - _maxDiff;

			// Put colors into texture
			_plasmaMap(i, j, 0) = absTrunc(plasma.r());
			_plasmaMap(i, j, 1) = absTrunc(plasma.g());
			_plasmaMap(i, j, 2) = absTrunc(plasma.b());
		}
	}

	// Update texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
		_plasmaWidth, _plasmaHeight, GL_RGB, GL_FLOAT, &_plasmaMap.front());

	// Draw it
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 0.0f);
		glTexCoord2f(0.0f, _texRight);
		glVertex2f(1.0f, 0.0f);
		glTexCoord2f(_texTop, 0.0f);
		glVertex2f(0.0f, 1.0f);
		glTexCoord2f(_texTop, _texRight);
		glVertex2f(1.0f, 1.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, origRowLength);

	//Common::flush();

        // Clear the error flag
        glGetError();
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

#define _LINUX
#include "../../../addons/include/xbmc_scr_dll.h"

extern "C" {

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

  Common::width = scrprops->width;
  Common::height = scrprops->height;
  Common::aspectRatio = float(Common::width) / float(Common::height);

  return ADDON_STATUS_OK;
}

void Start()
{
  Hack::start();
}

void Render()
{
  Hack::tick();
}

void ADDON_Stop()
{
  Hack::stop();
}

void ADDON_Destroy()
{
}

ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

bool ADDON_HasSettings()
{
  return false;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  return ADDON_STATUS_OK;
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

void GetInfo(SCR_INFO *info)
{
}

void Remove()
{
}

}

