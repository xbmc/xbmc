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
#include <oggsound.hh>
#include <pngimage.hh>
#include <resources.hh>
#include <skyrocket.hh>
#include <vector.hh>

#define STARTEXSIZE 512
#define MOONGLOWTEXSIZE 128
#define FLARESIZE 128
#define STARMESH 12

namespace Resources {
	namespace Sounds {
		Sound* boom1;
		Sound* boom2;
		Sound* boom3;
		Sound* boom4;
		Sound* launch1;
		Sound* launch2;
		Sound* nuke;
		Sound* popper;
		Sound* suck;
		Sound* whistle;

		void _init();
	};

	namespace Textures {
		GLuint cloud;
		GLuint stars;
		GLuint moon;
		GLuint moonGlow;
		GLuint sunset;
		GLuint earthNear;
		GLuint earthFar;
		GLuint earthLight;
		GLuint smoke[5];
		GLuint flare[4];

		void makeHeights(unsigned int, unsigned int, unsigned int*);
		void _init();
	};

	namespace DisplayLists {
		GLuint flares;
		GLuint rocket;
		GLuint smokes;
		GLuint stars;
		GLuint moon;
		GLuint moonGlow;
		GLuint sunset;
		GLuint earthNear;
		GLuint earthFar;
		GLuint earthLight;

		void _init();
	};

	void init(float, const Vector&, const Vector&, const UnitQuat&);
};

void Resources::Sounds::_init() {
	boom1 = new OGG("boom1.ogg", 1000.0f); Common::resources->manage(boom1);
	boom2 = new OGG("boom2.ogg", 1000.0f); Common::resources->manage(boom2);
	boom3 = new OGG("boom3.ogg", 1000.0f); Common::resources->manage(boom3);
	boom4 = new OGG("boom4.ogg", 1200.0f); Common::resources->manage(boom4);
	launch1 = new OGG("launch1.ogg", 10.0f); Common::resources->manage(launch1);
	launch2 = new OGG("launch2.ogg", 10.0f); Common::resources->manage(launch2);
	nuke = new OGG("nuke.ogg", 2000.0f); Common::resources->manage(nuke);
	popper = new OGG("popper.ogg", 700.0f, 2.5f); Common::resources->manage(popper);
	suck = new OGG("suck.ogg", 1500.0f); Common::resources->manage(suck);
	whistle = new OGG("whistle.ogg", 700.0f); Common::resources->manage(whistle);
}

void Resources::Textures::makeHeights(unsigned int first, unsigned int last,
		unsigned int* h) {
	unsigned int diff = last - first;
	if (diff <= 1) return;
	unsigned int middle = first + (last - first) / 2;
	int newHeight = (int(h[first]) + int(h[last])) / 2 +
		Common::randomInt(diff / 2) - (diff / 4);
	h[middle] = (newHeight < 1) ? 1 : newHeight;

	makeHeights(first, middle, h);
	makeHeights(middle, last, h);
}

void Resources::Textures::_init() {
	// Initialize cloud texture object even if clouds are not turned on.
	// Sunsets and shockwaves can also use cloud texture.
	PNG cloudPNG("cloud.png");
	cloud = Common::resources->genTexture(
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
		cloudPNG
	);

	if (Hack::starDensity > 0) {
		stdx::dim3<float, 3, STARTEXSIZE> starMap(STARTEXSIZE);

		for (unsigned int i = 0; i < (Hack::starDensity * 20); ++i) {
			unsigned int u = Common::randomInt(STARTEXSIZE - 4) + 2;
			unsigned int v = Common::randomInt(STARTEXSIZE - 4) + 2;
			RGBColor RGB(
				0.8627f + Common::randomFloat(0.1373f),
				0.8627f + Common::randomFloat(0.1373f),
				0.8627f + Common::randomFloat(0.1373f)
			);
			switch (Common::randomInt(3)) {
			case 0: RGB.r() = 1.0f; break;
			case 1: RGB.g() = 1.0f; break;
			case 2: RGB.b() = 1.0f; break;
			}
			switch (Common::randomInt(6)) { // different stars
			case 0: case 1: case 2: // small
				starMap(u, v, 0) = RGB.r() / 2.0f;
				starMap(u, v, 1) = RGB.g() / 2.0f;
				starMap(u, v, 2) = RGB.b() / 2.0f;
				RGB /= 3 + Common::randomInt(6);
				starMap(u + 1, v, 0) = starMap(u - 1, v, 0) =
					starMap(u, v + 1, 0) = starMap(u, v - 1, 0) = RGB.r();
				starMap(u + 1, v, 1) = starMap(u - 1, v, 1) =
					starMap(u, v + 1, 1) = starMap(u, v - 1, 1) = RGB.g();
				starMap(u + 1, v, 2) = starMap(u - 1, v, 2) =
					starMap(u, v + 1, 2) = starMap(u, v - 1, 2) = RGB.b();
				break;
			case 3: case 4: // medium
				starMap(u, v, 0) = RGB.r();
				starMap(u, v, 1) = RGB.g();
				starMap(u, v, 2) = RGB.b();
				RGB /= 2;
				starMap(u + 1, v, 0) = starMap(u - 1, v, 0) =
					starMap(u, v + 1, 0) = starMap(u, v - 1, 0) = RGB.r();
				starMap(u + 1, v, 1) = starMap(u - 1, v, 1) =
					starMap(u, v + 1, 1) = starMap(u, v - 1, 1) = RGB.g();
				starMap(u + 1, v, 2) = starMap(u - 1, v, 2) =
					starMap(u, v + 1, 2) = starMap(u, v - 1, 2) = RGB.b();
				break;
			case 5: // large
				starMap(u, v, 0) = RGB.r();
				starMap(u, v, 1) = RGB.g();
				starMap(u, v, 2) = RGB.b();
				starMap(u + 1, v + 1, 0) = starMap(u - 1, v + 1, 0) =
					starMap(u + 1, v - 1, 0) = starMap(u - 1, v - 1, 0) =
					RGB.r() / 4.0f;
				starMap(u + 1, v + 1, 1) = starMap(u - 1, v + 1, 1) =
					starMap(u + 1, v - 1, 1) = starMap(u - 1, v - 1, 1) =
					RGB.g() / 4.0f;
				starMap(u + 1, v + 1, 2) = starMap(u - 1, v + 1, 2) =
					starMap(u + 1, v - 1, 2) = starMap(u - 1, v - 1, 2) =
					RGB.b() / 4.0f;
				RGB *= 0.75;
				starMap(u + 1, v, 0) = starMap(u - 1, v, 0) =
					starMap(u, v + 1, 0) = starMap(u, v - 1, 0) = RGB.r();
				starMap(u + 1, v, 1) = starMap(u - 1, v, 1) =
					starMap(u, v + 1, 1) = starMap(u, v - 1, 1) = RGB.g();
				starMap(u + 1, v, 2) = starMap(u - 1, v, 2) =
					starMap(u, v + 1, 2) = starMap(u, v - 1, 2) = RGB.b();
			}
		}
		stars = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			3, STARTEXSIZE, STARTEXSIZE, GL_RGB, GL_FLOAT, &starMap.front()
		);
	}

	if (Hack::drawMoon) {
		moon = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			PNG("moon.png")
		);

		if (Hack::moonGlow > 0.0f) {
			stdx::dim3<float, 4, MOONGLOWTEXSIZE> moonGlowMap(MOONGLOWTEXSIZE);

			for (unsigned int i = 0; i < MOONGLOWTEXSIZE; ++i) {
				for (unsigned int j = 0; j < MOONGLOWTEXSIZE; ++j) {
					float u = 2 * float(i) / float(MOONGLOWTEXSIZE) - 1;
					float v = 2 * float(j) / float(MOONGLOWTEXSIZE) - 1;
					float temp1 = Common::clamp(
						4.0f * ((u * u) + (v * v)) * (1.0f - ((u * u) + (v * v))),
						0.0f, 1.0f
					);
					temp1 = temp1 * temp1 * temp1 * temp1;
					u *= 1.2f;
					v *= 1.2f;
					float temp2 = Common::clamp(
						4.0f * ((u * u) + (v * v)) * (1.0f - ((u * u) + (v * v))),
						0.0f, 1.0f
					);
					temp2 = temp2 * temp2 * temp2 * temp2;
					u *= 1.25f;
					v *= 1.25f;
					float temp3 = Common::clamp(
						4.0f * ((u * u) + (v * v)) * (1.0f - ((u * u) + (v * v))),
						0.0f, 1.0f
					);
					temp3 = temp3 * temp3 * temp3 * temp3;
					moonGlowMap(i, j, 0) = temp1 * 0.4f + temp2 * 0.4f + temp3 * 0.48f;
					moonGlowMap(i, j, 1) = temp1 * 0.4f + temp2 * 0.48f + temp3 * 0.38f;
					moonGlowMap(i, j, 2) = temp1 * 0.48f + temp2 * 0.4f + temp3 * 0.38f;
					moonGlowMap(i, j, 3) = temp1 * 0.48f + temp2 * 0.48f + temp3 * 0.48f;
				}
			}
			moonGlow = Common::resources->genTexture(
				GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
				4, MOONGLOWTEXSIZE, MOONGLOWTEXSIZE, GL_RGBA, GL_FLOAT,
				&moonGlowMap.front()
			);
		}
	}

	if (Hack::drawSunset) {
		RGBColor RGB;
		if (Common::randomInt(3))
			RGB.r() = 0.2353f + Common::randomFloat(0.1647f);
		else
			RGB.r() = Common::randomFloat(0.4f);
		RGB.g() = Common::randomFloat(RGB.r());
		if (RGB.g() < 0.0196f)
			RGB.b() = 0.0392f - Common::randomFloat(RGB.r());

		stdx::dim3<float, 3> sunsetMap(cloudPNG.width(), cloudPNG.height());
		for (unsigned int i = 0; i < cloudPNG.width(); ++i) {
			for (unsigned int j = 0; j < cloudPNG.height(); ++j) {
				sunsetMap(i, j, 0) = RGB.r();
				sunsetMap(i, j, 1) = RGB.g();
				sunsetMap(i, j, 2) = RGB.b();
			}
		}

		// Maybe clouds in sunset
		if (Common::randomInt(3)) {
			unsigned int xOffset = Common::randomInt(cloudPNG.width());
			unsigned int yOffset = Common::randomInt(cloudPNG.height());
			for (unsigned int i = 0; i < cloudPNG.width(); ++i) {
				for (unsigned int j = 0; j < cloudPNG.height(); ++j) {
					unsigned int x = (i + xOffset) % cloudPNG.width();
					unsigned int y = (j + yOffset) % cloudPNG.height();
					float cloudInfluence = cloudPNG(x, y).g();
					sunsetMap(i, j, 0) *= cloudInfluence;
					cloudInfluence *= cloudPNG(x, y).r();
					sunsetMap(i, j, 1) *= cloudInfluence;
				}
			}
		}

		std::vector<unsigned int> mountains(cloudPNG.width() + 1);
		mountains[0] = mountains[cloudPNG.width()] = Common::randomInt(10) + 5;
		makeHeights(0, cloudPNG.width(), &mountains.front());
		for (unsigned int i = 0; i < cloudPNG.width(); ++i) {
			for (unsigned int j = 0; j <= mountains[i]; ++j)
				sunsetMap(i, j, 0) = sunsetMap(i, j, 1) = sunsetMap(i, j, 2) = 0.0f;
			sunsetMap(i, mountains[i] + 1, 0) /= 4.0f;
			sunsetMap(i, mountains[i] + 1, 1) /= 4.0f;
			sunsetMap(i, mountains[i] + 1, 2) /= 4.0f;
			sunsetMap(i, mountains[i] + 2, 0) /= 2.0f;
			sunsetMap(i, mountains[i] + 2, 1) /= 2.0f;
			sunsetMap(i, mountains[i] + 2, 2) /= 2.0f;
			sunsetMap(i, mountains[i] + 3, 0) *= 0.75f;
			sunsetMap(i, mountains[i] + 3, 1) *= 0.75f;
			sunsetMap(i, mountains[i] + 3, 2) *= 0.75f;
		}

		sunset = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP, GL_CLAMP,
			3, cloudPNG.width(), cloudPNG.height(), GL_RGB, GL_FLOAT,
			&sunsetMap.front()
		);
	}

	if (Hack::drawEarth) {
		earthNear = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP, GL_CLAMP,
			PNG("earth-near.png")
		);
		earthFar = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP, GL_CLAMP,
			PNG("earth-far.png")
		);
		earthLight = Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP, GL_CLAMP,
			PNG("earth-light.png")
		);
	}

	for (unsigned int i = 0; i < 5; i++) {
		smoke[i] = Common::resources->genTexture(
			GL_NEAREST, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			PNG(stdx::oss() << "smoke" << (char)(i + '1') << ".png")
		);
	}

	stdx::dim3<GLubyte, 4, FLARESIZE> flareMap[4];
	for (unsigned int i = 0; i < 4; ++i)
		flareMap[i].resize(FLARESIZE);

	for (int i = 0; i < FLARESIZE; ++i) {
		for (int j = 0; j < FLARESIZE; ++j) {
			float x = float(i - FLARESIZE / 2) / float(FLARESIZE / 2);
			float y = float(j - FLARESIZE / 2) / float(FLARESIZE / 2);
			float temp;

			// Basic flare
			flareMap[0](i, j, 0) = 255;
			flareMap[0](i, j, 1) = 255;
			flareMap[0](i, j, 2) = 255;
			temp = Common::clamp(
				1.0f - ((x * x) + (y * y)),
				0.0f, 1.0f
			);
			flareMap[0](i, j, 3) = GLubyte(255.0f * temp * temp);

			// Flattened sphere
			flareMap[1](i, j, 0) = 255;
			flareMap[1](i, j, 1) = 255;
			flareMap[1](i, j, 2) = 255;
			temp = Common::clamp(
				2.5f * (1.0f - ((x * x) + (y * y))),
				0.0f, 1.0f
			);
			flareMap[1](i, j, 3) = GLubyte(255.0f * temp);

			// Torus
			flareMap[2](i, j, 0) = 255;
			flareMap[2](i, j, 1) = 255;
			flareMap[2](i, j, 2) = 255;
			temp = Common::clamp(
				4.0f * ((x * x) + (y * y)) * (1.0f - ((x * x) + (y * y))),
				0.0f, 1.0f
			);
			temp = temp * temp * temp * temp;
			flareMap[2](i, j, 3) = GLubyte(255.0f * temp);

			// Kick-ass!
			x = std::abs(x);
			y = std::abs(y);
			float xy = x * y;
			flareMap[3](i, j, 0) = 255;
			flareMap[3](i, j, 1) = 255;
			temp = Common::clamp(
				0.14f * (1.0f - ((x > y) ? x : y)) / ((xy > 0.05f) ? xy : 0.05f),
				0.0f, 1.0f
			);
			flareMap[3](i, j, 2) = GLubyte(255.0f * temp);
			temp = Common::clamp(
				0.1f * (1.0f - ((x > y) ? x : y)) / ((xy > 0.1f) ? xy : 0.1f),
				0.0f, 1.0f
			);
			flareMap[3](i, j, 3) = GLubyte(255.0f * temp);
		}
	}

	for (unsigned int i = 0; i < 4; ++i)
		flare[i] = Common::resources->genTexture(
			GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			4, FLARESIZE, FLARESIZE, GL_RGBA, GL_UNSIGNED_BYTE,
			&flareMap[i].front(), false
		);
}

void Resources::DisplayLists::_init() {
	flares = Common::resources->genLists(4);
	for (unsigned int i = 0; i < 4; ++i) {
		glNewList(flares + i, GL_COMPILE);
			glBindTexture(GL_TEXTURE_2D, Resources::Textures::flare[i]);
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

	rocket = Common::resources->genLists(1);
	glNewList(rocket, GL_COMPILE);
		// Draw rocket (10-sided cylinder)
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(0.075f, 0.0f, 0.0f);
			glVertex3f(0.075f, 1.0f, 0.0f);
			glVertex3f(0.060676f, 0.0f, 0.033504f);
			glVertex3f(0.060676f, 1.0f, 0.033504f);
			glVertex3f(0.023176f, 0.0f, 0.071329f);
			glVertex3f(0.023176f, 1.0f, 0.071329f);
			glVertex3f(-0.023176f, 0.0f, 0.071329f);
			glVertex3f(-0.023176f, 1.0f, 0.071329f);
			glVertex3f(-0.060676f, 0.0f, 0.033504f);
			glVertex3f(-0.060676f, 1.0f, 0.033504f);
			glVertex3f(-0.075f, 0.0f, 0.0f);
			glVertex3f(-0.075f, 1.0f, 0.0f);
			glVertex3f(-0.060676f, 0.0f, -0.033504f);
			glVertex3f(-0.060676f, 1.0f, -0.033504f);
			glVertex3f(-0.023176f, 0.0f, -0.071329f);
			glVertex3f(-0.023176f, 1.0f, -0.071329f);
			glVertex3f(0.023176f, 0.0f, -0.071329f);
			glVertex3f(0.023176f, 1.0f, -0.071329f);
			glVertex3f(0.060676f, 0.0f, -0.033504f);
			glVertex3f(0.060676f, 1.0f, -0.033504f);
			glVertex3f(0.075f, 0.0f, 0.0f);
			glVertex3f(0.075f, 1.0f, 0.0f);
		glEnd();
		// bottom of rocket
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(0.075f, 0.0f, 0.0f);
			glVertex3f(0.060676f, 0.0f, 0.033504f);
			glVertex3f(0.060676f, 0.0f, -0.033504f);
			glVertex3f(0.023176f, 0.0f, 0.071329f);
			glVertex3f(0.023176f, 0.0f, -0.071329f);
			glVertex3f(-0.023176f, 0.0f, 0.071329f);
			glVertex3f(-0.023176f, 0.0f, -0.071329f);
			glVertex3f(-0.060676f, 0.0f, 0.033504f);
			glVertex3f(-0.060676f, 0.0f, -0.033504f);
			glVertex3f(-0.075f, 0.0f, 0.0f);
		glEnd();
		// top of rocket
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(0.075f, 1.0f, 0.0f);
			glVertex3f(0.060676f, 1.0f, -0.033504f);
			glVertex3f(0.060676f, 1.0f, 0.033504f);
			glVertex3f(0.023176f, 1.0f, -0.071329f);
			glVertex3f(0.023176f, 1.0f, 0.071329f);
			glVertex3f(-0.023176f, 1.0f, -0.071329f);
			glVertex3f(-0.023176f, 1.0f, 0.071329f);
			glVertex3f(-0.060676f, 1.0f, -0.033504f);
			glVertex3f(-0.060676f, 1.0f, 0.033504f);
			glVertex3f(-0.075f, 1.0f, 0.0f);
		glEnd();
	glEndList();

	smokes = Common::resources->genLists(5);
	for (unsigned int i = 0; i < 5; i++) {
		glNewList(smokes + i, GL_COMPILE);
			glBindTexture(GL_TEXTURE_2D, Resources::Textures::smoke[i]);
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

	if (Hack::starDensity > 0) {
		Vector starPos[STARMESH + 1][STARMESH / 2];
		float starCoord[STARMESH + 1][STARMESH / 2][2];
		float starBrightness[STARMESH + 1][STARMESH / 2];
		for (unsigned int j = 0; j < STARMESH / 2; ++j) {
			float y = std::sin(M_PI_2 * float(j) / float(STARMESH / 2));
			for (unsigned int i = 0; i <= STARMESH; ++i) {
				float x = std::cos(M_PI * 2 * float(i) / float(STARMESH)) *
					std::cos(M_PI_2 * float(j) / float(STARMESH / 2));
				float z = std::sin(M_PI * 2 * float(i) / float(STARMESH)) *
					std::cos(M_PI_2 * float(j) / float(STARMESH / 2));
				starPos[i][j].set(
					x * 20000.0f,
					1500.0f + 18500.0f * y,
					z * 20000.0f
				);
				starCoord[i][j][0] = 1.2f * x * (2.5f - y);
				starCoord[i][j][1] = 1.2f * z * (2.5f - y);
				starBrightness[i][j] = (starPos[i][j].y() < 1501.0f) ? 0.0f : 1.0f;
			}
		}

		stars = Common::resources->genLists(1);
		glNewList(stars, GL_COMPILE);
			glBindTexture(GL_TEXTURE_2D, Resources::Textures::stars);
			for (unsigned int j = 0; j < (STARMESH / 2 - 1); ++j) {
				glBegin(GL_TRIANGLE_STRIP);
				for (unsigned int i = 0; i <= STARMESH; ++i) {
					glColor3f(
						starBrightness[i][j + 1],
						starBrightness[i][j + 1],
						starBrightness[i][j + 1]
					);
					glTexCoord2fv(starCoord[i][j + 1]);
					glVertex3fv(starPos[i][j + 1].get());
					glColor3f(
						starBrightness[i][j],
						starBrightness[i][j],
						starBrightness[i][j]
					);
					glTexCoord2fv(starCoord[i][j]);
					glVertex3fv(starPos[i][j].get());
				}
				glEnd();
			}
			glBegin(GL_TRIANGLE_FAN);
			glColor3f(1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(0.0f, 20000.0f, 0.0f);
			for (unsigned int i = 0; i <= STARMESH; ++i) {
				glColor3f(
					starBrightness[i][STARMESH / 2 - 1],
					starBrightness[i][STARMESH / 2 - 1],
					starBrightness[i][STARMESH / 2 - 1]
				);
				glTexCoord2fv(starCoord[i][STARMESH / 2 - 1]);
				glVertex3fv(starPos[i][STARMESH / 2 - 1].get());
			}
			glEnd();
		glEndList();
	}

	if (Hack::drawMoon) {
		moon = Common::resources->genLists(1);
		glNewList(moon, GL_COMPILE);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glBindTexture(GL_TEXTURE_2D, Resources::Textures::moon);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(-800.0f, -800.0f, 0.0f);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(800.0f, -800.0f, 0.0f);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(-800.0f, 800.0f, 0.0f);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(800.0f, 800.0f, 0.0f);
			glEnd();
		glEndList();

		if (Hack::moonGlow > 0.0f) {
			moonGlow = Common::resources->genLists(1);
			glNewList(moonGlow, GL_COMPILE);
				glBindTexture(GL_TEXTURE_2D, Resources::Textures::moonGlow);
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(-7000.0f, -7000.0f, 0.0f);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f(7000.0f, -7000.0f, 0.0f);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(-7000.0f, 7000.0f, 0.0f);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(7000.0f, 7000.0f, 0.0f);
				glEnd();
			glEndList();
		}
	}

	if (Hack::drawSunset) {
		float vert[6] = { 0.0f, 7654.0f, 8000.0f, 14142.0f, 18448.0f, 20000.0f };
		sunset = Common::resources->genLists(1);
		glNewList(sunset, GL_COMPILE);
			glBindTexture(GL_TEXTURE_2D, Resources::Textures::sunset);
			glBegin(GL_TRIANGLE_STRIP);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(vert[0], vert[2], vert[5]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(vert[0], vert[0], vert[5]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.125f);
				glVertex3f(-vert[1], vert[2], vert[4]);
				glColor3f(0.25f, 0.25f, 0.25f);
				glTexCoord2f(0.0f, 0.125f);
				glVertex3f(-vert[1], vert[0], vert[4]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.25f);
				glVertex3f(-vert[3], vert[2], vert[3]);
				glColor3f(0.5f, 0.5f, 0.5f);
				glTexCoord2f(0.0f, 0.25f);
				glVertex3f(-vert[3], vert[0], vert[3]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.375f);
				glVertex3f(-vert[4], vert[2], vert[1]);
				glColor3f(0.75f, 0.75f, 0.75f);
				glTexCoord2f(0.0f, 0.375f);
				glVertex3f(-vert[4], vert[0], vert[1]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.5f);
				glVertex3f(-vert[5], vert[2], vert[0]);
				glColor3f(1.0f, 1.0f, 1.0f);
				glTexCoord2f(0.0f, 0.5f);
				glVertex3f(-vert[5], vert[0], vert[0]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.625f);
				glVertex3f(-vert[4], vert[2], -vert[1]);
				glColor3f(0.75f, 0.75f, 0.75f);
				glTexCoord2f(0.0f, 0.625f);
				glVertex3f(-vert[4], vert[0], -vert[1]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.75f);
				glVertex3f(-vert[3], vert[2], -vert[3]);
				glColor3f(0.5f, 0.5f, 0.5f);
				glTexCoord2f(0.0f, 0.75f);
				glVertex3f(-vert[3], vert[0], -vert[3]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.875f);
				glVertex3f(-vert[1], vert[2], -vert[4]);
				glColor3f(0.25f, 0.25f, 0.25f);
				glTexCoord2f(0.0f, 0.875f);
				glVertex3f(-vert[1], vert[0], -vert[4]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(vert[0], vert[2], -vert[5]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(vert[0], vert[0], -vert[5]);
			glEnd();
		glEndList();
	}

	if (Hack::drawEarth) {
		float lit[] = {
			Hack::ambient * 0.01f,
			Hack::ambient * 0.01f,
			Hack::ambient * 0.01f
		};
		float unlit[] = { 0.0f, 0.0f, 0.0f };
		float vert[] = { 839.68f, 8396.8f };
		float tex[] = { 0.0f, 0.45f, 0.55f, 1.0f };
		earthNear = Common::resources->genLists(1);
		glNewList(earthNear, GL_COMPILE);
			glColor3fv(lit);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(tex[0], tex[0]);
				glVertex3f(-vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[0], tex[3]);
				glVertex3f(-vert[0], 0.0f, vert[0]);
				glTexCoord2f(tex[3], tex[0]);
				glVertex3f(vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[3], tex[3]);
				glVertex3f(vert[0], 0.0f, vert[0]);
			glEnd();
		glEndList();
		earthFar = Common::resources->genLists(1);
		glNewList(earthFar, GL_COMPILE);
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[1], tex[1]);
				glVertex3f(-vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[2], tex[1]);
				glVertex3f(vert[0], 0.0f, -vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[0], tex[0]);
				glVertex3f(-vert[1], 0.0f, -vert[1]);
				glTexCoord2f(tex[3], tex[0]);
				glVertex3f(vert[1], 0.0f, -vert[1]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[1], tex[2]);
				glVertex3f(-vert[0], 0.0f, vert[0]);
				glTexCoord2f(tex[1], tex[1]);
				glVertex3f(-vert[0], 0.0f, -vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[0], tex[3]);
				glVertex3f(-vert[1], 0.0f, vert[1]);
				glTexCoord2f(tex[0], tex[0]);
				glVertex3f(-vert[1], 0.0f, -vert[1]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[2], tex[2]);
				glVertex3f(vert[0], 0.0f, vert[0]);
				glTexCoord2f(tex[1], tex[2]);
				glVertex3f(-vert[0], 0.0f, vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[3], tex[3]);
				glVertex3f(vert[1], 0.0f, vert[1]);
				glTexCoord2f(tex[0], tex[3]);
				glVertex3f(-vert[1], 0.0f, vert[1]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[2], tex[1]);
				glVertex3f(vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[2], tex[2]);
				glVertex3f(vert[0], 0.0f, vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[3], tex[0]);
				glVertex3f(vert[1], 0.0f, -vert[1]);
				glTexCoord2f(tex[3], tex[3]);
				glVertex3f(vert[1], 0.0f, vert[1]);
			glEnd();
		glEndList();
		earthLight = Common::resources->genLists(1);
		glNewList(earthLight, GL_COMPILE);
			lit[0] = lit[1] = lit[2] = 0.4f;
			glColor3fv(lit);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(tex[1], tex[1]);
				glVertex3f(-vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[1], tex[2]);
				glVertex3f(-vert[0], 0.0f, vert[0]);
				glTexCoord2f(tex[2], tex[1]);
				glVertex3f(vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[2], tex[2]);
				glVertex3f(vert[0], 0.0f, vert[0]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[1], tex[1]);
				glVertex3f(-vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[2], tex[1]);
				glVertex3f(vert[0], 0.0f, -vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[0], tex[0]);
				glVertex3f(-vert[1], 0.0f, -vert[1]);
				glTexCoord2f(tex[3], tex[0]);
				glVertex3f(vert[1], 0.0f, -vert[1]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[1], tex[2]);
				glVertex3f(-vert[0], 0.0f, vert[0]);
				glTexCoord2f(tex[1], tex[1]);
				glVertex3f(-vert[0], 0.0f, -vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[0], tex[3]);
				glVertex3f(-vert[1], 0.0f, vert[1]);
				glTexCoord2f(tex[0], tex[0]);
				glVertex3f(-vert[1], 0.0f, -vert[1]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[2], tex[2]);
				glVertex3f(vert[0], 0.0f, vert[0]);
				glTexCoord2f(tex[1], tex[2]);
				glVertex3f(-vert[0], 0.0f, vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[3], tex[3]);
				glVertex3f(vert[1], 0.0f, vert[1]);
				glTexCoord2f(tex[0], tex[3]);
				glVertex3f(-vert[1], 0.0f, vert[1]);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor3fv(lit);
				glTexCoord2f(tex[2], tex[1]);
				glVertex3f(vert[0], 0.0f, -vert[0]);
				glTexCoord2f(tex[2], tex[2]);
				glVertex3f(vert[0], 0.0f, vert[0]);
				glColor3fv(unlit);
				glTexCoord2f(tex[3], tex[0]);
				glVertex3f(vert[1], 0.0f, -vert[1]);
				glTexCoord2f(tex[3], tex[3]);
				glVertex3f(vert[1], 0.0f, vert[1]);
			glEnd();
		glEndList();
	}
}

void Resources::init() {
	if (Hack::volume > 0.0f) {
		try {
			Sound::init(
				Hack::openalSpec, Hack::volume, Hack::speedOfSound,
				&Hack::cameraPos, &Hack::cameraVel, &Hack::cameraDir
			);
		} catch (Sound::Unavailable e) {
			Hack::volume = 0.0f;
		} catch (Common::Exception e) {
			WARN(e);
			Hack::volume = 0.0f;
		}
	}
	if (Hack::volume > 0.0f) {
		try {
			Sounds::_init();
		} catch (Sound::Unavailable e) {
			Hack::volume = 0.0f;
		}
	}
	Textures::_init();
	DisplayLists::_init();
}
