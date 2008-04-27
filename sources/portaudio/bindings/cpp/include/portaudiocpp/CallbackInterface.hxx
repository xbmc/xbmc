#ifndef INCLUDED_PORTAUDIO_CALLBACKINTERFACE_HXX
#define INCLUDED_PORTAUDIO_CALLBACKINTERFACE_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

// ---------------------------------------------------------------------------------------

namespace portaudio
{
	// -----------------------------------------------------------------------------------

	//////
	/// @brief Interface for an object that's callable as a PortAudioCpp callback object (ie that implements the 
	/// paCallbackFun method).
	//////
	class CallbackInterface
	{
	public:
		virtual ~CallbackInterface() {}

		virtual int paCallbackFun(const void *inputBuffer, void *outputBuffer, unsigned long numFrames, 
			const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags) = 0;
	};

	// -----------------------------------------------------------------------------------

	namespace impl
	{
		extern "C"
		{
			int callbackInterfaceToPaCallbackAdapter(const void *inputBuffer, void *outputBuffer, unsigned long numFrames, 
				const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, 
				void *userData);
		} // extern "C"
	}

	// -----------------------------------------------------------------------------------

} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_CALLBACKINTERFACE_HXX
