#ifndef INCLUDED_PORTAUDIO_INTERFACECALLBACKSTREAM_HXX
#define INCLUDED_PORTAUDIO_INTERFACECALLBACKSTREAM_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/CallbackStream.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s)
namespace portaudio
{
	class StreamParameters;
	class CallbackInterface;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{


	//////
	/// @brief Callback stream using an instance of an object that's derived from the CallbackInterface 
	/// interface.
	//////
	class InterfaceCallbackStream : public CallbackStream
	{
	public:
		InterfaceCallbackStream();
		InterfaceCallbackStream(const StreamParameters &parameters, CallbackInterface &instance);
		~InterfaceCallbackStream();
		
		void open(const StreamParameters &parameters, CallbackInterface &instance);

	private:
		InterfaceCallbackStream(const InterfaceCallbackStream &); // non-copyable
		InterfaceCallbackStream &operator=(const InterfaceCallbackStream &); // non-copyable
	};


} // portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_INTERFACECALLBACKSTREAM_HXX
