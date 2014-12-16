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

#if HAVE_SOUND

#if HAVE_UNISTD_H
	#include <unistd.h>
#endif
#include <sound.hh>

#include <dlopen.hh>

ALCdevice*         Sound::_device;
ALCcontext*        Sound::_context;
std::list<Source*> Sound::_inactive, Sound::_active;

float           Sound::_speedOfSound;
const Vector*   Sound::_cameraPos;
const Vector*   Sound::_cameraVel;
const UnitQuat* Sound::_cameraDir;

#if USE_DLOPEN
	#define AL_FUNC(T, F) T F;
#else
	#define AL_FUNC(T, F) using ::F;
#endif
namespace AL {
	AL_FUNC(LPALCCLOSEDEVICE,        alcCloseDevice);
	AL_FUNC(LPALCCREATECONTEXT,      alcCreateContext);
	AL_FUNC(LPALCDESTROYCONTEXT,     alcDestroyContext);
	AL_FUNC(LPALCGETCURRENTCONTEXT,  alcGetCurrentContext);
	AL_FUNC(LPALCMAKECONTEXTCURRENT, alcMakeContextCurrent);
	AL_FUNC(LPALCOPENDEVICE,         alcOpenDevice);
	AL_FUNC(LPALCPROCESSCONTEXT,     alcProcessContext);

	AL_FUNC(LPALBUFFERDATA,          alBufferData);
	AL_FUNC(LPALDELETEBUFFERS,       alDeleteBuffers);
	AL_FUNC(LPALDELETESOURCES,       alDeleteSources);
	AL_FUNC(LPALDISTANCEMODEL,       alDistanceModel);
	AL_FUNC(LPALGENBUFFERS,          alGenBuffers);
	AL_FUNC(LPALGENSOURCES,          alGenSources);
	AL_FUNC(LPALGETSOURCEI,          alGetSourcei);
	AL_FUNC(LPALLISTENERF,           alListenerf);
	AL_FUNC(LPALLISTENERFV,          alListenerfv);
	AL_FUNC(LPALSOURCEF,             alSourcef);
	AL_FUNC(LPALSOURCEFV,            alSourcefv);
	AL_FUNC(LPALSOURCEI,             alSourcei);
	AL_FUNC(LPALSOURCEPAUSE,         alSourcePause);
	AL_FUNC(LPALSOURCEPLAY,          alSourcePlay);
	AL_FUNC(LPALSOURCEREWIND,        alSourceRewind);
};

Buffer::Buffer(
	ALenum format, const ALvoid* data, ALsizei size, ALuint frequency
) {
	AL::alGenBuffers(1, &_buffer);
	AL::alBufferData(_buffer, format, data, size, frequency);
}

Buffer::~Buffer() { AL::alDeleteBuffers(1, &_buffer); }

class Device : public ResourceManager::Resource<ALCdevice*> {
private:
	ALCdevice* _device;
public:
	Device(const std::string& spec) {
		_device = AL::alcOpenDevice(spec.c_str());
		if (!_device) {
			// Sleep for a moment before trying again
			sleep(1);
			_device = AL::alcOpenDevice(spec.c_str());
			if (!_device)
				throw ResourceManager::Exception(
					stdx::oss() << "Could not open AL device: " <<
						(spec.length() ? spec : "(default)")
				);
		}
	}
	~Device() { AL::alcCloseDevice(_device); }
	ALCdevice* operator()() const { return _device; }
};

class Context : public ResourceManager::Resource<ALCcontext*> {
private:
	ALCcontext* _prev;
	ALCcontext* _context;
public:
	Context(ALCdevice* device) : _prev(AL::alcGetCurrentContext()) {
		_context = AL::alcCreateContext(device, NULL);
		if (!_context)
			throw ResourceManager::Exception("Could not create AL context");
		AL::alcMakeContextCurrent(_context);
	}
	~Context() {
		AL::alcMakeContextCurrent(_prev);
		AL::alcDestroyContext(_context);
	}
	ALCcontext* operator()() const { return _context; }
};

class Source : public ResourceManager::Resource<ALuint> {
private:
	ALuint _source;
	Vector _pos;
	float _time;
public:
	Source() { AL::alGenSources(1, &_source); }
	~Source() { AL::alDeleteSources(1, &_source); }
	ALuint operator()() const { return _source; }

	void start(ALuint buffer, const Vector& pos, float ref, float delay) {
		AL::alSourceRewind(_source);
		AL::alSourcei(_source, AL_BUFFER, buffer);
		AL::alSourcef(_source, AL_GAIN, 1.0f);
		AL::alSourcef(_source, AL_REFERENCE_DISTANCE, ref);
		AL::alSourcef(_source, AL_ROLLOFF_FACTOR, 1.0f);
		AL::alSourcei(_source, AL_LOOPING, AL_FALSE);
		AL::alSourcefv(_source, AL_POSITION, (float*)_pos.get());
		_pos = pos;
		_time = -delay;
	}

	bool update() {
		_time += Common::elapsedTime;
		if (_time < 0.0f) return false;

		// Do doppler effects manually due to OpenAL bugs
		float soundTime = _time;
		if (Sound::_speedOfSound) {
			Vector toCamera(*Sound::_cameraPos - _pos);
			float vls = Vector::dot(toCamera, *Sound::_cameraVel / Common::elapsedSecs) / toCamera.length();
			if (vls > Sound::_speedOfSound) vls = Sound::_speedOfSound;
			float factor = (Sound::_speedOfSound - vls) / Sound::_speedOfSound;
			AL::alSourcef(_source, AL_PITCH, Common::speed * factor);
			soundTime -= toCamera.length() / Sound::_speedOfSound;
		}

		ALenum state = 0x1234;
		AL::alGetSourcei(_source, AL_SOURCE_STATE, &state);
		switch (state) {
		case AL_INITIAL:
		case AL_PAUSED:
			if (Common::speed > 0.0f && soundTime >= 0.0f)
				AL::alSourcePlay(_source);
			break;
		case AL_PLAYING:
			if (Common::speed == 0.0f || soundTime < 0.0f)
				AL::alSourcePause(_source);
			break;
		case AL_STOPPED:
			return false;
		}

		return true;
	}
};

void Sound::init(
	const std::string& spec, float volume, float speedOfSound,
	const Vector* cameraPos, const Vector* cameraVel, const UnitQuat* cameraDir
) {
#if USE_DLOPEN
	try {
		const Library* libopenal = Common::resources->manage(new Library("libopenal"));
		AL::alcCloseDevice        = (LPALCCLOSEDEVICE)(*libopenal)("alcCloseDevice");
		AL::alcCreateContext      = (LPALCCREATECONTEXT)(*libopenal)("alcCreateContext");
		AL::alcDestroyContext     = (LPALCDESTROYCONTEXT)(*libopenal)("alcDestroyContext");
		AL::alcGetCurrentContext  = (LPALCGETCURRENTCONTEXT)(*libopenal)("alcGetCurrentContext");
		AL::alcMakeContextCurrent = (LPALCMAKECONTEXTCURRENT)(*libopenal)("alcMakeContextCurrent");
		AL::alcOpenDevice         = (LPALCOPENDEVICE)(*libopenal)("alcOpenDevice");
		AL::alcProcessContext     = (LPALCPROCESSCONTEXT)(*libopenal)("alcProcessContext");
		AL::alBufferData          = (LPALBUFFERDATA)(*libopenal)("alBufferData");
		AL::alDeleteBuffers       = (LPALDELETEBUFFERS)(*libopenal)("alDeleteBuffers");
		AL::alDeleteSources       = (LPALDELETESOURCES)(*libopenal)("alDeleteSources");
		AL::alDistanceModel       = (LPALDISTANCEMODEL)(*libopenal)("alDistanceModel");
		AL::alGenBuffers          = (LPALGENBUFFERS)(*libopenal)("alGenBuffers");
		AL::alGenSources          = (LPALGENSOURCES)(*libopenal)("alGenSources");
		AL::alGetSourcei          = (LPALGETSOURCEI)(*libopenal)("alGetSourcei");
		AL::alListenerf           = (LPALLISTENERF)(*libopenal)("alListenerf");
		AL::alListenerfv          = (LPALLISTENERFV)(*libopenal)("alListenerfv");
		AL::alSourcef             = (LPALSOURCEF)(*libopenal)("alSourcef");
		AL::alSourcefv            = (LPALSOURCEFV)(*libopenal)("alSourcefv");
		AL::alSourcei             = (LPALSOURCEI)(*libopenal)("alSourcei");
		AL::alSourcePause         = (LPALSOURCEPAUSE)(*libopenal)("alSourcePause");
		AL::alSourcePlay          = (LPALSOURCEPLAY)(*libopenal)("alSourcePlay");
		AL::alSourceRewind        = (LPALSOURCEREWIND)(*libopenal)("alSourceRewind");
	} catch (Common::Exception e) {
		throw Unavailable();
	}
#endif // USE_DLOPEN

	_device = Common::resources->manage(new Device(spec));
	_context = Common::resources->manage(new Context(_device));

	for (unsigned int i = 0; i < SOURCES; ++i) {
		Source* source = new Source();
		Common::resources->manage(source);
		_inactive.push_back(source);
	}

	AL::alDistanceModel(AL_INVERSE_DISTANCE);
	AL::alListenerf(AL_GAIN, volume * 0.01f);
	_speedOfSound = speedOfSound;
	_cameraPos = cameraPos;
	_cameraVel = cameraVel;
	_cameraDir = cameraDir;
}

void Sound::update() {
	Vector forward(_cameraDir->forward());
	Vector up(_cameraDir->up());
	float orientation[] = {
		forward.x(), forward.y(), forward.z(),
		up.x(), up.y(), up.z()
	};
			
	AL::alListenerfv(AL_POSITION, (float*)_cameraPos->get());
	AL::alListenerfv(AL_ORIENTATION, orientation);

	for (
		std::list<Source*>::iterator it = _active.begin();
		it != _active.end();
	) {
		if (!(*it)->update()) {
			_inactive.push_back(*it);
			it = _active.erase(it);
			continue;
		}
		++it;
	}

	AL::alcProcessContext(_context);
}

void Sound::play(const Vector& pos) {
	Source* source;
	if (_inactive.empty()) {
		/* Kill the oldest source */
		source = _active.front();
		_active.pop_front();
	} else {
		source = _inactive.front();
		_inactive.pop_front();
	}
	_active.push_back(source);
	source->start(_buffer, pos, _ref, _delay);
}

Sound::Sound(float ref, float delay) : _ref(ref), _delay(delay) {}

#endif // HAVE_SOUND
