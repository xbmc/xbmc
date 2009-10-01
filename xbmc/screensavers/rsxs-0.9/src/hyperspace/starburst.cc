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

#include <flares.hh>
#include <hyperspace.hh>
#include <nebula.hh>
#include <particle.hh>
#include <shaders.hh>
#include <starburst.hh>

#define STARBURST_COUNT 200

namespace StarBurst {
	std::vector<StretchedParticle> _stars;
	bool _active[STARBURST_COUNT];
	UnitVector _velocity[STARBURST_COUNT];
	float _size;
	Vector _pos;

	GLuint _list;

	void drawStars();
};

void StarBurst::init() {
	for (unsigned int i = 0; i < STARBURST_COUNT; ++i) {
		_stars.push_back(StretchedParticle(
			Vector(), 0.03f, RGBColor(1.0f, 1.0f, 1.0f), 0.0f
		));
		_active[i] = false;
		_velocity[i] = UnitVector(Vector(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		));
	}

	_list = Common::resources->genLists(1);
	glNewList(_list, GL_COMPILE);
		for (unsigned int j = 0; j < 32; ++j) {
			float cj = std::cos(float(j) * M_PI * 2.0f / 32.0f);
			float sj = std::sin(float(j) * M_PI * 2.0f / 32.0f);
			float cjj = std::cos(float(j + 1) * M_PI * 2.0f / 32.0f);
			float sjj = std::sin(float(j + 1) * M_PI * 2.0f / 32.0f);
			glBegin(GL_TRIANGLE_STRIP);
				for (unsigned int i = 0; i <= 32; ++i) {
					float ci = std::cos(float(i) * M_PI * 2.0f / 32.0f);
					float si = std::sin(float(i) * M_PI * 2.0f / 32.0f);
					glNormal3f(sj * ci, cj, sj * si);
					glVertex3f(sj * ci, cj, sj * si);
					glNormal3f(sjj * ci, cjj, sjj * si);
					glVertex3f(sjj * ci, cjj, sjj * si);
				}
			glEnd();
		}
	glEndList();

	_size = 4.0f;
}

void StarBurst::drawStars() {
	// draw stars
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, Flares::blob);
	for (unsigned int i = 0; i < STARBURST_COUNT; ++i) {
		_stars[i].offsetPos(_velocity[i] * Common::elapsedTime);
		float distance = (_stars[i].getPos() - Hack::camera).length();
		if (distance > Hack::fogDepth) _active[i] = false;
		if (_active[i]) _stars[i].draw();
	}
}

void StarBurst::restart(const Vector& XYZ) {
	// don't restart if any star is still active
	for (unsigned int i = 0; i < STARBURST_COUNT; ++i)
		if (_active[i])
			return;
	// or if flare hasn't faded out completely
	if (_size < 3.0f)
		return;

	RGBColor RGB(
		Common::randomFloat(1.0f),
		Common::randomFloat(1.0f),
		Common::randomFloat(1.0f)
	);
	switch (Common::randomInt(3)) {
	case 0: RGB.r() = 1.0f; break;
	case 1: RGB.g() = 1.0f; break;
	case 2: RGB.b() = 1.0f; break;
	}

	for (unsigned int i = 0; i < STARBURST_COUNT; ++i) {
		_active[i] = true;
		_stars[i].setPos(XYZ);
		_stars[i].setColor(RGB);
	}

	_size = 0.0f;
	_pos = XYZ;
}

void StarBurst::update() {
	static float starBurstTime = Common::randomFloat(180.0f) + 180.0f; // burst after 3-6 minutes
	starBurstTime -= Common::elapsedTime;
	if (starBurstTime <= 0.0f) {
		Vector pos(
			Hack::camera.x() + (Hack::dir.x() * Hack::fogDepth * 0.5f) +
				Common::randomFloat(Hack::fogDepth * 0.5f) - Hack::fogDepth * 0.25f,
			Common::randomFloat(2.0f) - 1.0f,
			Hack::camera.z() + (Hack::dir.z() * Hack::fogDepth * 0.5f) +
				Common::randomFloat(Hack::fogDepth * 0.5f) - Hack::fogDepth * 0.25f
		);
		StarBurst::restart(pos); // it won't actually restart unless it's ready to
		starBurstTime = Common::randomFloat(540.0f) + 60.0f; // burst again within 1-10 minutes
	}
}

void StarBurst::draw() {
	drawStars();

	_size += Common::elapsedTime * 0.5f;
	if (_size >= 3.0f)
		return;

	// draw flare
	float brightness = 1.0f - (_size / 3.0f);
	if (brightness > 0.0f)
		Flares::draw(_pos, RGBColor(1.0f, 1.0f, 1.0f), brightness);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(_size, _size, _size);

		// draw sphere
		glPushAttrib(GL_ENABLE_BIT);
			Nebula::use();

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glEnable(GL_BLEND);
			glColor4f(brightness, brightness, brightness, (Hack::shaders ? Hack::lerp : 1.0f));
			glCallList(_list);
		glPopAttrib();
	glPopMatrix();
}
