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

#include <lattice.hh>
#include <pngimage.hh>
#include <resource.hh>
#include <resources.hh>

namespace Resources {
	GLuint lists;

	struct TextureData {
		unsigned int texture;
		float shininess;
		bool sphereMap;
		bool colored;
		bool modulate;
	};

	std::vector<TextureData> _textures;

	void makeTorus(float, float);
};

void Resources::makeTorus(float centerRadius, float thickRadius) {
	// Smooth shading?
	if (Hack::smooth)
		glShadeModel(GL_SMOOTH);
	else
		glShadeModel(GL_FLAT);

	// Initialize texture stuff
	float vStep = 1.0f / float(Hack::latitude);
	float uStep = float(int((centerRadius / thickRadius) + 0.5f)) /
		float(Hack::longitude);
	float v2 = 0.0f;

	for (unsigned int i = 0; i < Hack::latitude; ++i) {
		float temp = M_PI * 2.0f * float(i) / float(Hack::latitude);
		float cosn = std::cos(temp);
		float sinn = std::sin(temp);
		temp = M_PI * 2.0f * float(i + 1) / float(Hack::latitude);
		float cosnn = std::cos(temp);
		float sinnn = std::sin(temp);
		float r = centerRadius + thickRadius * cosn;
		float rr = centerRadius + thickRadius * cosnn;
		float z = thickRadius * sinn;
		float zz = thickRadius * sinnn;
		if (!Hack::smooth) {	// Redefine normals for flat shaded model
			temp = M_PI * 2.0f * (float(i) + 0.5f) / float(Hack::latitude);
			cosn = cosnn = std::cos(temp);
			sinn = sinnn = std::sin(temp);
		}
		float v1 = v2;
		v2 += vStep;
		float u = 0.0f;
		float oldcosa = 0.0f, oldsina = 0.0f, oldncosa = 0.0f, oldnsina = 0.0f;
		float oldcosn = 0.0f, oldcosnn = 0.0f, oldsinn = 0.0f, oldsinnn = 0.0f;
		glBegin(GL_TRIANGLE_STRIP);
			for (unsigned int j = 0; j < Hack::longitude; ++j) {
				temp = M_PI * 2.0f * float(j) / float(Hack::longitude);
				float cosa = std::cos(temp);
				float sina = std::sin(temp);
				float ncosa, nsina;
				if (Hack::smooth) {
					ncosa = cosa;
					nsina = sina;
				} else {	// Redefine longitudinal component of normal for flat shading
					temp = M_PI * 2.0f * (float(j) - 0.5f) / float(Hack::longitude);
					ncosa = std::cos(temp);
					nsina = std::sin(temp);
				}
				if (j == 0) {	// Save first values for end of circular tri-strip
					oldcosa = cosa;
					oldsina = sina;
					oldncosa = ncosa;
					oldnsina = nsina;
					oldcosn = cosn;
					oldcosnn = cosnn;
					oldsinn = sinn;
					oldsinnn = sinnn;
				}
				glNormal3f(cosnn * ncosa, cosnn * nsina, sinnn);
				glTexCoord2f(u, v2);
				glVertex3f(cosa * rr, sina * rr, zz);
				glNormal3f(cosn * ncosa, cosn * nsina, sinn);
				glTexCoord2f(u, v1);
				glVertex3f(cosa * r, sina * r, z);
				u += uStep;	// update u texture coordinate
			}
			// Finish off circular tri-strip with saved first values
			glNormal3f(oldcosnn * oldncosa, oldcosnn * oldnsina, oldsinnn);
			glTexCoord2f(u, v2);
			glVertex3f(oldcosa * rr, oldsina * rr, zz);
			glNormal3f(oldcosn * oldncosa, oldcosn * oldnsina, oldsinn);
			glTexCoord2f(u, v1);
			glVertex3f(oldcosa * r, oldsina * r, z);
		glEnd();
	}
}

void Resources::init() {
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_LIGHT0);
	float ambient[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float diffuse[4] = {1.0f, 1.0f, 1.0f, 0.0f};
	float specular[4] = {1.0f, 1.0f, 1.0f, 0.0f};
	float position[4] = {400.0f, 300.0f, 500.0f, 0.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glEnable(GL_COLOR_MATERIAL);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	for (
		std::vector<Hack::Texture>::const_iterator it = Hack::textures.begin();
		it != Hack::textures.end();
		++it
	) {
		TextureData data;
		data.shininess = it->shininess;
		data.sphereMap = it->sphereMap;
		data.colored = it->colored;
		data.modulate = it->modulate;

		if (it->filename != "") {
			PNG png(it->filename, !data.modulate);
			data.texture = Common::resources->genTexture(
				GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT,
				png
			);
			if (Hack::linkType == Hack::SOLID_LINKS)
				if (!data.modulate && !png.hasAlphaChannel())
					data.colored = false;
		} else
			data.texture = 0;
		_textures.push_back(data);
	}

	switch (Hack::linkType) {
	case Hack::SOLID_LINKS:
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		break;
	case Hack::TRANSLUCENT_LINKS:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		break;
	case Hack::HOLLOW_LINKS:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		break;
	default:
		throw Common::Exception("Invalid linkType");
	}

	unsigned int d = 0;
	float thickness = Hack::thickness * 0.001f;
	lists = Common::resources->genLists(NUMOBJECTS);
	for (unsigned int i = 0; i < NUMOBJECTS; ++i) {
		glNewList(lists + i, GL_COMPILE);
			const TextureData& data =
				_textures[Common::randomInt(Hack::textures.size())];

			if (data.shininess >= 0.0f) {
				glEnable(GL_LIGHTING);
				// Add 0.0f to convert -0.0f into +0.0f, else GL complains
				glMaterialf(GL_FRONT, GL_SHININESS, 0.0f + data.shininess);
				glColorMaterial(GL_FRONT, GL_SPECULAR);
			} else
				glDisable(GL_LIGHTING);
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

			if (data.texture) {
				if (data.sphereMap) {
					glEnable(GL_TEXTURE_GEN_S);
					glEnable(GL_TEXTURE_GEN_T);
				} else {
					glDisable(GL_TEXTURE_GEN_S);
					glDisable(GL_TEXTURE_GEN_T);
				}
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, data.texture);
				if (data.modulate)
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				else
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			} else
				glDisable(GL_TEXTURE_2D);

			if (!data.colored)
				glColor3f(1.0f, 1.0f, 1.0f);
			if (d < Hack::density) {
				glPushMatrix();
					if (data.colored)
						glColor3f(
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f)
						);
					glTranslatef(-0.25f, -0.25f, -0.25f);
					if (Common::randomInt(2))
						glRotatef(180.0f, 1, 0, 0);
					makeTorus(0.36f - thickness, thickness);
				glPopMatrix();
			}
			d = (d + 37) % 100;
			if (d < Hack::density) {
				glPushMatrix();
					if (data.colored)
						glColor3f(
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f)
						);
					glTranslatef(0.25f, -0.25f, -0.25f);
					if (Common::randomInt(2))
						glRotatef(90.0f, 1, 0, 0);
					else
						glRotatef(-90.0f, 1, 0, 0);
					makeTorus(0.36f - thickness, thickness);
				glPopMatrix();
			}
			d = (d + 37) % 100;
			if (d < Hack::density) {
				glPushMatrix();
					if (data.colored)
						glColor3f(
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f)
						);
					glTranslatef(0.25f, -0.25f, 0.25f);
					if (Common::randomInt(2))
						glRotatef(90.0f, 0, 1, 0);
					else
						glRotatef(-90.0f, 0, 1, 0);
					makeTorus(0.36f - thickness, thickness);
				glPopMatrix();
			}
			d = (d + 37) % 100;
			if (d < Hack::density) {
				glPushMatrix();
					if (data.colored)
						glColor3f(
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f)
						);
					glTranslatef(0.25f, 0.25f, 0.25f);
					if (Common::randomInt(2))
						glRotatef(180.0f, 1, 0, 0);
					makeTorus(0.36f - thickness, thickness);
				glPopMatrix();
			}
			d = (d + 37) % 100;
			if (d < Hack::density) {
				glPushMatrix();
					if (data.colored)
						glColor3f(
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f)
						);
					glTranslatef(-0.25f, 0.25f, 0.25f);
					if (Common::randomInt(2))
						glRotatef(90.0f, 1, 0, 0);
					else
						glRotatef(-90.0f, 1, 0, 0);
					makeTorus(0.36f - thickness, thickness);
				glPopMatrix();
			}
			d = (d + 37) % 100;
			if (d < Hack::density) {
				glPushMatrix();
					if (data.colored)
						glColor3f(
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f),
							Common::randomFloat(1.0f)
						);
					glTranslatef(-0.25f, 0.25f, -0.25f);
					if (Common::randomInt(2))
						glRotatef(90.0f, 0, 1, 0);
					else
						glRotatef(-90.0f, 0, 1, 0);
					makeTorus(0.36f - thickness, thickness);
				glPopMatrix();
			}
		glEndList();
		d = (d + 37) % 100;
	}
}
