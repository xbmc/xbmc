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

#include <bomb.hh>
#include <color.hh>
#include <explosion.hh>
#include <skyrocket.hh>
#include <star.hh>
#include <vector.hh>

void Bomb::update() {
	_remaining -= Common::elapsedTime;

	bool alive = _remaining > 0.0f && _pos.y() >= 0.0f;
	if (!alive) {
		switch (_bombType) {
		case BOMB_STARS:
			Hack::pending.push_back(new Explosion(_pos, _vel,
				Explosion::EXPLODE_STARS_FROM_BOMB, _RGB));
			break;
		case BOMB_STREAMERS:
			Hack::pending.push_back(new Explosion(_pos, _vel,
				Explosion::EXPLODE_STREAMERS_FROM_BOMB, _RGB));
			break;
		case BOMB_METEORS:
			Hack::pending.push_back(new Explosion(_pos, _vel,
				Explosion::EXPLODE_METEORS_FROM_BOMB, _RGB));
			break;
		case BOMB_CRACKER:
			Hack::pending.push_back(new Star(_pos, _vel, 0.4f, 0.2f,
				RGBColor(1.0f, 0.8f, 0.6f), Common::randomFloat(3.0f) + 7.0f));
			break;
		}
		_depth = DEAD_DEPTH;
		++Hack::numDead;
		return;
	}

	_vel.y() -= Common::elapsedTime * 32.0f;
	_pos += _vel * Common::elapsedTime;
	_pos.x() +=
		(0.1f - 0.00175f * _pos.y() + 0.0000011f * _pos.y() * _pos.y()) *
		Hack::wind * Common::elapsedTime;

	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Bomb::updateCameraOnly() {
	Vector diff(Hack::cameraPos - _pos);
	_depth = diff.x() * Hack::cameraMat.get(8) +
		diff.y() * Hack::cameraMat.get(9) +
		diff.z() * Hack::cameraMat.get(10);
}

void Bomb::draw() const {
	// Invisible!
}
