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

#include <blend.hh>
#include <cyclone.hh>
#include <particle.hh>
#include <vector.hh>

GLuint Particle::_list;

void Particle::update() {
	_lastV = _V;
	if (_step > 1.0f)
		setup();

	// Take local copy for optimizability
	unsigned int complexity = Hack::complexity;

	_V.set(0, 0, 0);
	for (unsigned int i = 0; i < complexity + 3; ++i)
		_V += _cy->v(i) * Blend::blend(i, _step);

	Vector dir;
	Vector up(0.0f, 1.0f, 0.0f);

	for (unsigned int i = 0; i < complexity + 3; ++i)
		dir += _cy->v(i) * Blend::blend(i, _step - 0.01f);
	dir = _V - dir;
	dir.normalize();
	Vector crossVec(Vector::cross(dir, up));

	float tiltAngle = -std::acos(Vector::dot(dir, up)) * R2D;
	unsigned int i = (unsigned int)(_step * (float(Hack::complexity) + 2.0f));
	if (i >= Hack::complexity + 2)
		i = Hack::complexity + 1;
	float between = (
		_step -
		(float(i) / float(Hack::complexity + 2))
	) * float(Hack::complexity + 2);
	float cyWidth = _cy->width(i) * (1.0f - between) +
		_cy->width(i + 1) * between;
	float newStep = (0.005f * Hack::speed) / (_width * _width * cyWidth);
	_step += newStep;
	float newSpinAngle = (40.0f * Hack::speed) / (_width * cyWidth);
	_spinAngle += Hack::southern ? -newSpinAngle : newSpinAngle;

	float scale = 1.0f;
	if (Hack::stretch)
		scale = Common::clamp(
			_width * cyWidth * newSpinAngle * 0.02f,
			1.0f, cyWidth * 2.0f / Hack::size
		);
	glColor3fv(_RGB.get());
	glPushMatrix();
		glLoadIdentity();
		glTranslatef(_V.x(), _V.y(), _V.z());
		glRotatef(tiltAngle, crossVec.x(), crossVec.y(), crossVec.z());
		glRotatef(_spinAngle, 0, 1, 0);
		glTranslatef(_width * cyWidth, 0, 0);
		glScalef(1.0f, 1.0f, scale);
		glCallList(_list);
	glPopMatrix();
}
