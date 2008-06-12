#ifndef INCLUDED_PORTAUDIO_CALLBACKSTREAM_HXX
#define INCLUDED_PORTAUDIO_CALLBACKSTREAM_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/Stream.hxx"

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{


	//////
	/// @brief Base class for all Streams which use a callback-based mechanism.
	//////
	class CallbackStream : public Stream
	{
	protected:
		CallbackStream();
		virtual ~CallbackStream();

	public:
		// stream info (time-varying)
		double cpuLoad() const;

	private:
		CallbackStream(const CallbackStream &); // non-copyable
		CallbackStream &operator=(const CallbackStream &); // non-copyable
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_CALLBACKSTREAM_HXX
