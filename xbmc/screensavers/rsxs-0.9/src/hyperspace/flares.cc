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

#include <color.hh>
#include <hyperspace.hh>
#include <flares.hh>

#define FLARESIZE 64

namespace Flares {
	GLuint blob;
};

namespace Flares {
	GLuint _lists;
	GLuint _flare[4];
};

void Flares::init() {
	stdx::dim3<GLubyte, 4, FLARESIZE> flareMap[4];
	for (unsigned int i = 0; i < 4; ++i)
		flareMap[i].resize(FLARESIZE);

	for (unsigned int i = 0; i < FLARESIZE; ++i) {
		float x = float(int(i) - FLARESIZE / 2) / float(FLARESIZE / 2);
		for (unsigned int j = 0; j < FLARESIZE; ++j) {
			float y = float(int(j) - FLARESIZE / 2) / float(FLARESIZE / 2);
			float temp;

			// Basic flare
			flareMap[0](i, j, 0) = 255;
			flareMap[0](i, j, 1) = 255;
			flareMap[0](i, j, 2) = 255;
			temp = 1.0f - ((x * x) + (y * y));
			if (temp > 1.0f) temp = 1.0f;
			if (temp < 0.0f) temp = 0.0f;
			flareMap[0](i, j, 3) = GLubyte(255.0f * temp * temp);

			// Flattened sphere
			flareMap[1](i, j, 0) = 255;
			flareMap[1](i, j, 1) = 255;
			flareMap[1](i, j, 2) = 255;
			temp = 2.5f * (1.0f - ((x * x) + (y * y)));
			if (temp > 1.0f) temp = 1.0f;
			if (temp < 0.0f) temp = 0.0f;
			flareMap[1](i, j, 3) = GLubyte(255.0f * temp);

			// Torus
			flareMap[2](i, j, 0) = 255;
			flareMap[2](i, j, 1) = 255;
			flareMap[2](i, j, 2) = 255;
			temp = 4.0f * ((x * x) + (y * y)) * (1.0f - ((x * x) + (y * y)));
			if (temp > 1.0f) temp = 1.0f;
			if (temp < 0.0f) temp = 0.0f;
			temp = temp * temp * temp * temp;
			flareMap[2](i, j, 3) = GLubyte(255.0f * temp);

			// Kick-ass!
			x = std::abs(x);
			y = std::abs(y);
			float xy = x * y;
			flareMap[3](i, j, 0) = 255;
			flareMap[3](i, j, 1) = 255;
			temp = 0.14f * (1.0f - ((x > y) ? x : y)) / ((xy > 0.05f) ? xy : 0.05f);
			if (temp > 1.0f) temp = 1.0f;
			if (temp < 0.0f) temp = 0.0f;
			flareMap[3](i, j, 2) = GLubyte(255.0f * temp);
			temp = 0.1f * (1.0f - ((x > y) ? x : y)) / ((xy > 0.1f) ? xy : 0.1f);
			if (temp > 1.0f) temp = 1.0f;
			if (temp < 0.0f) temp = 0.0f;
			flareMap[3](i, j, 3) = GLubyte(255.0f * temp);
		}
	}

	for (unsigned int i = 0; i < 4; ++i)
		_flare[i] = Common::resources->genTexture(
			GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			4, FLARESIZE, FLARESIZE, GL_RGBA, GL_UNSIGNED_BYTE,
			&flareMap[i].front(), false
		);

	_lists = Common::resources->genLists(4);
	for (unsigned int i = 0; i < 4; ++i) {
		glNewList(_lists + i, GL_COMPILE);
			glBindTexture(GL_TEXTURE_2D, _flare[i]);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(-0.5f, -0.5f, 0.0f);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(0.5f, -0.5f, 0.0f);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(-0.5f, 0.5f, 0.0f);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(0.5f, 0.5f, 0.0f);
			glEnd();
		glEndList();
	}

	blob = _flare[0];
}

// Draw a flare at a specified (x,y) location on the screen
// Screen corners are at (0,0) and (1,1)
void Flares::draw(const Vector& XYZ, const RGBColor& RGB, float alpha) {
	double winX, winY, winZ;
	gluProject(
		XYZ.x(), XYZ.y(), XYZ.z(),
		Hack::modelMat, Hack::projMat, Hack::viewport,
		&winX, &winY, &winZ
	);
	if (winZ > 1.0f)
		return;

	// Fade alpha if source is off edge of screen
	float fadeWidth = float(Common::width) * 0.1f;
	if (winY < 0.0f) {
		float temp = fadeWidth + winY;
		if (temp < 0.0f) return;
		alpha *= temp / fadeWidth;
	}
	if (winY > Common::height) {
		float temp = fadeWidth - winY + Common::height;
		if (temp < 0.0f) return;
		alpha *= temp / fadeWidth;
	}
	if (winX < 0) {
		float temp = fadeWidth + winX;
		if (temp < 0.0f) return;
		alpha *= temp / fadeWidth;
	}
	if (winX > Common::width) {
		float temp = fadeWidth - winX + Common::width;
		if (temp < 0.0f) return;
		alpha *= temp / fadeWidth;
	}

	float x = (float(winX) / float(Common::width)) * Common::aspectRatio;
	float y = float(winY) / float(Common::height);

	// Find lens flare vector
	// This vector runs from the light source through the screen's center
	float dx = 0.5f * Common::aspectRatio - x;
	float dy = 0.5f - y;

	glPushAttrib(GL_ENABLE_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	// Setup projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, Common::aspectRatio, 0, 1.0f);

	// Update fractal flickering
	static float flicker = 0.95f;
	flicker += Common::elapsedSecs * (Common::randomFloat(2.0f) - 1.0f);
	if (flicker < 0.9f) flicker = 0.9f;
	if (flicker > 1.1f) flicker = 1.1f;
	alpha *= flicker;

	// Draw stuff
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	glTranslatef(x, y, 0.0f);
	glScalef(0.1f * flicker, 0.1f * flicker, 1.0f);
	glColor4f(RGB.r(), RGB.g(), RGB.b() * 0.8f, alpha);
	glCallList(_lists + 0);

	// wide flare
	glLoadIdentity();
	glTranslatef(x, y, 0.0f);
	glScalef(5.0f * alpha, 0.05f * alpha, 1.0f);
	glColor4f(RGB.r() * 0.3f, RGB.g() * 0.3f, RGB.b(), alpha);
	glCallList(_lists + 0);

	// torus
	glLoadIdentity();
	glTranslatef(x, y, 0.0f);
	glScalef(0.5f, 0.2f, 1.0f);
	glColor4f(RGB.r(), RGB.g() * 0.5f, RGB.b() * 0.5f, alpha * 0.4f);
	glCallList(_lists + 2);

	// 3 blueish dots
	glLoadIdentity();
	glTranslatef(x + dx * 0.35f, y + dy * 0.35f, 0.0f);
	glScalef(0.06f, 0.06f, 1.0f);
	glColor4f(RGB.r() * 0.85f, RGB.g() * 0.85f, RGB.b(), alpha * 0.5f);
	glCallList(_lists + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 0.45f, y + dy * 0.45f, 0.0f);
	glScalef(0.09f, 0.09f, 1.0f);
	glColor4f(RGB.r() * 0.7f, RGB.g() * 0.7f, RGB.b(), alpha * 0.4f);
	glCallList(_lists + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 0.55f, y + dy * 0.55f, 0.0f);
	glScalef(0.12f, 0.12f, 1.0f);
	glColor4f(RGB.r() * 0.55f, RGB.g() * 0.55f, RGB.b(), alpha * 0.3f);
	glCallList(_lists + 1);

	// 4 more dots
	glLoadIdentity();
	glTranslatef(x + dx * 0.75f, y + dy * 0.75f, 0.0f);
	glScalef(0.14f, 0.07f, 1.0f);
	glColor4f(RGB.r() * 0.3f, RGB.g() * 0.3f, RGB.b() * 0.3f, alpha);
	glCallList(_lists + 3);

	glLoadIdentity();
	glTranslatef(x + dx * 0.78f, y + dy * 0.78f, 0.0f);
	glScalef(0.06f, 0.06f, 1.0f);
	glColor4f(RGB.r() * 0.3f, RGB.g() * 0.4f, RGB.b() * 0.4f, alpha * 0.5f);
	glCallList(_lists + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 1.25f, y + dy * 1.25f, 0.0f);
	glScalef(0.1f, 0.1f, 1.0f);
	glColor4f(RGB.r() * 0.3f, RGB.g() * 0.4f, RGB.b() * 0.3f, alpha * 0.5f);
	glCallList(_lists + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 1.3f, y + dy * 1.3f, 0.0f);
	glScalef(0.07f, 0.07f, 1.0f);
	glColor4f(RGB.r() * 0.6f, RGB.g() * 0.45f, RGB.b() * 0.3f, alpha * 0.5f);
	glCallList(_lists + 1);

	// stretched weird flare
	glLoadIdentity();
	glTranslatef(x + dx * 1.45f, y + dy * 1.45f, 0.0f);
	glScalef(0.8f, 0.2f, 1.0f);
	glRotatef(x * 70.0f, 0, 0, 1);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.4f);
	glCallList(_lists + 3);

	// circle
	glLoadIdentity();
	glTranslatef(x + dx * 2.0f, y + dy * 2.0f, 0.0f);
	glScalef(0.3f, 0.3f, 1.0f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.2f);
	glCallList(_lists + 1);

	// big weird flare
	glLoadIdentity();
	glTranslatef(x + dx * 2.4f, y + dy * 2.4f, 0.0f);
	glRotatef(y * 40.0f, 0, 0, 1);
	glScalef(0.7f, 0.7f, 1.0f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.3f);
	glCallList(_lists + 3);

	// Unsetup projection matrix
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}
