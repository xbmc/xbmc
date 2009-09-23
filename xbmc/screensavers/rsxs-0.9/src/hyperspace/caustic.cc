/*
 * Really Slick XScreenSavers
 * Copyright (C) 2002-2006  Michael Chapman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundAT2on.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * FoundAT2on, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 * This is a Linux port of the Really Slick Screensavers,
 * Copyright (C) 2005 Terence M. Welsh, available from www.reallyslick.com
 */
#include <common.hh>

#include <caustic.hh>
#include <hyperspace.hh>

#define CAUSTIC_RESOLUTION 100
#define CAUSTIC_SIZE       256
#define CAUSTIC_DEPTH      1.0f
#define CAUSTIC_AMPLITUDE  0.01f
#define CAUSTIC_REFRACTION 20.0f

namespace CausticTextures {
	std::vector<GLuint> _textures;

	void draw(
		const stdx::dim2<float, CAUSTIC_RESOLUTION + 1>&,
		const float*, const float*,
		const stdx::dim2<std::pair<float, float>, CAUSTIC_RESOLUTION + 1>&,
		unsigned int, unsigned int, unsigned int, unsigned int
	);
};

void CausticTextures::init() {
	unsigned int size = CAUSTIC_SIZE;
	if (Common::width < size)  size = Common::width;
	if (Common::height < size) size = Common::height;

	float x[CAUSTIC_RESOLUTION + 1];
	float z[CAUSTIC_RESOLUTION + 1];

	stdx::dim2<float, CAUSTIC_RESOLUTION> y[Hack::frames];
	for (unsigned int i = 0; i < Hack::frames; ++i)
		y[i].resize(CAUSTIC_RESOLUTION);
	stdx::dim2<std::pair<float, float>, CAUSTIC_RESOLUTION + 1> xz(CAUSTIC_RESOLUTION + 1);
	stdx::dim2<float, CAUSTIC_RESOLUTION + 1> intensity(CAUSTIC_RESOLUTION + 1);

	// set x and z geometry positions
	for (unsigned int i = 0; i <= CAUSTIC_RESOLUTION; ++i) {
		x[i] = z[i] = float(i) / float(CAUSTIC_RESOLUTION);
	}

	// set y geometry positions (altitudes)
	for (unsigned int k = 0; k < Hack::frames; ++k) {
		float offset = M_PI * 2.0f * float(k) / float(Hack::frames);
		for (unsigned int i = 0; i < CAUSTIC_RESOLUTION; ++i) {
			float xx = M_PI * 2.0f * float(i) / float(CAUSTIC_RESOLUTION);
			for (unsigned int j = 0; j < CAUSTIC_RESOLUTION; ++j) {
				float zz = M_PI * 2.0f * float(j) / float(CAUSTIC_RESOLUTION);
				y[k](i, j) = CAUSTIC_AMPLITUDE * (
					0.08f * std::cos(xx * 2.0f + offset)
					+ 0.06f * std::cos(-1.0f * xx + 2.0f * zz + offset)
					+ 0.04f * std::cos(-2.0f * xx - 3.0f * zz + offset)
					+ 0.01f * std::cos(xx - 7.0f * zz - 2.0f * offset)
					+ 0.01f * std::cos(3.0f * xx + 5.0f * zz + offset)
					+ 0.01f * std::cos(9.0f * xx + zz - offset)
					+ 0.005f * std::cos(11.0f * xx + 7.0f * zz - offset)
					+ 0.005f * std::cos(4.0f * xx - 13.0f * zz + offset)
					+ 0.003f * std::cos(19.0f * xx - 9.0f * zz - offset)
				);
			}
		}
	}

	// prepare to draw textures
	Common::flush();
	glXWaitX();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_FOG);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRotatef(-90.0f, 1, 0, 0);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glReadBuffer(GL_BACK);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	std::vector<unsigned char> bitmap(size * size * 3);

	// project vertices and create textures

	// reciprocal of vertical component of light ray
	float recvert = float(CAUSTIC_RESOLUTION) * 0.5f;
	for (unsigned int k = 0; k < Hack::frames; ++k) {
		// compute projected offsets
		// (this uses surface normals, not actual refractions, but it's faster this way)
		for (unsigned int i = 0; i < CAUSTIC_RESOLUTION; ++i) {
			for (unsigned int j = 0; j < CAUSTIC_RESOLUTION; ++j) {
				unsigned int minus, plus;

				minus = (i == 0 ? CAUSTIC_RESOLUTION - 1 : i - 1);
				plus  = (i == CAUSTIC_RESOLUTION - 1 ? 0 : i + 1);
				xz(i, j).first = (y[k](plus, j) - y[k](minus, j)) * recvert * (CAUSTIC_DEPTH + y[k](i, j));

				minus = (j == 0 ? CAUSTIC_RESOLUTION - 1 : j - 1);
				plus  = (j == CAUSTIC_RESOLUTION - 1 ? 0 : j + 1);
				xz(i, j).second = (y[k](i, plus) - y[k](i, minus)) * recvert * (CAUSTIC_DEPTH + y[k](i, j));
			}
		}

		// copy offsets to edges of xz array
		for (unsigned int i = 0; i < CAUSTIC_RESOLUTION; ++i)
			xz(i, CAUSTIC_RESOLUTION) = xz(i, 0);
		for (unsigned int j = 0; j <= CAUSTIC_RESOLUTION; ++j)
			xz(CAUSTIC_RESOLUTION, j) = xz(0, j);

		// compute light intensities
		float space = 1.0f / float(CAUSTIC_RESOLUTION);
		for (unsigned int i = 0; i < CAUSTIC_RESOLUTION; ++i) {
			for (unsigned int j = 0; j < CAUSTIC_RESOLUTION; ++j) {
				unsigned int xminus = (i == 0 ? CAUSTIC_RESOLUTION - 1 : i - 1);
				unsigned int xplus  = (i == CAUSTIC_RESOLUTION - 1 ? 0 : i + 1);
				unsigned int zminus = (j == 0 ? CAUSTIC_RESOLUTION - 1 : j - 1);
				unsigned int zplus  = (j == CAUSTIC_RESOLUTION - 1 ? 0 : j + 1);
				// this assumes nominal light intensity is 0.25
				intensity(i, j) = (1.0f / (float(CAUSTIC_RESOLUTION) * float(CAUSTIC_RESOLUTION)))
					/ ((std::abs(xz(xplus, j).first - xz(i, j).first + space)
					+ std::abs(xz(i, j).first - xz(xminus, j).first + space))
					* (std::abs(xz(i, zplus).second - xz(i, j).second + space)
					+ std::abs(xz(i, j).second - xz(i, zminus).second + space)))
					- 0.125f;
				if (intensity(i, j) > 1.0f)
					intensity(i, j) = 1.0f;
			}
		}

		// copy intensities to edges of intensity array
		for (unsigned int i = 0; i < CAUSTIC_RESOLUTION; ++i)
			intensity(i, CAUSTIC_RESOLUTION) = intensity(i, 0);
		for (unsigned int j = 0; j <= CAUSTIC_RESOLUTION; ++j)
			intensity(CAUSTIC_RESOLUTION, j) = intensity(0, j);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -0.5f, 0.5f);
		glViewport(
			(Common::width - size) >> 1,
			(Common::height - size) >> 1,
			size, size
		);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		// draw texture
		glClear(GL_COLOR_BUFFER_BIT);
		// draw most of texture
		draw(intensity, x, z, xz, 0, CAUSTIC_RESOLUTION, 0, CAUSTIC_RESOLUTION);
		// draw edges of texture that wrap around from opposite sides
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glTranslatef(-1.0f, 0.0f, 0.0f);
			draw(intensity, x, z, xz, CAUSTIC_RESOLUTION * 9 / 10, CAUSTIC_RESOLUTION, 0, CAUSTIC_RESOLUTION);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(1.0f, 0.0f, 0.0f);
			draw(intensity, x, z, xz, 0, CAUSTIC_RESOLUTION / 10, 0, CAUSTIC_RESOLUTION);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(0.0f, 0.0f, -1.0f);
			draw(intensity, x, z, xz, 0, CAUSTIC_RESOLUTION, CAUSTIC_RESOLUTION * 9 / 10, CAUSTIC_RESOLUTION);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(0.0f, 0.0f, 1.0f);
			draw(intensity, x, z, xz, 0, CAUSTIC_RESOLUTION, 0, CAUSTIC_RESOLUTION / 10);
		glPopMatrix();
		// draw corners too
		glPushMatrix();
			glTranslatef(-1.0f, 0.0f, -1.0f);
			draw(intensity, x, z, xz, CAUSTIC_RESOLUTION * 9 / 10, CAUSTIC_RESOLUTION, CAUSTIC_RESOLUTION * 9 / 10, CAUSTIC_RESOLUTION);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(1.0f, 0.0f, -1.0f);
			draw(intensity, x, z, xz, 0, CAUSTIC_RESOLUTION / 10, CAUSTIC_RESOLUTION * 9 / 10, CAUSTIC_RESOLUTION);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(-1.0f, 0.0f, 1.0f);
			draw(intensity, x, z, xz, CAUSTIC_RESOLUTION * 9 / 10, CAUSTIC_RESOLUTION, 0, CAUSTIC_RESOLUTION / 10);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(1.0f, 0.0f, 1.0f);
			draw(intensity, x, z, xz, 0, CAUSTIC_RESOLUTION / 10, 0, CAUSTIC_RESOLUTION / 10);
		glPopMatrix();

		// read back texture
		glReadPixels(
			(Common::width - size) >> 1,
			(Common::height - size) >> 1,
			size, size,
			GL_RGB, GL_UNSIGNED_BYTE, &bitmap.front()
		);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, Common::width, 0.0f, Common::height, -0.5f, 0.5f);
		glViewport(0, 0, Common::width, Common::height);

		unsigned int left   = ((Common::width - size) >> 1) - 5;
		unsigned int right  = left + size + 10;
		unsigned int bottom = ((Common::height - size) >> 1) - 5;
		unsigned int top    = bottom + size + 10;
		unsigned int barBottom = bottom - 10;
		unsigned int barTop    = barBottom - 20;

		glBlendFunc(GL_ONE, GL_ZERO);
		glLineWidth(2.0f);
		glBegin(GL_LINE_STRIP);
			glColor3f(0.0f, 0.4f, 0.0f);
			glVertex3f(left, 0.0f, bottom);
			glVertex3f(right, 0.0f, bottom);
			glVertex3f(right, 0.0f, top);
			glVertex3f(left, 0.0f, top);
			glVertex3f(left, 0.0f, bottom);
		glEnd();
		glBegin(GL_QUADS);
			glColor3f(0.0f, 0.2f, 0.0f);
			glVertex3f(left, 0.0f, barBottom);
			glVertex3f(left + (k + 1) * float(right - left) / float(Hack::frames), 0.0f, barBottom);
			glVertex3f(left + (k + 1) * float(right - left) / float(Hack::frames), 0.0f, barTop);
			glVertex3f(left, 0.0f, barTop);
		glEnd();
		glBegin(GL_LINE_STRIP);
			glColor3f(0.0f, 0.4f, 0.0f);
			glVertex3f(left, 0.0f, barBottom);
			glVertex3f(right, 0.0f, barBottom);
			glVertex3f(right, 0.0f, barTop);
			glVertex3f(left, 0.0f, barTop);
			glVertex3f(left, 0.0f, barBottom);
		glEnd();

		Common::flush();

		// create texture object
		_textures.push_back(Common::resources->genTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
			3, size, size, GL_RGB, GL_UNSIGNED_BYTE, &bitmap.front()
		));
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void CausticTextures::draw(
	const stdx::dim2<float, CAUSTIC_RESOLUTION + 1>& intensity,
	const float* x, const float* z,
	const stdx::dim2<std::pair<float, float>, CAUSTIC_RESOLUTION + 1>& xz,
	unsigned int xLo, unsigned int xHi, unsigned int zLo, unsigned int zHi
) {
	for (unsigned int j = zLo; j < zHi; ++j) {
		// red
		float mult = 1.0f - CAUSTIC_REFRACTION / float(CAUSTIC_RESOLUTION);
		glBegin(GL_TRIANGLE_STRIP);
		for (unsigned int i = xLo; i <= xHi; ++i) {
			glColor3f(intensity(i, j + 1), 0.0f, 0.0f);
			glVertex3f(x[i] + xz(i, j + 1).first * mult, 0.0f, z[j + 1] + xz(i, j + 1).second * mult);
			glColor3f(intensity(i, j), 0.0f, 0.0f);
			glVertex3f(x[i] + xz(i, j).first * mult, 0.0f, z[j] + xz(i, j).second * mult);
		}
		glEnd();
		// green
		glBegin(GL_TRIANGLE_STRIP);
		for(unsigned int i = xLo; i <= xHi; ++i) {
			glColor3f(0.0f, intensity(i, j + 1), 0.0f);
			glVertex3f(x[i] + xz(i, j + 1).first, 0.0f, z[j + 1] + xz(i, j + 1).second);
			glColor3f(0.0f, intensity(i, j), 0.0f);
			glVertex3f(x[i] + xz(i, j).first, 0.0f, z[j] + xz(i, j).second);
		}
		glEnd();
		// blue
		mult = 1.0f + CAUSTIC_REFRACTION / float(CAUSTIC_RESOLUTION);
		glBegin(GL_TRIANGLE_STRIP);
		for(unsigned int i = xLo; i <= xHi; ++i) {
			glColor3f(0.0f, 0.0f, intensity(i, j + 1));
			glVertex3f(x[i] + xz(i, j + 1).first * mult, 0.0f, z[j + 1] + xz(i, j + 1).second * mult);
			glColor3f(0.0f, 0.0f, intensity(i, j));
			glVertex3f(x[i] + xz(i, j).first * mult, 0.0f, z[j] + xz(i, j).second * mult);
		}
		glEnd();
	}
}
