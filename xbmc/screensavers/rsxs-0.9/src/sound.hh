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
#ifndef _SOUND_HH
#define _SOUND_HH

#include <common.hh>

#include <vector.hh>

#if HAVE_SOUND

#include <AL/al.h>
#include <AL/alc.h>

#define SOURCES 32

class Source;

class Buffer : public ResourceManager::Resource<ALuint> {
private:
	ALuint _buffer;
public:
	Buffer(ALenum, const ALvoid*, ALsizei, ALuint);
	~Buffer();
	ALuint operator()() const { return _buffer; }
};

class Sound : public ResourceManager::Resource<void> {
private:
	static ALCdevice* _device;
	static ALCcontext* _context;
	static std::list<Source*> _inactive, _active;

	static float _speedOfSound;
	static const Vector* _cameraPos;
	static const Vector* _cameraVel;
	static const UnitQuat* _cameraDir;

	friend class Source;

	float _ref;
	float _delay;
protected:
	ALuint _buffer;

	Sound(float, float);
public:
	class Unavailable {};

	static void init(
		const std::string&, float, float,
		const Vector*, const Vector*, const UnitQuat*
	);
	static void update();
	void operator()() const {}
	void play(const Vector&);
};

#else // !HAVE_SOUND

class Sound : public ResourceManager::Resource<void> {
public:
	class Unavailable {};

	static void init(
		const std::string&, float, float,
		const Vector*, const Vector*, const UnitQuat*
	) {
		throw Common::Exception("No sound support");
	}
	static void update() {}
	void operator()() const {}
	void play(const Vector&) {}
};

#endif // !HAVE_SOUND

#endif // _SOUND_HH
