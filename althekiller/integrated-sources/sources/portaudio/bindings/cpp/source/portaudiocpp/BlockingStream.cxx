#include "portaudiocpp/BlockingStream.hxx"

#include "portaudio.h"

#include "portaudiocpp/StreamParameters.hxx"
#include "portaudiocpp/Exception.hxx"

namespace portaudio
{

	// --------------------------------------------------------------------------------------

	BlockingStream::BlockingStream()
	{
	}

	BlockingStream::BlockingStream(const StreamParameters &parameters)
	{
		open(parameters);
	}

	BlockingStream::~BlockingStream()
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

	// --------------------------------------------------------------------------------------

	void BlockingStream::open(const StreamParameters &parameters)
	{
		PaError err = Pa_OpenStream(&stream_, parameters.inputParameters().paStreamParameters(), parameters.outputParameters().paStreamParameters(), 
			parameters.sampleRate(), parameters.framesPerBuffer(), parameters.flags(), NULL, NULL);

		if (err != paNoError)
		{
			throw PaException(err);
		}
	}

	// --------------------------------------------------------------------------------------

	void BlockingStream::read(void *buffer, unsigned long numFrames)
	{
		PaError err = Pa_ReadStream(stream_, buffer, numFrames);

		if (err != paNoError)
		{
			throw PaException(err);
		}
	}

	void BlockingStream::write(const void *buffer, unsigned long numFrames)
	{
		PaError err = Pa_WriteStream(stream_, buffer, numFrames);

		if (err != paNoError)
		{
			throw PaException(err);
		}
	}

	// --------------------------------------------------------------------------------------

	signed long BlockingStream::availableReadSize() const
	{
		signed long avail = Pa_GetStreamReadAvailable(stream_);

		if (avail < 0)
		{
			throw PaException(avail);
		}

		return avail;
	}

	signed long BlockingStream::availableWriteSize() const
	{
		signed long avail = Pa_GetStreamWriteAvailable(stream_);

		if (avail < 0)
		{
			throw PaException(avail);
		}

		return avail;
	}

	// --------------------------------------------------------------------------------------

} // portaudio



