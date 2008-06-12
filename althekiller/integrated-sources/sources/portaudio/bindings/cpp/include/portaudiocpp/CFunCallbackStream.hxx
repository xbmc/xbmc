#ifndef INCLUDED_PORTAUDIO_CFUNCALLBACKSTREAM_HXX
#define INCLUDED_PORTAUDIO_CFUNCALLBACKSTREAM_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/CallbackStream.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s)
namespace portaudio
{
	class StreamParameters;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{
	// -----------------------------------------------------------------------------------

	//////
	/// @brief Callback stream using a free function with C linkage. It's important that the function 
	/// the passed function pointer points to is declared ``extern "C"''.
	//////
	class CFunCallbackStream : public CallbackStream
	{
	public:
		CFunCallbackStream();
		CFunCallbackStream(const StreamParameters &parameters, PaStreamCallback *funPtr, void *userData);
		~CFunCallbackStream();
		
		void open(const StreamParameters &parameters, PaStreamCallback *funPtr, void *userData);

	private:
		CFunCallbackStream(const CFunCallbackStream &); // non-copyable
		CFunCallbackStream &operator=(const CFunCallbackStream &); // non-copyable
	};

	// -----------------------------------------------------------------------------------
} // portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_MEMFUNCALLBACKSTREAM_HXX

