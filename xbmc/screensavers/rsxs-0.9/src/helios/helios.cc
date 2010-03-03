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
#include <helios.hh>
#include <implicit.hh>
#include <particle.hh>
#include <pngimage.hh>
#include <sphere.hh>
#include <resource.hh>
#include <vector.hh>

namespace Hack {
	unsigned int numIons = 1500;
	float size = 10.0f;
	unsigned int numEmitters = 3;
	unsigned int numAttractors = 3;
	float speed = 10.0f;
	float cameraSpeed = 10.0f;
	bool surface = true;
	bool wireframe = false;
	float blur = 30.0f;
	std::string texture("spheremap.png");
};

namespace Hack {
	enum Arguments {
		ARG_IONS = 1,
		ARG_EMITTERS,
		ARG_ATTRACTORS,
		ARG_SIZE,
		ARG_SPEED,
		ARG_CAMERASPEED,
		ARG_BLUR,
		ARG_TEXTURE,
		ARG_SURFACE = 0x100, ARG_NO_SURFACE,
		ARG_WIREFRAME = 0x200, ARG_NO_WIREFRAME
	};

	std::vector<Node> _eList;
	std::vector<Node> _aList;
	std::vector<Ion> _iList;
	std::vector<Sphere> _spheres;

	GLuint _texture;
	Implicit* _surface;

	void setTargets(unsigned int);
	float surfaceFunction(const Vector&);

	error_t parse(int, char*, struct argp_state*);
};

error_t Hack::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARG_IONS:
		if (Common::parseArg(arg, numIons, 1u, 30000u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of ions must be between 1 and 30000");
		return 0;
	case ARG_EMITTERS:
		if (Common::parseArg(arg, numEmitters, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of ion emitters must be between 1 and 10");
		return 0;
	case ARG_ATTRACTORS:
		if (Common::parseArg(arg, numAttractors, 1u, 10u))
			argp_failure(state, EXIT_FAILURE, 0,
				"number of ion attractors must be between 1 and 10");
		return 0;
	case ARG_SIZE:
		if (Common::parseArg(arg, size, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"ion size must be between 1 and 100");
		return 0;
	case ARG_SPEED:
		if (Common::parseArg(arg, speed, 1.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"ion speed must be between 1 and 100");
		return 0;
	case ARG_CAMERASPEED:
		if (Common::parseArg(arg, cameraSpeed, 0.0f, 100.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"camera speed must be between 0 and 100");
		return 0;
	case ARG_BLUR:
		if (Common::parseArg(arg, blur, 0.0f, 30.0f))
			argp_failure(state, EXIT_FAILURE, 0,
				"motion blur must be between 0 and 30");
		return 0;
	case ARG_SURFACE:
		surface = true;
		return 0;
	case ARG_NO_SURFACE:
		surface = false;
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
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

const struct argp* Hack::getParser() {
	static struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Ion options:" },
		{ "ions", ARG_IONS, "NUM", 0, "Number of ions (1-30000, default = 1500)" },
		{ "emitters", ARG_EMITTERS, "NUM", 0,
			"Number of ion emitters (1-10, default = 3)" },
		{ "attractors", ARG_ATTRACTORS, "NUM", 0,
			"Number of ion attractors (1-10, default = 3)" },
		{ "size", ARG_SIZE, "NUM", 0, "Ion size (1-100, default = 10)" },
		{ "speed", ARG_SPEED, "NUM", 0, "Ion speed (1-100, default = 10)" },
		{ NULL, 0, NULL, 0, "Surface options:" },
		{ "surface", ARG_SURFACE, NULL, OPTION_HIDDEN, "Disable iso-surface" },
		{ "no-surface", ARG_NO_SURFACE, NULL, OPTION_ALIAS },
		{ "wireframe", ARG_WIREFRAME, NULL, 0, "Draw iso-surface using wireframe" },
		{ "no-wireframe", ARG_NO_WIREFRAME, NULL, OPTION_ALIAS | OPTION_HIDDEN },
		{ "texture", ARG_TEXTURE, "FILE", 0, "Surface image PNG file" },
		{ "surfaceimage", ARG_TEXTURE, "FILE", OPTION_ALIAS | OPTION_HIDDEN },
		{ NULL, 0, NULL, 0, "View options:" },
		{ "cameraspeed", ARG_CAMERASPEED, "NUM", 0,
			"Camera speed (0-100, default = 10)" },
		{ "blur", ARG_BLUR, "NUM", 0, "Motion blur (0-100, default = 30)" },
		{}
	};
	static struct argp parser = {
		options, parse, NULL,
		"Draws exploding ion systems and smooth helion surfaces."
	};
	return &parser;
}

std::string Hack::getShortName() { return "helios"; }
std::string Hack::getName()      { return "Helios"; }

void Hack::setTargets(unsigned int whichTarget) {
	switch (whichTarget) {
	case 0:	// random
		for (unsigned int i = 0; i < numEmitters; ++i)
			_eList[i].setTargetPos(Vector(
				Common::randomFloat(1000.0f) - 500.0f,
				Common::randomFloat(1000.0f) - 500.0f,
				Common::randomFloat(1000.0f) - 500.0f
			));
		for (unsigned int i = 0; i < numAttractors; ++i)
			_aList[i].setTargetPos(Vector(
				Common::randomFloat(1000.0f) - 500.0f,
				Common::randomFloat(1000.0f) - 500.0f,
				Common::randomFloat(1000.0f) - 500.0f
			));
		break;
	case 1:	// line (all emitters on one side, all attracters on the other)
		{
			float position = -500.0f, change = 1000.0f /
				(numEmitters + numAttractors - 1);
			for (unsigned int i = 0; i < numEmitters; ++i) {
				_eList[i].setTargetPos(Vector(
					position,
					position * 0.5f,
					0.0f
				));
				position += change;
			}
			for (unsigned int i = 0; i < numAttractors; ++i) {
				_aList[i].setTargetPos(Vector(
					position,
					position * 0.5f,
					0.0f
				));
				position += change;
			}
		}
		break;
	case 2:	// line (emitters and attracters staggered)
		{
			float change = (numEmitters > numAttractors)
				? 1000.0f / (numEmitters * 2 - 1)
				: 1000.0f / (numAttractors * 2 - 1);
			float position = -500.0f;
			for (unsigned int i = 0; i < numEmitters; ++i) {
				_eList[i].setTargetPos(Vector(
					position,
					position * 0.5f,
					0.0f
				));
				position += change * 2.0f;
			}
			position = -500.0f + change;
			for (unsigned int i = 0; i < numAttractors; ++i) {
				_aList[i].setTargetPos(Vector(
					position,
					position * 0.5f,
					0.0f
				));
				position += change * 2.0f;
			}
		}
		break;
	case 3:	// 2 lines (parallel)
		{
			float change = 1000.0f / (numEmitters * 2 - 1);
			float position = -500.0f;
			float height = -525.0f + (numEmitters * 25);
			for (unsigned int i = 0; i < numEmitters; ++i) {
				_eList[i].setTargetPos(Vector(
					position,
					height,
					-50.0f
				));
				position += change * 2.0f;
			}
			change = 1000.0f / (numAttractors * 2 - 1);
			position = -500.0f;
			height = 525.0f - (numAttractors * 25);
			for (unsigned int i = 0; i < numAttractors; ++i) {
				_aList[i].setTargetPos(Vector(
					position,
					height,
					50.0f
				));
				position += change * 2.0f;
			}
		}
		break;
	case 4:	// 2 lines (skewed)
		{
			float change = 1000.0f / (numEmitters * 2 - 1);
			float position = -500.0f;
			float height = -525.0f + (numEmitters * 25);
			for (unsigned int i = 0; i < numEmitters; ++i) {
				_eList[i].setTargetPos(Vector(
					position,
					height,
					0.0f
				));
				position += change * 2.0f;
			}
			change = 1000.0f / (numAttractors * 2 - 1);
			position = -500.0f;
			height = 525.0f - (numAttractors * 25);
			for (unsigned int i = 0; i < numAttractors; ++i) {
				_aList[i].setTargetPos(Vector(
					10.0f,
					height,
					position
				));
				position += change * 2.0f;
			}
		}
		break;
	case 5:	// random distribution across a plane
		for (unsigned int i = 0; i < numEmitters; ++i)
			_eList[i].setTargetPos(Vector(
				Common::randomFloat(1000.0f) - 500.0f,
				0.0f,
				Common::randomFloat(1000.0f) - 500.0f
			));
		for (unsigned int i = 0; i < numAttractors; ++i)
			_aList[i].setTargetPos(Vector(
				Common::randomFloat(1000.0f) - 500.0f,
				0.0f,
				Common::randomFloat(1000.0f) - 500.0f
			));
		break;
	case 6:	// random distribution across 2 planes
		{
			float height = -525.0f + (numEmitters * 25);
			for (unsigned int i = 0; i < numEmitters; ++i)
				_eList[i].setTargetPos(Vector(
					Common::randomFloat(1000.0f) - 500.0f,
					height,
					Common::randomFloat(1000.0f) - 500.0f
				));
			height = 525.0f - (numAttractors * 25);
			for (unsigned int i = 0; i < numAttractors; i++)
				_aList[i].setTargetPos(Vector(
					Common::randomFloat(1000.0f) - 500.0f,
					height,
					Common::randomFloat(1000.0f) - 500.0f
				));
		}
		break;
	case 7:	// 2 rings (1 inside and 1 outside)
		{
			float angle = 0.5f, cosangle, sinangle;
			float change = M_PI * 2.0f / numEmitters;
			for (unsigned int i = 0; i < numEmitters; ++i) {
				angle += change;
				cosangle = std::cos(angle) * 200.0f;
				sinangle = std::sin(angle) * 200.0f;
				_eList[i].setTargetPos(Vector(
					cosangle,
					sinangle,
					0.0f
				));
			}
			angle = 1.5f;
			change = M_PI * 2.0f / numAttractors;
			for (unsigned int i = 0; i < numAttractors; ++i) {
				angle += change;
				cosangle = std::cos(angle) * 500.0f;
				sinangle = std::sin(angle) * 500.0f;
				_aList[i].setTargetPos(Vector(
					cosangle,
					sinangle,
					0.0f
				));
			}
		}
		break;
	case 8:	// ring (all emitters on one side, all attracters on the other)
		{
			float angle = 0.5f, cosangle, sinangle;
			float change = M_PI * 2.0f / (numEmitters + numAttractors);
			for (unsigned int i = 0; i < numEmitters; ++i) {
				angle += change;
				cosangle = std::cos(angle) * 500.0f;
				sinangle = std::sin(angle) * 500.0f;
				_eList[i].setTargetPos(Vector(
					cosangle,
					sinangle,
					0.0f
				));
			}
			for (unsigned int i = 0; i < numAttractors; ++i) {
				angle += change;
				cosangle = std::cos(angle) * 500.0f;
				sinangle = std::sin(angle) * 500.0f;
				_aList[i].setTargetPos(Vector(
					cosangle,
					sinangle,
					0.0f
				));
			}
		}
		break;
	case 9:	// ring (emitters and attracters staggered)
		{
			float change = (numEmitters > numAttractors)
				? M_PI * 2.0f / (numEmitters * 2)
				: M_PI * 2.0f / (numAttractors * 2);
			float angle = 0.5f, cosangle, sinangle;
			for (unsigned int i = 0; i < numEmitters; ++i) {
				cosangle = std::cos(angle) * 500.0f;
				sinangle = std::sin(angle) * 500.0f;
				_eList[i].setTargetPos(Vector(
					cosangle,
					sinangle,
					0.0f
				));
				angle += change * 2.0f;
			}
			angle = 0.5f + change;
			for (unsigned int i = 0; i < numAttractors; ++i) {
				cosangle = std::cos(angle) * 500.0f;
				sinangle = std::sin(angle) * 500.0f;
				_aList[i].setTargetPos(Vector(
					cosangle,
					sinangle,
					0.0f
				));
				angle += change * 2.0f;
			}
		}
		break;
	case 10:	// 2 points
		for (unsigned int i = 0; i < numEmitters; ++i)
			_eList[i].setTargetPos(Vector(500.0f, 100.0f, 50.0f));
		for (unsigned int i = 0; i < numAttractors; ++i)
			_aList[i].setTargetPos(Vector(-500.0f, -100.0f, -50.0f));
		break;
	}
}

float Hack::surfaceFunction(const Vector& XYZ) {
	static unsigned int points = numEmitters + numAttractors;

	float value = 0.0f;
	for (unsigned int i = 0; i < points; ++i)
		value += _spheres[i].value(XYZ);

	return value;
}

void Hack::start() {
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, Common::aspectRatio, 0.1, 10000.0f);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_TEXTURE_2D);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Initialize surface
	if (surface) {
		_texture = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			PNG(texture)
		);

		Implicit::init(70, 70, 70, 25.0f);
		_surface = new Implicit(surfaceFunction);
		Common::resources->manage(_surface);
		_spheres = std::vector<Sphere>(numEmitters + numAttractors);
		float sphereScaleFactor = 1.0f /
			std::sqrt(float(2 * numEmitters + numAttractors));
		stdx::call_each(_spheres.begin(), _spheres.begin() + numEmitters,
			&Sphere::setScale, 400.0f * sphereScaleFactor);
		stdx::call_each(_spheres.begin() + numEmitters, _spheres.begin() + numEmitters + numAttractors,
			&Sphere::setScale, 200.0f * sphereScaleFactor);
	}

	Ion::init();

	// Initialize particles
	stdx::construct_n(_eList, numEmitters);
	stdx::construct_n(_aList, numAttractors);
	stdx::construct_n(_iList, numIons);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
}

void Hack::tick() {
	// Camera movements
	// first do translation (distance from center)
	static float targetCameraDistance = -1000.0f;
	static float preCameraInterp = M_PI;
	static float oldCameraDistance;

	preCameraInterp += cameraSpeed * Common::elapsedSecs / 100.0f;
	float cameraInterp = 0.5f - (0.5f * std::cos(preCameraInterp));
	float cameraDistance = (1.0f - cameraInterp) * oldCameraDistance
		+ cameraInterp * targetCameraDistance;
	if (preCameraInterp >= M_PI) {
		oldCameraDistance = targetCameraDistance;
		targetCameraDistance = -Common::randomFloat(1300.0f) - 200.0f;
		preCameraInterp = 0.0f;
	}
	glLoadIdentity();
	glTranslatef(0.0, 0.0, cameraDistance);

	// then do rotation
	static Vector radialVel(0.0f, 0.0f, 0.0f);
	static Vector targetRadialVel = radialVel;
	static UnitQuat rotQuat;

	Vector radialVelDiff(targetRadialVel - radialVel);
	float changeRemaining = radialVelDiff.normalize();
	float change = cameraSpeed * 0.0002f * Common::elapsedSecs;
	if (changeRemaining > change) {
		radialVelDiff *= change;
		radialVel += radialVelDiff;
	} else {
		radialVel = targetRadialVel;
		if (Common::randomInt(2)) {
			targetRadialVel.set(
				Common::randomFloat(1.0f),
				Common::randomFloat(1.0f),
				Common::randomFloat(1.0f)
			);
			targetRadialVel.normalize();
			targetRadialVel *= cameraSpeed * Common::randomFloat(0.002f);
		} else
			targetRadialVel.set(0.0f, 0.0f, 0.0f);
	}
	Vector tempRadialVel(radialVel);
	float angle = tempRadialVel.normalize();
	UnitQuat radialQuat(angle, tempRadialVel);
	rotQuat.multiplyBy(radialQuat);
	RotationMatrix billboardMat(rotQuat);

	// Calculate new color
	static HSLColor oldHSL;
	static HSLColor newHSL(
		Common::randomFloat(1.0f),
		1.0f,
		1.0f
	);
	static HSLColor targetHSL;
	static float colorInterp = 1.0f;
	static float colorChange;
	static RGBColor RGB;
	colorInterp += Common::elapsedSecs * colorChange;
	if (colorInterp >= 1.0f) {
		if (!Common::randomInt(3) && numIons >= 100) // change color suddenly
			newHSL.set(
				Common::randomFloat(1.0f),
				1.0f - (Common::randomFloat(1.0f) * Common::randomFloat(1.0f)),
				1.0f
			);
		oldHSL = newHSL;
		targetHSL.set(
			Common::randomFloat(1.0f),
			1.0f - (Common::randomFloat(1.0f) * Common::randomFloat(1.0f)),
			1.0f
		);
		colorInterp = 0.0f;
		// amount by which to change colorInterp each second
		colorChange = Common::randomFloat(0.005f * speed) + (0.002f * speed);
	} else {
		float diff = targetHSL.h() - oldHSL.h();
		if (diff < -0.5f || (diff > 0.0f && diff < 0.5f))
			newHSL.h() = oldHSL.h() + colorInterp * diff;
		else
			newHSL.h() = oldHSL.h() - colorInterp * diff;
		diff = targetHSL.s() - oldHSL.s();
			newHSL.s() = oldHSL.s() + colorInterp * diff;
		newHSL.clamp();
		RGB = newHSL;
	}

	// Release ions
	static unsigned int ionsReleased = 0;
	static int releaseMicros = 0;
	if (ionsReleased < numIons) {
		releaseMicros -= Common::elapsedMicros;
		while (ionsReleased < numIons && releaseMicros <= 0) {
			_iList[ionsReleased].start(
				_eList[Common::randomInt(numEmitters)].getPos(), RGB
			);
			++ionsReleased;
			// all ions released after 2 minutes
			releaseMicros += 120000000 / numIons;
		}
	}

	// Set interpolation value for emitters and attracters
	static int wait = 0;
	static float preInterp = M_PI, interp;
	static float interpConst = 0.001f;
	wait -= Common::elapsedMicros;
	if (wait <= 0) {
		preInterp += Common::elapsedSecs * speed * interpConst;
		interp = 0.5f - (0.5f * std::cos(preInterp));
	}
	if (preInterp >= M_PI) {
		// select new target points (not the same pattern twice in a row)
		static int newTarget = 0, lastTarget;
		lastTarget = newTarget;
		newTarget = Common::randomInt(10);
		if (newTarget == lastTarget) {
			++newTarget;
			newTarget %= 10;
		}
		setTargets(newTarget);
		preInterp = 0.0f;
		interp = 0.0f;
		wait = 10000000;	// pause after forming each new pattern
		// interpolate really fast sometimes
		interpConst = Common::randomInt(4) ? 0.001f : 0.1f;
	}

	// Update particles
	stdx::call_all(_eList, &Node::update, interp);
	stdx::call_all(_aList, &Node::update, interp);
	for (
		std::vector<Ion>::iterator i = _iList.begin(), j = i + ionsReleased;
		i != j;
		++i
	)
		i->update(_eList, _aList, RGB);

	if (surface) {
		for (unsigned int i = 0; i < numEmitters; ++i)
			_spheres[i].setCenter(_eList[i].getPos());
		for (unsigned int i = 0; i < numAttractors; ++i)
			_spheres[numEmitters + i].setCenter(_aList[i].getPos());
		std::list<Vector> crawlPointList;
		stdx::map_each(
			_spheres.begin(), _spheres.end(),
			std::back_inserter(crawlPointList),
			&Sphere::getCenter
		);
		static float valueTrig = 0.0f;
		valueTrig += Common::elapsedSecs;
		_surface->update(0.45f + 0.05f * std::cos(valueTrig), crawlPointList);
	}

	// Draw
	// clear the screen
	if (blur) {	// partially
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f - (std::sqrt(std::sqrt(blur)) * 0.15495f));
		glBindTexture(GL_TEXTURE_2D, 0);
		glPushMatrix();
			glLoadIdentity();
			glBegin(GL_TRIANGLE_STRIP);
				glVertex3f(-5.0f, -4.0f, -3.0f);
				glVertex3f(5.0f, -4.0f, -3.0f);
				glVertex3f(-5.0f, 4.0f, -3.0f);
				glVertex3f(5.0f, 4.0f, -3.0f);
			glEnd();
		glPopMatrix();
	} else	// completely
		glClear(GL_COLOR_BUFFER_BIT);

	// Draw ions
	glBlendFunc(GL_ONE, GL_ONE);
	stdx::call_each(_iList.begin(), _iList.begin() + ionsReleased,
		&Ion::draw, billboardMat);

	RGBColor surfaceColor;
	if (surface) {
		if (numIons >= 100) {
			float brightFactor = wireframe
				? 2.0f / ((blur + 30) * (blur + 30))
				: 4.0f / ((blur + 30) * (blur + 30));
			for (unsigned int i = 0; i < 100; ++i)
				surfaceColor += _iList[i].getRGB() * brightFactor;
		} else {
			float brightFactor = wireframe
				? 200.0f / ((blur + 30) * (blur + 30))
				: 400.0f / ((blur + 30) * (blur + 30));
			surfaceColor = RGB * brightFactor;
		}
		glPushMatrix(); glPushAttrib(GL_ENABLE_BIT);
			glBindTexture(GL_TEXTURE_2D, _texture);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glColor3fv(surfaceColor.get());
			glMultMatrixf(billboardMat.get());
			GLenum mode = GL_TRIANGLE_STRIP;
			if (Hack::wireframe) {
				glDisable(GL_TEXTURE_2D);
				mode = GL_LINE_STRIP;
			}
			_surface->draw(mode);
		glPopAttrib(); glPopMatrix();
	}

	Common::flush();
}

void Hack::reshape() {
	glViewport(0, 0, Common::width, Common::height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, Common::aspectRatio, 0.1, 10000.0f);
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

void FreeSettings()
{
}

void GetInfo(SCR_INFO *info)
{
}

void Remove()
{
}

}
