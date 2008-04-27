#ifndef INCLUDED_PORTAUDIO_CPPFUNCALLBACKSTREAM_HXX
#define INCLUDED_PORTAUDIO_CPPFUNCALLBACKSTREAM_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/CallbackStream.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s):
namespace portaudio
{
	class StreamParameters;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{


	namespace impl
	{
		extern "C"
		{
			int cppCallbackToPaCallbackAdapter(const void *inputBuffer, void *outputBuffer, unsigned long numFrames, 
				const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, 
				void *userData);
		} // extern "C"
	}

	// -----------------------------------------------------------------------------------

	//////
	/// @brief Callback stream using a C++ function (either a free function or a static function) 
	/// callback.
	//////
	class FunCallbackStream : public CallbackStream
	{
	public:
		typedef int (*CallbackFunPtr)(const void *inputBuffer, void *outputBuffer, unsigned long numFrames, 
			const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, 
			void *userData);

		// -------------------------------------------------------------------------------

		//////
		/// @brief Simple structure containing a function pointer to the C++ callback function and a 
		/// (void) pointer to the user supplied data.
		//////
		struct CppToCCallbackData
		{
			CppToCCallbackData();
			CppToCCallbackData(CallbackFunPtr funPtr, void *userData);
			void init(CallbackFunPtr funPtr, void *userData);

			CallbackFunPtr funPtr;
			void *userData;
		};

		// -------------------------------------------------------------------------------

		FunCallbackStream();
		FunCallbackStream(const StreamParameters &parameters, CallbackFunPtr funPtr, void *userData);
		~FunCallbackStream();

		void open(const StreamParameters &parameters, CallbackFunPtr funPtr, void *userData);

	private:
		FunCallbackStream(const FunCallbackStream &); // non-copyable
		FunCallbackStream &operator=(const FunCallbackStream &); // non-copyable

		CppToCCallbackData adapterData_;

		void open(const StreamParameters &parameters);
	};


} // portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_CPPFUNCALLBACKSTREAM_HXX
