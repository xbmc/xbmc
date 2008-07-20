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
#include <resources.hh>
#include <shockwave.hh>
#include <skyrocket.hh>
#include <star.hh>
#include <vector.hh>

Vector Shockwave::_geom[7][WAVESTEPS + 1];

void Shockwave::init() {
	_geom[0][0].set(1.0f, 0.0f, 0.0f);
	_geom[1][0].set(0.985f, 0.035f, 0.0f);
	_geom[2][0].set(0.95f, 0.05f, 0.0f);
	_geom[3][0].set(0.85f, 0.05f, 0.0f);
	_geom[4][0].set(0.75f, 0.035f, 0.0f);
	_geom[5][0].set(0.65f, 0.01f, 0.0f);
	_geom[6][0].set(0.5f, 0.0f, 0.0f);

	for (unsigned int i = 1; i <= WAVESTEPS; ++i) {
		float ch = std::cos(M_PI * 2.0f * (float(i) / float(WAVESTEPS)));
		float sh = std::sin(M_PI * 2.0f * (float(i) / float(WAVESTEPS)));
		for (unsigned int j = 0; j < 7; ++j)
			_geom[j][i].set(
				ch * _geom[j][0].x(),
				_geom[j][0].y(),
				sh * _geom[j][0].x()
			);
	}
}

Shockwave::Shockwave(const Vector& pos, const Vector& vel) :
		Particle(pos, vel, 0.612f, 5.0f), _size(0.0f) {
	RGBColor RGB(Particle::randomColor());
	for (unsigned int i = 0; i < 75; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= 100.0f + Common::randomFloat(10.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(2.0f) + 2.0f, RGB, 100.0f));
	}

	RGB = Particle::randomColor();
	for (unsigned int i = 0; i < 150; ++i) {
		Vector velocity(
			Common::randomFloat(1.0f) - 0.5f,
			Common::randomFloat(0.03f) - 0.005f,
			Common::randomFloat(1.0f) - 0.5f
		);
		velocity.normalize();
		velocity *= 500.0f + Common::randomFloat(30.0f);
		velocity += _vel;

		Hack::pending.push_back(new Star(_pos, velocity, 0.612f,
			Common::randomFloat(2.0f) + 3.0f, RGB, 50.0f));
	}

	if (Hack::volume > 0.0f)
		Resources::Sounds::nuke->play(_pos);
}

void Shockwave::update() {
	_remaining -= Common::elapsedTime;

	if (_remaining <= 0.0f || _pos.y() <= 0.0f) {
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	_vel.y() -= Common::elapsedTime * 32.0f;
	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	_life = _remaining / _lifetime;
	_size += 400.0f * _life;

	Hack::shock(_pos, (1.0f - _life) * 0.002f * Common::elapsedTime);
	Hack::superFlare(_pos, RGBColor(1.0f, 1.0f, 1.0f), _life);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Shockwave::updateCameraOnly() {
	Hack::superFlare(_pos, RGBColor(1.0f, 1.0f, 1.0f), _life);

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Shockwave::draw() const {
	if (_depth < 0.0f)
		return;

	glPushMatrix();
		glTranslatef(_pos.x(), _pos.y(), _pos.z());
		glScalef(_size, _size, _size);
		drawShockwave(_life, float(std::sqrt(_size)) * 0.08f);
		if (_life > 0.7f) {
			glMultMatrixf(Hack::cameraMat.get());
			glScalef(5.0f, 5.0f, 5.0f);
			glColor4f(1.0f, _life, 1.0f, (_life - 0.7f) * 3.333f);
			glCallList(Resources::DisplayLists::flares + 2);
		}
	glPopMatrix();
}

void Shockwave::drawShockwave(float temperature, float texMove) const {
	float temp;
	float alphas[7];
	if (temperature > 0.5f) {
		temp = 1.0f;
		alphas[0] = 1.0f; alphas[1] = 0.9f; alphas[2] = 0.8f;
		alphas[3] = 0.7f; alphas[4] = 0.5f; alphas[5] = 0.3f;
		alphas[6] = 0.0f;
	} else {
		temp = temperature * 2.0f;
		alphas[0] = temp; alphas[1] = temp * 0.9f; alphas[2] = temp * 0.8f;
		alphas[3] = temp * 0.7f; alphas[4] = temp * 0.5f; alphas[5] = temp * 0.3f;
		alphas[6] = 0.0f;
	}
	RGBColor colors[7];
	for (unsigned int i = 0; i < 6; ++i)
		colors[i].set(1.0f, temp, temperature);

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, Resources::Textures::cloud);

	// draw bottom of shockwave
	for (unsigned int i = 0; i < 6; ++i) {
		float v1 = float(i + 1) * 0.07f - texMove;
		float v2 = float(i) * 0.07f - texMove;
		glBegin(GL_TRIANGLE_STRIP);
		for (unsigned int j = 0; j <= WAVESTEPS; ++j) {
			float u = (float(j) / float(WAVESTEPS)) * 10.0f;
			glColor4f(colors[i + 1].r(), colors[i + 1].g(),
				colors[i + 1].b(), alphas[i + 1]);
			glTexCoord2f(u, v1);
			glVertex3fv(_geom[i + 1][j].get());
			glColor4f(colors[i].r(), colors[i].g(), colors[i].b(),
				alphas[i]);
			glTexCoord2f(u, v2);
			glVertex3fv(_geom[i][j].get());
		}
		glEnd();
	}

	// keep colors a little warmer on top (more green)
	if (temperature < 0.5f)
		for (unsigned int i = 1; i < 6; ++i)
			colors[i].g() = temperature + 0.5f;

	// draw top of shockwave
	for (unsigned int i = 0; i < 6; ++i) {
		float v1 = float(i) * 0.07f - texMove;
		float v2 = float(i + 1) * 0.07f - texMove;
		glBegin(GL_TRIANGLE_STRIP);
		for (unsigned int j = 0; j <= WAVESTEPS; ++j) {
			float u = (float(j) / float(WAVESTEPS)) * 10.0f;
			glColor4f(colors[i + 1].r(), colors[i + 1].g(),
				colors[i + 1].b(), alphas[i + 1]);
			glTexCoord2f(u, v1);
			glVertex3fv(_geom[i + 1][j].get());
			glColor4f(colors[i].r(), colors[i].g(), colors[i].b(),
				alphas[i]);
			glTexCoord2f(u, v2);
			glVertex3fv(_geom[i][j].get());
		}
		glEnd();
	}

	glPopAttrib();
}

void Shockwave::shock(const Vector&, float) {
}
