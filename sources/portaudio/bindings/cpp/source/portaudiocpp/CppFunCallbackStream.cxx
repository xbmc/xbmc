#include "portaudiocpp/CppFunCallbackStream.hxx"

#include "portaudiocpp/StreamParameters.hxx"
#include "portaudiocpp/Exception.hxx"

namespace portaudio
{
	namespace impl
	{
		//////
		/// Adapts any a C++ callback to a C-callable function (ie this function). A 
		/// pointer to a struct with the C++ function pointer and the actual user data should be 
		/// passed as the ``userData'' parameter when setting up the callback.
		//////
		int cppCallbackToPaCallbackAdapter(const void *inputBuffer, void *outputBuffer, unsigned long numFrames, 
			const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
		{
			FunCallbackStream::CppToCCallbackData *data = static_cast<FunCallbackStream::CppToCCallbackData *>(userData);
			return data->funPtr(inputBuffer, outputBuffer, numFrames, timeInfo, statusFlags, data->userData);
		}
	}

	// -----------------------------------------------------------------------------------

	FunCallbackStream::CppToCCallbackData::CppToCCallbackData()
	{
	}

	FunCallbackStream::CppToCCallbackData::CppToCCallbackData(CallbackFunPtr funPtr, void *userData) : funPtr(funPtr), userData(userData)
	{
	}

	void FunCallbackStream::CppToCCallbackData::init(CallbackFunPtr funPtr, void *userData)
	{
		this->funPtr = funPtr;
		this->userData = userData;
	}

	// -----------------------------------------------------------------------------------

	FunCallbackStream::FunCallbackStream()
	{
	}

	FunCallbackStream::FunCallbackStream(const StreamParameters &parameters, CallbackFunPtr funPtr, void *userData) : adapterData_(funPtr, userData)
	{
		open(parameters);
	}

	FunCallbackStream::~FunCallbackStream()
	{
		try
		{
			close();
		}
		catch (...)
		{
			// ignore all errors
		}
	}

	void FunCallbackStream::open(const StreamParameters &parameters, CallbackFunPtr funPtr, void *userData)
	{
		adapterData_.init(funPtr, userData);
		open(parameters);
	}

	void FunCallbackStream::open(const StreamParameters &parameters)
	{
		PaError err = Pa_OpenStream(&stream_, parameters.inputParameters().paStreamParameters(), parameters.outputParameters().paStreamParameters(), 
			parameters.sampleRate(), parameters.framesPerBuffer(), parameters.flags(), &impl::cppCallbackToPaCallbackAdapter, 
			static_cast<void *>(&adapterData_));

		if (err != paNoError)
		{
			throw PaException(err);
		}
	}

	// -----------------------------------------------------------------------------------
}
