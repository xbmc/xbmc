#include "portaudiocpp/CFunCallbackStream.hxx"

#include "portaudiocpp/StreamParameters.hxx"
#include "portaudiocpp/Exception.hxx"

namespace portaudio
{
	CFunCallbackStream::CFunCallbackStream()
	{
	}

	CFunCallbackStream::CFunCallbackStream(const StreamParameters &parameters, PaStreamCallback *funPtr, void *userData)
	{
		open(parameters, funPtr, userData);
	}

	CFunCallbackStream::~CFunCallbackStream()
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

	// ---------------------------------------------------------------------------------==

	void CFunCallbackStream::open(const StreamParameters &parameters, PaStreamCallback *funPtr, void *userData)
	{
		PaError err = Pa_OpenStream(&stream_, parameters.inputParameters().paStreamParameters(), parameters.outputParameters().paStreamParameters(), 
			parameters.sampleRate(), parameters.framesPerBuffer(), parameters.flags(), funPtr, userData);

		if (err != paNoError)
		{
			throw PaException(err);
		}
	}
}
