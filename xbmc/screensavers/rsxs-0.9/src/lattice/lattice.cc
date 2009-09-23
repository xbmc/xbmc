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

#include <camera.hh>
#include <common.hh>
#include <hack.hh>
#include <lattice.hh>
#include <resources.hh>
#include <vector.hh>

// Important: See XXX below
#define LATSIZE 10u

namespace Hack {
	unsigned int longitude = 16;
	unsigned int latitude = 8;
	float thickness = 50.0f;
	unsigned int density = 50;
	unsigned int depth = 4;
	float fov = 90.0;
	unsigned int pathRand = 7;
	float speed = 10.0f;
	LinkType linkType = UNKNOWN_LINKS;
	std::vector<Texture> textures;
	bool smooth = false;
	bool fog = true;
	bool widescreen = false;
};

namespace Hack {
	enum Arguments {
		ARG_LATITUDE = 1,
		ARG_LONGITUDE,
		ARG_THICKNESS,
		ARG_DENSITY,
		ARG_DEPTH,
		ARG_FOV,
		ARG_RANDOMNESS,
		ARG_SPEED,
		ARG_SHININESS,
		ARG_PLAIN,
		ARG_TEXTURE,
		ARG_SMOOTH = 0x100, ARG_NO_SMOOTH,
		ARG_FOG = 0x200, ARG_NO_FOG,
		ARG_WIDESCREEN = 0x300, ARG_NO_WIDESCREEN,
		ARG_INDUSTRIAL_TEXTURE = 0x400, ARG_CRYSTAL_TEXTURE,
			ARG_CHROME_TEXTURE, ARG_BRASS_TEXTURE,
			ARG_SHINY_TEXTURE, ARG_GHOSTLY_TEXTURE,
			ARG_CIRCUITS_TEXTURE, ARG_DOUGHNUTS_TEXTURE,
			ARG_RANDOM_TEXTURE,
		ARG_SOLID_LINKS = 0x500, ARG_TRANSLUCENT_LINKS, ARG_HOLLOW_LINKS,
		ARG_SPHEREMAP = 0x600, ARG_NO_SPHEREMAP,
		ARG_COLORED = 0x700, ARG_NO_COLORED,
		ARG_MODULATE = 0x800, ARG_NO_MODULATE
	};

	GLuint _lattice[LATSIZE][LATSIZE][LATSIZE];

	// Border points and direction vectors where camera can cross
	// from cube to cube
	const float _bPnt[][6] = {
		{  0.5f , -0.25f,  0.25f,  1.0f ,  0.0f ,  0.0f },
		{  0.5f ,  0.25f, -0.25f,  1.0f ,  0.0f ,  0.0f },
		{ -0.25f,  0.5f ,  0.25f,  0.0f ,  1.0f ,  0.0f },
		{  0.25f,  0.5f , -0.25f,  0.0f ,  1.0f ,  0.0f },
		{ -0.25f, -0.25f,  0.5f ,  0.0f ,  0.0f ,  1.0f },
		{  0.25f,  0.25f,  0.5f ,  0.0f ,  0.0f ,  1.0f },
		{  0.5f , -0.5f , -0.5f ,  1.0f , -1.0f , -1.0f },
		{  0.5f ,  0.5f , -0.5f ,  1.0f ,  1.0f , -1.0f },
		{  0.5f , -0.5f ,  0.5f ,  1.0f , -1.0f ,  1.0f },
		{  0.5f ,  0.5f ,  0.5f ,  1.0f ,  1.0f ,  1.0f }
	};
	float _path[7][6];
	const unsigned int _transitions[][6] = {
		{  1,  2, 12,  4, 14,  8 },
		{  0,  3, 15,  7,  7,  7 },
		{  3,  4, 14,  0,  7, 16 },
		{  2,  1, 15,  7,  7,  7 },
		{  5, 10, 12, 17, 17, 17 },
		{  4,  3, 13, 11,  9, 17 },
		{ 12,  4, 10, 17, 17, 17 },
		{  2,  0, 14,  8, 16, 19 },
		{  1,  3, 15,  7,  7,  7 },
		{  4, 10, 12, 17, 17, 17 },
		{ 11,  4, 12, 17, 17, 17 },
		{ 10,  5, 15, 13, 17, 18 },
		{ 13, 10,  4, 17, 17, 17 },
		{ 12,  1, 11,  5,  6, 17 },
		{ 15,  2, 12,  0,  7, 19 },
		{ 14,  3,  1,  7,  7,  7 },
		{  3,  1, 15,  7,  7,  7 },
		{  5, 11, 13,  6,  9, 18 },
		{ 10,  4, 12, 17, 17, 17 },
		{ 15,  1,  3,  7,  7,  7 }
	};
	int _globalXYZ[3];
	unsigned int _lastBorder;
	unsigned int _segments;

	unsigned int latticeMod(int);
	float interpolate(float, float, float, float, float);
	void reconfigure();

	void setLinkType(LinkType);
	error_t parse(int, char*, struct argp_state*);
};

// Modulus function for picking the correct element of lattice array
unsigned int Hack::latticeMod(int x) {
	if (x < 0)
		return (LATSIZE - (-x % LATSIZE)) % LATSIZE;
	else
		return x % LATSIZE;
}

// start point, start slope, end point, end slope, position (0.0 - 1.0)
// returns point somewhere along a smooth curve between the start point
// and end point
float Hack::interpolate(float a, float b, float c, float d, float where) {
	float q = 2.0f * (a - c) + b + d;
	float r = 3.0f * (c - a) - 2.0f * b - d;
	return (where * where * where * q) + (where * where * r) + (where * b) + a;
}

void Hack::reconfigure() {
	// End of old path = start of new path
	for (unsigned int i = 0; i < 6; ++i)
		_path[0][i] = _path[_segments][i];

	// determine if direction of motion is positive or negative
	// update global position
	bool positive;
	if (_lastBorder < 6) {
		if ((_path[0][3] + _path[0][4] + _path[0][5]) > 0.0f) {
			positive = true;
			++_globalXYZ[_lastBorder / 2];
		} else {
			positive = false;
			--_globalXYZ[_lastBorder / 2];
		}
	} else {
		if (_path[0][3] > 0.0f) {
			positive = true;
			++_globalXYZ[0];
		} else {
			positive = false;
			--_globalXYZ[0];
		}
		if (_path[0][4] > 0.0f)
			++_globalXYZ[1];
		else
			--_globalXYZ[1];
		if (_path[0][5] > 0.0f)
			++_globalXYZ[2];
		else
			--_globalXYZ[2];
	}

	if (!Common::randomInt(11 - pathRand)) {	// Change directions
		if (!positive)
			_lastBorder += 10;
		unsigned int newBorder = _transitions[_lastBorder][Common::randomInt(6)];
		positive = false;
		if (newBorder < 10)
			positive = true;
		else
			newBorder -= 10;
		for (unsigned int i = 0; i < 6; ++i)	// set the new border point
			_path[1][i] = _bPnt[newBorder][i];
		if (!positive) {	// flip everything if direction is negative
			if (newBorder < 6)
				_path[1][newBorder / 2] *= -1.0f;
			else
				for (unsigned int i = 0; i < 3; ++i)
					_path[1][i] *= -1.0f;
			for (unsigned int i = 3; i < 6; ++i)
				_path[1][i] *= -1.0f;
		}
		for (unsigned int i = 0; i < 3; ++i)	// reposition the new border
			_path[1][i] += _globalXYZ[i];
		_lastBorder = newBorder;
		_segments = 1;
	} else {	// Just keep going straight
		unsigned int newBorder = _lastBorder;
		for (unsigned int i = 0; i < 6; ++i)
			_path[1][i] = _bPnt[newBorder][i];
		unsigned int i = newBorder / 2;
		if (!positive) {
			if (newBorder < 6)
				_path[1][i] *= -1.0f;
			else {
				_path[1][0] *= -1.0f;
				_path[1][1] *= -1.0f;
				_path[1][2] *= -1.0f;
			}
			_path[1][3] *= -1.0f;
			_path[1][4] *= -1.0f;
			_path[1][5] *= -1.0f;
		}
		for (unsigned int j = 0; j < 3; ++j) {
			_path[1][j] += _globalXYZ[j];
			if ((newBorder < 6) && (j != 1))
				_path[1][j] += Common::randomFloat(0.15f) - 0.075f;
		}
		if (newBorder >= 6)
			_path[1][0] += Common::randomFloat(0.1f) - 0.05f;
		_segments = 1;
	}
}

void Hack::setLinkType(LinkType lt) {
	if (linkType == lt)
		return;
	if (linkType != UNKNOWN_LINKS) {
		static char* linkTypeOption[3] = { "--solid", "--translucent", "--hollow" };
		WARN("Overriding " << linkTypeOption[linkType] << " with " <<
			linkTypeOption[lt]);
	}
	linkType = lt;
}

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	static float shininess = 50.0f;
	static bool sphereMap = false;
	static bool colored = true;
	static bool modulate = true;

retry:
	switch (key) {
	case ARG_LATITUDE:
		if (Common::parseArg(arg, latitude, 2u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"latitudinal divisions must be between 2 and 100");
		return 0;
	case ARG_LONGITUDE:
		if (Common::parseArg(arg, longitude, 4u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"longitudinal divisions must be between 4 and 100");
		return 0;
	case ARG_THICKNESS:
		if (Common::parseArg(arg, thickness, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"torus thickness must be between 1 and 100");
		return 0;
	case ARG_DENSITY:
		if (Common::parseArg(arg, density, 1u, 100u))
			argp_failure(state, EXIT_FAILURE, 0,
				"lattice density must be between 1 and 100");
		return 0;
	case ARG_DEPTH:
		if (Common::parseArg(arg, depth, 1u, LATSIZE - 2))
			argp_failure(state, EXIT_FAILURE, 0,
				"lattice depth must be between 1 and %d", LATSIZE - 2);
		return 0;
	case ARG_FOV:
		if (Common::parseArg(arg, fov, 10.0f, 150.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"field of view must be between 10 and 150");
		return 0;
	case ARG_RANDOMNESS:
		if (Common::parseArg(arg, pathRand, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"path randomness must be between 1 and 10");
		return 0;
	case ARG_SPEED:
		speed = std::strtod(arg, NULL);
		if (speed < 1.0f || speed > 100.0f)
			argp_failure(state, EXIT_FAILURE, 0,
				"camera speed must be between 1 and 100");
		return 0;
	case ARG_SHININESS:
		if (Common::parseArg(arg, shininess, 0.0f, 128.0f, -1.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"shininess must be -1 (to disable lighting), or between 0 and 128");
		return 0;
	case ARG_PLAIN:
		{
			Texture texture = { "", shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_TEXTURE:
		{
			Texture texture = { arg, shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_SMOOTH:
		smooth = true;
		return 0;
	case ARG_NO_SMOOTH:
		smooth = false;
		return 0;
	case ARG_FOG:
		fog = true;
		return 0;
	case ARG_NO_FOG:
		fog = false;
		return 0;
	case ARG_WIDESCREEN:
		widescreen = true;
		return 0;
	case ARG_NO_WIDESCREEN:
		widescreen = false;
		return 0;
	case ARG_INDUSTRIAL_TEXTURE:
		setLinkType(SOLID_LINKS);
		shininess = 0.0f;
		sphereMap = false;
		colored = false;
		modulate = true;
		{
			Texture texture = { "industrial1.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		{
			Texture texture = { "industrial2.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_CRYSTAL_TEXTURE:
		setLinkType(TRANSLUCENT_LINKS);
		shininess = 10.0f;
		sphereMap = true;
		colored = false;
		modulate = true;
		{
			Texture texture = { "crystal.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_CHROME_TEXTURE:
		setLinkType(SOLID_LINKS);
		shininess = -1.0f;
		sphereMap = true;
		colored = false;
		modulate = true;
		{
			Texture texture = { "chrome.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_BRASS_TEXTURE:
		setLinkType(SOLID_LINKS);
		shininess = -1.0f;
		sphereMap = true;
		colored = false;
		modulate = true;
		{
			Texture texture = { "brass.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_SHINY_TEXTURE:
		setLinkType(SOLID_LINKS);
		shininess = 50.0f;
		sphereMap = true;
		colored = true;
		modulate = false;
		{
			Texture texture = { "shiny.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_GHOSTLY_TEXTURE:
		setLinkType(TRANSLUCENT_LINKS);
		shininess = -1.0f;
		sphereMap = true;
		colored = true;
		modulate = true;
		{
			Texture texture = { "ghostly.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_CIRCUITS_TEXTURE:
		setLinkType(HOLLOW_LINKS);
		shininess = 50.0f;
		sphereMap = false;
		colored = true;
		modulate = true;
		{
			Texture texture = { "circuits.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_DOUGHNUTS_TEXTURE:
		setLinkType(SOLID_LINKS);
		shininess = 50.0f;
		sphereMap = false;
		colored = true;
		modulate = false;
		{
			Texture texture = { "doughnuts.png",
				shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
		return 0;
	case ARG_RANDOM_TEXTURE:
		key = Common::randomInt(8) + ARG_INDUSTRIAL_TEXTURE;
		goto retry;
	case ARG_SOLID_LINKS:
		setLinkType(SOLID_LINKS);
		return 0;
	case ARG_TRANSLUCENT_LINKS:
		setLinkType(TRANSLUCENT_LINKS);
		return 0;
	case ARG_HOLLOW_LINKS:
		setLinkType(HOLLOW_LINKS);
		return 0;
	case ARG_SPHEREMAP:
		sphereMap = true;
		return 0;
	case ARG_NO_SPHEREMAP:
		sphereMap = false;
		return 0;
	case ARG_COLORED:
		colored = true;
		return 0;
	case ARG_NO_COLORED:
		colored = false;
		return 0;
	case ARG_MODULATE:
		modulate = true;
		return 0;
	case ARG_NO_MODULATE:
		modulate = false;
		return 0;
	case ARGP_KEY_FINI:
		if (linkType == UNKNOWN_LINKS)
			linkType = SOLID_LINKS;
		if (textures.empty()) {
			Texture texture = { "", shininess, sphereMap, colored, modulate };
			textures.push_back(texture);
		}
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Lattice options:" },
		{ "density", ARG_DENSITY, "NUM", 0, "Lattice density (1-100, default = 50)" },
		// XXX 8 = LATSIZE - 2
		{ "depth", ARG_DEPTH, "NUM", 0, "Lattice depth (1-8, default = 4)" },
		{ "fog", ARG_FOG, NULL, OPTION_HIDDEN, "Disable depth fogging" },
		{ "no-fog", ARG_NO_FOG, NULL, OPTION_ALIAS },
		{ NULL, 0, NULL, 0, "Torus options:" },
		{ "latitude", ARG_LATITUDE, "NUM", 0,
			"Latitudinal divisions on each torus (2-100, default = 8)" },
		{ "longitude", ARG_LONGITUDE, "NUM", 0,
			"Longitudinal divisions on each torus (4-100, default = 16)" },
		{ "thickness", ARG_THICKNESS, "NUM", 0,
			"Thickness of each torus (1-100, default = 50)" },
		{ "smooth", ARG_SMOOTH, NULL, 0, "Enable smooth shading" },
		{ "no-smooth", ARG_NO_SMOOTH, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ NULL, 0, NULL, 0, "Rendering options:" },
		{ "solid", ARG_SOLID_LINKS, NULL, 0,
			"Surface rendering mode (default = solid)" },
		{ "translucent", ARG_TRANSLUCENT_LINKS, NULL, OPTION_ALIAS },
		{ "hollow", ARG_HOLLOW_LINKS, NULL, OPTION_ALIAS },
		{ "These options define the global rendering scheme for the surfaces of the"
			" toruses. If more than one of these options are specified, the last"
			" will take precedence.", 0, NULL, OPTION_DOC | OPTION_NO_USAGE },
		{ NULL, 0, NULL, 0, "Surface options:" },
		{ "shininess", ARG_SHININESS, "NUM", 0,
			"Degree of specular reflection (0-128, -1 to disable lighting, default"
			" = 50)" },
		{ "spheremap", ARG_SPHEREMAP, NULL, 0,
			"Enable environment mapping of surface texture" },
		{ "no-spheremap", ARG_NO_SPHEREMAP, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "colored", ARG_COLORED, NULL, 0,
			"Colorize (or whiten) the surfaces (default = colored)" },
		{ "white", ARG_NO_COLORED, NULL, OPTION_ALIAS },
		{ "no-colored", ARG_NO_COLORED, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "no-white", ARG_COLORED, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "modulate", ARG_MODULATE, NULL, 0,
			"Modulate the surface color with the texture, or paste the texture over"
			" the color as a decal (default = modulate)" },
		{ "decal", ARG_NO_MODULATE, NULL, OPTION_ALIAS },
		{ "no-modulate", ARG_NO_MODULATE, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "no-decal", ARG_MODULATE, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ NULL, 0, NULL, 0, "" },
		{ "plain", ARG_PLAIN, NULL, 0, "Plain surface" },
		{ "texture", ARG_TEXTURE, "FILE", 0, "PNG image surface" },
		{ "Any of these options may be specified multiple times. Each --plain and"
			" --texture option defines a group of toruses that are rendered in the"
			" same manner. Any --shininess, --solid, --translucent, --hollow and"
			" --spheremap options must precede the --plain or --texture they are"
			" to affect.", 0, NULL, OPTION_DOC | OPTION_NO_USAGE },
		{ NULL, 0, NULL, 0, "Predefined surfaces:" },
		{ "random", ARG_RANDOM_TEXTURE, NULL, 0,
			"Randomly choose a predefined surface" },
		{ NULL, 0, NULL, 0, "" },
		{ "industrial", ARG_INDUSTRIAL_TEXTURE, NULL, 0 },
		{ "crystal", ARG_CRYSTAL_TEXTURE, NULL, 0 },
		{ "chrome", ARG_CHROME_TEXTURE, NULL, 0 },
		{ "brass", ARG_BRASS_TEXTURE, NULL, 0 },
		{ "shiny", ARG_SHINY_TEXTURE, NULL, 0 },
		{ "ghostly", ARG_GHOSTLY_TEXTURE, NULL, 0 },
		{ "circuits", ARG_CIRCUITS_TEXTURE, NULL, 0 },
		{ "donuts", ARG_DOUGHNUTS_TEXTURE, NULL, 0 },
		{ "doughnuts", ARG_DOUGHNUTS_TEXTURE, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "Each of these selects one of the rendering options (--solid,"
			" --translucent, and --hollow) and defines one or more torus groups.",
			0, NULL, OPTION_DOC | OPTION_NO_USAGE },
		{ NULL, 0, NULL, 0, "View options:" },
		{ "fov", ARG_FOV, "NUM", 0, "Field of view (10-150, default = 90)" },
		{ "randomness", ARG_RANDOMNESS, "NUM", 0,
			"Path randomness (1-10, default = 7)" },
		{ "speed", ARG_SPEED, "NUM", 0, "Camera speed (1-100, default = 10)" },
		{ "widescreen", ARG_WIDESCREEN, NULL, 0, "Enable widescreen view" },
		{ "no-widescreen", ARG_NO_WIDESCREEN, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{}
	};
	static struct argp parser = {
		options, parse, NULL,
		"Fly through an infinite lattice of interlocking rings."
	};
	return &parser;
}

std::string Hack::getShortName() { return "lattice"; }
std::string Hack::getName()      { return "Lattice"; }

void Hack::start() {
	if (widescreen)
		glViewport(
			0, Common::height / 2 - Common::width / 4,
			Common::width, Common::width / 2
		);
	else
		glViewport(0, 0, Common::width, Common::height);

	Resources::init();

	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float mat[16] = {
		std::cos(fov * 0.5f * D2R) / std::sin(fov * 0.5f * D2R), 0.0f, 0.0f, 0.0f,
		0.0f, 0.0, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f - 0.02f / float(depth), -1.0f,
		0.0f, 0.0f, -(0.02f + 0.0002f / float(depth)), 0.0f
	};
	if (widescreen)
		mat[5] = mat[0] * 2.0f;
	else
		mat[5] = mat[0] * Common::aspectRatio;
	glLoadMatrixf(mat);
	Camera::set(mat, float(depth));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (fog) {
		glEnable(GL_FOG);
		float fog_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		glFogfv(GL_FOG_COLOR, fog_color);
		glFogf(GL_FOG_MODE, GL_LINEAR);
		glFogf(GL_FOG_START, float(depth) * 0.3f);
		glFogf(GL_FOG_END, float(depth) - 0.1f);
	}

	// Initialize lattice objects and their positions in the lattice array
	for (unsigned int i = 0; i < LATSIZE; ++i)
		for (unsigned int j = 0; j < LATSIZE; ++j)
			for (unsigned int k = 0; k < LATSIZE; ++k)
				_lattice[i][j][k] = Resources::lists + Common::randomInt(NUMOBJECTS);

	_globalXYZ[0] = 0;
	_globalXYZ[1] = 0;
	_globalXYZ[2] = 0;

	// Set up first path section
	_path[0][0] = 0.0f;
	_path[0][1] = 0.0f;
	_path[0][2] = 0.0f;
	_path[0][3] = 0.0f;
	_path[0][4] = 0.0f;
	_path[0][5] = 0.0f;
	unsigned int j = Common::randomInt(12);
	unsigned int k = j % 6;
	for (unsigned int i = 0; i < 6; ++i)
		_path[1][i] = _bPnt[k][i];
	if (j > 5) {	// If we want to head in a negative direction
		unsigned int i = k / 2;	// then we need to flip along the appropriate axis
		_path[1][i] *= -1.0f;
		_path[1][i + 3] *= -1.0f;
	}
	_lastBorder = k;
	_segments = 1;
}

void Hack::tick() {
	static float where = 0.0f;	// Position on path
	static unsigned int seg = 0;	// Section of path
	where += speed * 0.05f * Common::elapsedSecs;
	if (where >= 1.0f) {
		where -= 1.0f;
		seg++;
	}
	if (seg >= _segments) {
		seg = 0;
		reconfigure();
	}

	static Vector oldXYZ(0.0f, 0.0f, 0.0f);
	static UnitVector oldDir(Vector(0.0f, 0.0f, -1.0f));
	static Vector oldAngVel(0.0f, 0.0f, 0.0f);

	// Calculate position
	Vector XYZ(
		interpolate(_path[seg][0], _path[seg][3],
			_path[seg + 1][0], _path[seg + 1][3], where),
		interpolate(_path[seg][1], _path[seg][4],
			_path[seg + 1][1], _path[seg + 1][4], where),
		interpolate(_path[seg][2], _path[seg][5],
			_path[seg + 1][2], _path[seg + 1][5], where)
	);

	static float maxSpin = 0.0025f * speed;
	static float rotationInertia = 0.007f * speed;

	// Do rotation stuff
	UnitVector dir(XYZ - oldXYZ);	// Direction of motion
	Vector angVel(Vector::cross(dir, oldDir));	// Desired axis of rotation
	float temp = Vector::dot(oldDir, dir);
	if (temp < -1.0f) temp = -1.0f;
	if (temp > +1.0f) temp = +1.0f;
	float angle = Common::clamp(
		float(std::acos(temp)), // Desired turn angle
		-maxSpin, maxSpin
	);
	angVel *= angle;	// Desired angular velocity
	Vector tempVec(angVel - oldAngVel);	// Change in angular velocity
	// Don't let angular velocity change too much
	float distance = tempVec.length();
	if (distance > rotationInertia * Common::elapsedSecs) {
		tempVec *= (rotationInertia * Common::elapsedSecs) / distance;
		angVel = oldAngVel + tempVec;
	}

	static float flymodeChange = 20.0f;
	static int flymode = 1;

	flymodeChange -= Common::elapsedSecs;
	if (flymodeChange <= 1.0f)	// prepare to transition
		angVel *= flymodeChange;
	if (flymodeChange <= 0.0f) {	// transition from one fly mode to the other?
		flymode = Common::randomInt(4);
		flymodeChange = Common::randomFloat(float(150 - speed)) + 5.0f;
	}
	tempVec = angVel;  // Recompute desired rotation
	angle = tempVec.normalize();

	static UnitQuat quat;

	if (flymode)	// fly normal (straight)
		quat.multiplyBy(UnitQuat(angle, tempVec));
	else  // don't fly normal (go backwards and stuff)
		quat.preMultiplyBy(UnitQuat(angle, tempVec));

	// Roll
	static float rollChange = Common::randomFloat(10.0f) + 2.0f;
	static float rollAcc = 0.0f;
	static float rollVel = 0.0f;
	rollChange -= Common::elapsedSecs;
	if (rollChange <= 0.0f) {
		rollAcc = Common::randomFloat(0.02f * speed) - (0.01f * speed);
		rollChange = Common::randomFloat(10.0f) + 2.0f;
	}
	rollVel += rollAcc * Common::elapsedSecs;
	if (rollVel > (0.04f * speed) && rollAcc > 0.0f)
		rollAcc = 0.0f;
	if (rollVel < (-0.04f * speed) && rollAcc < 0.0f)
		rollAcc = 0.0f;
	quat.multiplyBy(UnitQuat(rollVel * Common::elapsedSecs, oldDir));

	RotationMatrix rotMat(quat);

	// Save old stuff
	oldXYZ = XYZ;
	oldDir = Vector(
		-rotMat.get()[2],
		-rotMat.get()[6],
		-rotMat.get()[10]
	);
	oldAngVel = angVel;

	// Apply transformations
	glLoadMatrixf(rotMat.get());
	glTranslatef(-XYZ.x(), -XYZ.y(), -XYZ.z());

	// Render everything
	static int drawDepth = depth + 2;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (int i = _globalXYZ[0] - drawDepth;
		i <= _globalXYZ[0] + drawDepth; ++i) {
		for (int j = _globalXYZ[1] - drawDepth;
			j <= _globalXYZ[1] + drawDepth; ++j) {
			for (int k = _globalXYZ[2] - drawDepth;
				k <= _globalXYZ[2] + drawDepth; ++k) {
				Vector tempVec(Vector(i, j, k) - XYZ);
				// transformed position
				Vector tPos(rotMat.transform(tempVec));
				if (Camera::isVisible(tPos, 0.9f)) {
					unsigned int indexX = latticeMod(i);
					unsigned int indexY = latticeMod(j);
					unsigned int indexZ = latticeMod(k);
					// draw it
					glPushMatrix();
						glTranslatef(float(i), float(j), float(k));
						glCallList(_lattice[indexX][indexY][indexZ]);
					glPopMatrix();
				}
			}
		}
	}

	Common::flush();
}

void Hack::reshape() {
	if (widescreen)
		glViewport(0, Common::height / 2 - Common::width / 4,
			Common::width, Common::width / 2);
	else
		glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float mat[16] = {
		std::cos(fov * 0.5f * D2R) / std::sin(fov * 0.5f * D2R), 0.0f, 0.0f, 0.0f,
		0.0f, 0.0, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f - 0.02f / float(depth), -1.0f,
		0.0f, 0.0f, -(0.02f + 0.0002f / float(depth)), 0.0f
	};
	if (widescreen)
		mat[5] = mat[0] * 2.0f;
	else
		mat[5] = mat[0] * Common::aspectRatio;
	glLoadMatrixf(mat);
	Camera::set(mat, float(depth));
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
