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
#include <flares.hh>
#include <resources.hh>

// Draw a flare at a specified (x,y) location on the screen
// Screen corners are at (0,0) and (1,1)
void Flares::draw(float x, float y, const RGBColor& RGB, float alpha) {
	// Fade alpha if source is off edge of screen
	float fadeWidth = float(Common::width) / 10.0f;
	if (y < 0.0f) {
		float temp = fadeWidth + y;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}
	if (y > Common::height) {
		float temp = fadeWidth - y + Common::height;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}
	if (x < 0) {
		float temp = fadeWidth + x;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}
	if (x > Common::width) {
		float temp = fadeWidth - x + Common::width;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}

	// Find lens flare vector
	// This vector runs from the light source through the screen's center
	float dx = 0.5f * Common::aspectRatio - x;
	float dy = 0.5f - y;

	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// Setup projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, Common::aspectRatio, 0, 1.0f);

	// Draw stuff
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	glTranslatef(x + dx * 0.05f, y + dy * 0.05f, 0.0f);
	glScalef(0.065f, 0.065f, 0.065f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.4f);
	glCallList(Resources::DisplayLists::flares + 2);

	glLoadIdentity();
	glTranslatef(x + dx * 0.15f, y + dy * 0.15f, 0.0f);
	glScalef(0.04f, 0.04f, 0.04f);
	glColor4f(RGB.r() * 0.9f, RGB.g() * 0.9f, RGB.b(), alpha * 0.9f);
	glCallList(Resources::DisplayLists::flares + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 0.25f, y + dy * 0.25f, 0.0f);
	glScalef(0.06f, 0.06f, 0.06f);
	glColor4f(RGB.r() * 0.8f, RGB.g() * 0.8f, RGB.b(), alpha * 0.9f);
	glCallList(Resources::DisplayLists::flares + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 0.35f, y + dy * 0.35f, 0.0f);
	glScalef(0.08f, 0.08f, 0.08f);
	glColor4f(RGB.r() * 0.7f, RGB.g() * 0.7f, RGB.b(), alpha * 0.9f);
	glCallList(Resources::DisplayLists::flares + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 1.25f, y + dy * 1.25f, 0.0f);
	glScalef(0.05f, 0.05f, 0.05f);
	glColor4f(RGB.r(), RGB.g() * 0.6f, RGB.b() * 0.6f, alpha * 0.9f);
	glCallList(Resources::DisplayLists::flares + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 1.65f, y + dy * 1.65f, 0.0f);
	glRotatef(x, 0, 0, 1);
	glScalef(0.3f, 0.3f, 0.3f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha);
	glCallList(Resources::DisplayLists::flares + 3);

	glLoadIdentity();
	glTranslatef(x + dx * 1.85f, y + dy * 1.85f, 0.0f);
	glScalef(0.04f, 0.04f, 0.04f);
	glColor4f(RGB.r(), RGB.g() * 0.6f, RGB.b() * 0.6f, alpha * 0.9f);
	glCallList(Resources::DisplayLists::flares + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 2.2f, y + dy * 2.2f, 0.0f);
	glScalef(0.3f, 0.3f, 0.3f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.7f);
	glCallList(Resources::DisplayLists::flares + 1);

	glLoadIdentity();
	glTranslatef(x + dx * 2.5f, y + dy * 2.5f, 0.0f);
	glScalef(0.6f, 0.6f, 0.6f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.8f);
	glCallList(Resources::DisplayLists::flares + 3);

	glPopMatrix();

	// Unsetup projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopAttrib();
}

// super bright elongated glow for sucker, shockwave, stretcher, and bigmama
void Flares::drawSuper(float x, float y, const RGBColor& RGB, float alpha) {
	// Fade alpha if source is off edge of screen
	float fadeWidth = float(Common::width) / 10.0f;
	if (y < 0.0f) {
		float temp = fadeWidth + y;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}
	if (y > Common::height) {
		float temp = fadeWidth - y + Common::height;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}
	if (x < 0) {
		float temp = fadeWidth + x;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}
	if (x > Common::width) {
		float temp = fadeWidth - x + Common::width;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadeWidth;
	}

	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// Setup projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, Common::aspectRatio, 0, 1.0f);

	// Draw stuff
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	glTranslatef(x, y, 0.0f);
	glScalef(2.0f * alpha, 0.08f, 0.0f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha);
	glCallList(Resources::DisplayLists::flares + 0);

	glLoadIdentity();
	glTranslatef(x, y, 0.0f);
	glScalef(0.4f, 0.35f * alpha + 0.05f, 1.0f);
	glColor4f(RGB.r(), RGB.g(), RGB.b(), alpha * 0.4f);
	glCallList(Resources::DisplayLists::flares + 2);

	glPopMatrix();

	// Unsetup projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopAttrib();
}
