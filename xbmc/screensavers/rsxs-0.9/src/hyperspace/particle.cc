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

#include <hyperspace.hh>
#include <particle.hh>

StretchedParticle::StretchedParticle(
	const Vector& XYZ, float radius, const RGBColor& color, float fov) :
	_XYZ(XYZ), _lastXYZ(XYZ), _radius(radius),
	_color(color), _fov(fov), _moved(true) {
	_lastScreenPos[0] = _lastScreenPos[1] = 0.0f;
}

void StretchedParticle::update() {
	Vector temp(_XYZ - Hack::camera);
	if (temp.x() > Hack::fogDepth) {
		_XYZ.x() = _XYZ.x() - Hack::fogDepth * 2.0f;
		_lastXYZ.x() = _lastXYZ.x() - Hack::fogDepth * 2.0f;
	}
	if (temp.x() < -Hack::fogDepth) {
		_XYZ.x() = _XYZ.x() + Hack::fogDepth * 2.0f;
		_lastXYZ.x() = _lastXYZ.x() + Hack::fogDepth * 2.0f;
	}
	if (temp.z() > Hack::fogDepth) {
		_XYZ.z() = _XYZ.z() - Hack::fogDepth * 2.0f;
		_lastXYZ.z() = _lastXYZ.z() - Hack::fogDepth * 2.0f;
	}
	if (temp.z() < -Hack::fogDepth) {
		_XYZ.z() = _XYZ.z() + Hack::fogDepth * 2.0f;
		_lastXYZ.z() = _lastXYZ.z() + Hack::fogDepth * 2.0f;
	}
}

void StretchedParticle::draw() {
	double winX, winY, winZ;
	gluProject(
		_XYZ.x(), _XYZ.y(), _XYZ.z(),
		Hack::modelMat, Hack::projMat, Hack::viewport,
		&winX, &winY, &winZ
	);

	double screenPos[2];
	if (winZ > 0.0f) {
		screenPos[0] = winX;
		screenPos[1] = winY;
	} else {
		screenPos[0] = _lastScreenPos[0];
		screenPos[1] = _lastScreenPos[1];
	}

	Vector drawXYZ((_XYZ + _lastXYZ) * 0.5f);
	_lastXYZ = _XYZ;

	if (_moved) {
		// Window co-ords are screwed, so skip it
		_moved = false;
	} else {
		float sd[2];  // screen difference, position difference
		sd[0] = float(screenPos[0] - _lastScreenPos[0]);
		sd[1] = float(screenPos[1] - _lastScreenPos[1]);

		Vector pd(Hack::camera - drawXYZ);
		float n = pd.normalize();
		RotationMatrix bbMat = RotationMatrix::lookAt(pd, Vector(), Vector(0, 1, 0));

		float stretch = 0.0015f * std::sqrt(sd[0] * sd[0] + sd[1] * sd[1]) * n / _radius;
		if (stretch < 1.0f) stretch = 1.0f;
		if (stretch > 0.5f / _radius) stretch = 0.5f / _radius;
		glPushMatrix();
			glTranslatef(drawXYZ.x(), drawXYZ.y(), drawXYZ.z());
			glMultMatrixf(bbMat.get());
			glRotatef(R2D * std::atan2(sd[1], sd[0]) + Hack::unroll, 0, 0, 1);
			glScalef(stretch, 1.0f, 1.0f);
			float darkener = stretch * 0.5f;
			if (darkener < 1.0f) darkener = 1.0f;
			// draw colored aura
			glColor3f(_color.r() / darkener, _color.g() / darkener, _color.b() / darkener);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(-_radius, -_radius, 0.0f);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(_radius, -_radius, 0.0f);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(-_radius, _radius, 0.0f);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(_radius, _radius, 0.0f);
			glEnd();
			// draw white center
			glColor3f(1.0f / darkener, 1.0f / darkener, 1.0f / darkener);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(-_radius * 0.3f, -_radius * 0.3f, 0.0f);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(_radius * 0.3f, -_radius * 0.3f, 0.0f);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(-_radius * 0.3f, _radius * 0.3f, 0.0f);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(_radius * 0.3f, _radius * 0.3f, 0.0f);
			glEnd();
		glPopMatrix();
	}

	_lastScreenPos[0] = screenPos[0];
	_lastScreenPos[1] = screenPos[1];
}
