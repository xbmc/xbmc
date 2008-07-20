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
#include <skyrocket.hh>
#include <resources.hh>
#include <vector.hh>
#include <world.hh>

namespace World {
	Vector _cloudPos[CLOUDMESH + 1][CLOUDMESH + 1];
	float _cloudCoord[CLOUDMESH + 1][CLOUDMESH + 1][2];
	float _cloudBrightness[CLOUDMESH + 1][CLOUDMESH + 1];
	RGBColor _cloudColor[CLOUDMESH + 1][CLOUDMESH + 1];
};

void World::init() {
	if (Hack::drawClouds) {
		for (unsigned int j = 0; j <= CLOUDMESH; ++j) {
			for (unsigned int i = 0; i <= CLOUDMESH; ++i) {
				_cloudPos[i][j].x() = 40000.0f * float(i) / float(CLOUDMESH) - 20000.0f;
				_cloudPos[i][j].z() = 40000.0f * float(j) / float(CLOUDMESH) - 20000.0f;
				float x = std::abs(2 * float(i) / float(CLOUDMESH) - 1);
				float z = std::abs(2 * float(j) / float(CLOUDMESH) - 1);
				_cloudPos[i][j].y() = 2000.0f - 1000.0f * float(x * x + z * z);
				_cloudCoord[i][j][0] = -float(i) / float(CLOUDMESH / 6);
				_cloudCoord[i][j][1] = -float(j) / float(CLOUDMESH / 6);
				_cloudBrightness[i][j] = (_cloudPos[i][j].y() - 1000.0f) *
					0.00001f * Hack::ambient;
				if (_cloudBrightness[i][j] < 0.0f)
					_cloudBrightness[i][j] = 0.0f;
			}
		}
	}
}

void World::update() {
	if (Hack::drawClouds) {
		// Blow clouds (particles should get the same drift at 2000 feet)
		// Maximum wind is 100 and texture repeats every 6666.67 feet
		// so 100 * 0.00015 * 6666.67 = 100 ft/sec maximum windspeed.
		static float cloudShift = 0.0f;
		cloudShift += 0.00015f * Hack::wind * Common::elapsedTime;
		if (cloudShift > 1.0f)
			cloudShift -= 1.0f;
		for (unsigned int j = 0; j <= CLOUDMESH; ++j) {
			for (unsigned int i = 0; i <= CLOUDMESH; ++i) {
				_cloudColor[i][j].set(
					_cloudBrightness[i][j],
					_cloudBrightness[i][j],
					_cloudBrightness[i][j]
				);	// darken clouds
				_cloudCoord[i][j][0] = -float(i) * 1.0f / float(CLOUDMESH / 6) + cloudShift;
			}
		}
	}
}

void World::draw() {
	static float moonRotation = Common::randomFloat(360.0f);
	static float moonHeight = Common::randomFloat(40.0f) + 20.0f;

	glMatrixMode(GL_MODELVIEW);

	if (Hack::starDensity > 0) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glDisable(GL_BLEND);
		glCallList(Resources::DisplayLists::stars);
		glPopAttrib();
	}

	if (Hack::drawMoon) {
		glPushMatrix();
		glRotatef(moonRotation, 0, 1, 0);
		glRotatef(moonHeight, 1, 0, 0);
		glTranslatef(0.0f, 0.0f, -20000.0f);
		glCallList(Resources::DisplayLists::moon);
		glPopMatrix();
	}

	if (Hack::drawClouds) {
		glPushMatrix();
		glBindTexture(GL_TEXTURE_2D, Resources::Textures::cloud);
		glTranslatef(0.0f, 0.0f, 0.0f);
		for (unsigned int j = 0; j < CLOUDMESH; ++j) {
			glBegin(GL_TRIANGLE_STRIP);
			for (unsigned int i = 0; i <= CLOUDMESH; ++i) {
				glColor3fv(_cloudColor[i][j + 1].get());
				glTexCoord2fv(_cloudCoord[i][j + 1]);
				glVertex3fv(_cloudPos[i][j + 1].get());
				glColor3fv(_cloudColor[i][j].get());
				glTexCoord2fv(_cloudCoord[i][j]);
				glVertex3fv(_cloudPos[i][j].get());
			}
			glEnd();
		}
		glPopMatrix();
	}

	if (Hack::drawSunset) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glBlendFunc(GL_ONE, GL_ONE);
		glCallList(Resources::DisplayLists::sunset);
		glPopAttrib();
	}

	if (Hack::drawMoon && Hack::moonGlow > 0.0f) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glPushMatrix();
		float glow = Hack::moonGlow * 0.005f; // half of max possible value
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glRotatef(moonRotation, 0, 1, 0);
		glRotatef(moonHeight, 1, 0, 0);
		glTranslatef(0.0f, 0.0f, -20000.0f);
		glColor4f(1.0f, 1.0f, 1.0f, glow);
		glCallList(Resources::DisplayLists::moonGlow);
		glScalef(6000.0f, 6000.0f, 6000.0f);
		glColor4f(1.0f, 1.0f, 1.0f, glow * 0.7f);
		glCallList(Resources::DisplayLists::flares);
		glPopMatrix();
		glPopAttrib();
	}

	if (Hack::drawEarth) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glPushMatrix();
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, Resources::Textures::earthNear);
		glCallList(Resources::DisplayLists::earthNear);
		glBindTexture(GL_TEXTURE_2D, Resources::Textures::earthFar);
		glCallList(Resources::DisplayLists::earthFar);
		if (Hack::ambient <= 30.0f) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glBindTexture(GL_TEXTURE_2D, Resources::Textures::earthLight);
			glCallList(Resources::DisplayLists::earthLight);
		}
		glPopMatrix();
		glPopAttrib();
	}
}

void World::illuminateClouds(
	const Vector& pos, const RGBColor& newRGB,
	float brightness, float cloudDistanceSquared
) {
	float cloudDistance = std::sqrt(cloudDistanceSquared);

	unsigned int south =
		(unsigned int)(
			(pos.z() - cloudDistance) * 0.00005f *
			float(CLOUDMESH / 2) + CLOUDMESH / 2
		);
	unsigned int north =
		(unsigned int)(
			(pos.z() + cloudDistance) * 0.00005f *
			float(CLOUDMESH / 2) + 0.5f + CLOUDMESH / 2
		);
	unsigned int west =
		(unsigned int)(
			(pos.x() - cloudDistance) * 0.00005f *
			float(CLOUDMESH / 2) + CLOUDMESH / 2
		);
	unsigned int east =
		(unsigned int)(
			(pos.x() + cloudDistance) * 0.00005f *
			float(CLOUDMESH / 2) + 0.5f + CLOUDMESH / 2
		);
	for (unsigned int i = west; i <= east; ++i) {
		for (unsigned int j = south; j <= north; ++j) {
			float dSquared = (_cloudPos[i][j] - pos).lengthSquared();
			if (dSquared < cloudDistanceSquared) {
				float temp = (cloudDistanceSquared - dSquared) / cloudDistanceSquared;
				temp *= temp * brightness;
				_cloudColor[i][j] += newRGB * temp;
				_cloudColor[i][j].clamp();
			}
		}
	}
}
