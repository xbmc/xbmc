#include "portaudiocpp/CallbackInterface.hxx"

namespace portaudio
{

	namespace impl
	{

		//////
		/// Adapts any CallbackInterface object to a C-callable function (ie this function). A 
		/// pointer to the object should be passed as ``userData'' when setting up the callback.
		//////
		int callbackInterfaceToPaCallbackAdapter(const void *inputBuffer, void *outputBuffer, unsigned long numFrames, 
			const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
		{
			CallbackInterface *cb = static_cast<CallbackInterface *>(userData);
			return cb->paCallbackFun(inputBuffer, outputBuffer, numFrames, timeInfo, statusFlags);
		}


	} // namespace impl

} // namespace portaudio


