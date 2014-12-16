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

#include <cubemaps.hh>
#include <hyperspace.hh>
#include <vector.hh>

#define CUBEMAPS_SIZE 128

#if USE_GL_EXTENSIONS

namespace WavyNormalCubeMaps {
	std::vector<GLuint> textures;
};

namespace WavyNormalCubeMaps {
	inline UnitVector wavyFunc(const Vector&, float);
};

void WavyNormalCubeMaps::init() {
	stdx::dim3<GLubyte, 3, CUBEMAPS_SIZE> map(CUBEMAPS_SIZE);

	// calculate normal cube maps
	Vector vec;
	Vector norm;
	float offset = -0.5f * float(CUBEMAPS_SIZE) + 0.5f;
	for (unsigned int g = 0; g < Hack::frames; ++g) {
		textures.push_back(Common::resources->genCubeMapTexture(
			GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE
		));

		float phase = M_PI * 2.0f * float(g) / float(Hack::frames);

		// left
		for (unsigned int i = 0; i < CUBEMAPS_SIZE; ++i) {
			for (unsigned int j = 0; j < CUBEMAPS_SIZE; ++j) {
				vec.set(
					-0.5f,
					-(float(j) + offset) / float(CUBEMAPS_SIZE),
					(float(i) + offset) / float(CUBEMAPS_SIZE)
				);
				vec.normalize();
				norm = wavyFunc(vec, phase);
				map(j, i, 0) = GLubyte(norm.x() * 127.999f + 128.0f);
				map(j, i, 1) = GLubyte(norm.y() * 127.999f + 128.0f);
				map(j, i, 2) = GLubyte(norm.z() * -127.999f + 128.0f);
			}
		}
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 3, CUBEMAPS_SIZE, CUBEMAPS_SIZE,
			GL_RGB, GL_UNSIGNED_BYTE, &map.front());

		// right
		for (unsigned int i = 0; i < CUBEMAPS_SIZE; ++i) {
			for (unsigned int j = 0; j < CUBEMAPS_SIZE; ++j) {
				vec.set(
					0.5f,
					-(float(j) + offset) / float(CUBEMAPS_SIZE),
					-(float(i) + offset) / float(CUBEMAPS_SIZE)
				);
				vec.normalize();
				norm = wavyFunc(vec, phase);
				map(j, i, 0) = GLubyte(norm.x() * 127.999f + 128.0f);
				map(j, i, 1) = GLubyte(norm.y() * 127.999f + 128.0f);
				map(j, i, 2) = GLubyte(norm.z() * -127.999f + 128.0f);
			}
		}
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 3, CUBEMAPS_SIZE, CUBEMAPS_SIZE,
			GL_RGB, GL_UNSIGNED_BYTE, &map.front());

		// back
		for (unsigned int i = 0; i < CUBEMAPS_SIZE; ++i) {
			for (unsigned int j = 0; j < CUBEMAPS_SIZE; ++j) {
				vec.set(
					-(float(i) + offset) / float(CUBEMAPS_SIZE),
					-(float(j) + offset) / float(CUBEMAPS_SIZE),
					-0.5f
				);
				vec.normalize();
				norm = wavyFunc(vec, phase);
				map(j, i, 0) = GLubyte(norm.x() * 127.999f + 128.0f);
				map(j, i, 1) = GLubyte(norm.y() * 127.999f + 128.0f);
				map(j, i, 2) = GLubyte(norm.z() * -127.999f + 128.0f);
			}
		}
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 3, CUBEMAPS_SIZE, CUBEMAPS_SIZE,
			GL_RGB, GL_UNSIGNED_BYTE, &map.front());

		// front
		for (unsigned int i = 0; i < CUBEMAPS_SIZE; ++i) {
			for (unsigned int j = 0; j < CUBEMAPS_SIZE; ++j) {
				vec.set(
					(float(i) + offset) / float(CUBEMAPS_SIZE),
					-(float(j) + offset) / float(CUBEMAPS_SIZE),
					0.5f
				);
				vec.normalize();
				norm = wavyFunc(vec, phase);
				map(j, i, 0) = GLubyte(norm.x() * 127.999f + 128.0f);
				map(j, i, 1) = GLubyte(norm.y() * 127.999f + 128.0f);
				map(j, i, 2) = GLubyte(norm.z() * -127.999f + 128.0f);
			}
		}
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 3, CUBEMAPS_SIZE, CUBEMAPS_SIZE,
			GL_RGB, GL_UNSIGNED_BYTE, &map.front());

		// bottom
		for (unsigned int i = 0; i < CUBEMAPS_SIZE; ++i) {
			for (unsigned int j = 0; j < CUBEMAPS_SIZE; ++j) {
				vec.set(
					(float(i) + offset) / float(CUBEMAPS_SIZE),
					-0.5f,
					-(float(j) + offset) / float(CUBEMAPS_SIZE)
				);
				vec.normalize();
				norm = wavyFunc(vec, phase);
				map(j, i, 0) = GLubyte(norm.x() * 127.999f + 128.0f);
				map(j, i, 1) = GLubyte(norm.y() * 127.999f + 128.0f);
				map(j, i, 2) = GLubyte(norm.z() * -127.999f + 128.0f);
			}
		}
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 3, CUBEMAPS_SIZE, CUBEMAPS_SIZE,
			GL_RGB, GL_UNSIGNED_BYTE, &map.front());

		// top
		for (unsigned int i = 0; i < CUBEMAPS_SIZE; ++i) {
			for (unsigned int j = 0; j < CUBEMAPS_SIZE; ++j) {
				vec.set(
					(float(i) + offset) / float(CUBEMAPS_SIZE),
					0.5f,
					(float(j) + offset) / float(CUBEMAPS_SIZE)
				);
				vec.normalize();
				norm = wavyFunc(vec, phase);
				map(j, i, 0) = GLubyte(norm.x() * 127.999f + 128.0f);
				map(j, i, 1) = GLubyte(norm.y() * 127.999f + 128.0f);
				map(j, i, 2) = GLubyte(norm.z() * -127.999f + 128.0f);
			}
		}
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 3, CUBEMAPS_SIZE, CUBEMAPS_SIZE,
			GL_RGB, GL_UNSIGNED_BYTE, &map.front());
	}
}

inline UnitVector WavyNormalCubeMaps::wavyFunc(const Vector& vec, float phase) {
	Vector normal(
		vec.x() + (
			0.3f * std::cos((1.0f * vec.x() + 4.0f * vec.y()) * M_PI + phase)
			+ 0.15f * std::cos((3.0f * vec.y() + 13.0f * vec.z()) * M_PI - phase)
		),
		vec.y() + (
			0.3f * std::cos((2.0f * vec.y() - 5.0f * vec.z()) * M_PI + phase)
			+ 0.15f * std::cos((2.0f * vec.z() + 12.0f * vec.x()) * M_PI - phase)
		),
		vec.z() + (
			0.3f * std::cos((1.0f * vec.z() + 6.0f * vec.x()) * M_PI + phase)
			+ 0.15f * std::cos((1.0f * vec.x() - 11.0f * vec.y()) * M_PI - phase)
		)
	);
	return UnitVector(normal);
}

#endif // USE_GL_EXTENSIONS
