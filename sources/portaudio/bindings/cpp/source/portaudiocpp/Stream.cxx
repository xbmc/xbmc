#include "portaudiocpp/Stream.hxx"

#include <cstddef>

#include "portaudiocpp/Exception.hxx"
#include "portaudiocpp/System.hxx"

namespace portaudio
{

	// -----------------------------------------------------------------------------------

	Stream::Stream() : stream_(NULL)
	{
	}

	Stream::~Stream()
	{
		// (can't call close here, 
		// the derived class should atleast call 
		// close() in it's deconstructor)
	}

	// -----------------------------------------------------------------------------------

	//////
	/// Closes the Stream if it's open, else does nothing.
	//////
	void Stream::close()
	{
		if (isOpen() && System::exists())
		{
			PaError err = Pa_CloseStream(stream_);
			stream_ = NULL;

			if (err != paNoError)
				throw PaException(err);
		}
	}

	//////
	/// Returns true if the Stream is open.
	//////
	bool Stream::isOpen() const
	{
		return (stream_ != NULL);
	}

	// -----------------------------------------------------------------------------------

	void Stream::setStreamFinishedCallback(PaStreamFinishedCallback *callback)
	{
		PaError err = Pa_SetStreamFinishedCallback(stream_, callback);

		if (err != paNoError)
			throw PaException(err);
	}

	// -----------------------------------------------------------------------------------

	void Stream::start()
	{
		PaError err = Pa_StartStream(stream_);

		if (err != paNoError)
			throw PaException(err);
	}

	void Stream::stop()
	{
		PaError err = Pa_StopStream(stream_);

		if (err != paNoError)
			throw PaException(err);
	}

	void Stream::abort()
	{
		PaError err = Pa_AbortStream(stream_);

		if (err != paNoError)
			throw PaException(err);
	}

	bool Stream::isStopped() const
	{
		PaError ret = Pa_IsStreamStopped(stream_);

		if (ret < 0)
			throw PaException(ret);

		return (ret == 1);
	}

	bool Stream::isActive() const
	{
		PaError ret = Pa_IsStreamActive(stream_);

		if (ret < 0)
			throw PaException(ret);

		return (ret == 1);
	}

	// -----------------------------------------------------------------------------------

	//////
	/// Returns the best known input latency for the Stream. This value may differ from the 
	/// suggested input latency set in the StreamParameters. Includes all sources of 
	/// latency known to PortAudio such as internal buffering, and Host API reported latency. 
	/// Doesn't include any estimates of unknown latency.
	//////
	PaTime Stream::inputLatency() const
	{
		const PaStreamInfo *info = Pa_GetStreamInfo(stream_);
		if (info == NULL)
		{
			throw PaException(paInternalError);
			return PaTime(0.0);
		}

		return info->inputLatency;
	}

	//////
	/// Returns the best known output latency for the Stream. This value may differ from the 
	/// suggested output latency set in the StreamParameters. Includes all sources of 
	/// latency known to PortAudio such as internal buffering, and Host API reported latency. 
	/// Doesn't include any estimates of unknown latency.
	//////
	PaTime Stream::outputLatency() const
	{
		const PaStreamInfo *info = Pa_GetStreamInfo(stream_);
		if (info == NULL)
		{
			throw PaException(paInternalError);
			return PaTime(0.0);
		}

		return info->outputLatency;
	}

	//////
	/// Returns the sample rate of the Stream. Usually this will be the 
	/// best known estimate of the used sample rate. For instance when opening a 
	/// Stream setting 44100.0 Hz in the StreamParameters, the actual sample 
	/// rate might be something like 44103.2 Hz (due to imperfections in the 
	/// sound card hardware).
	//////
	double Stream::sampleRate() const
	{
		const PaStreamInfo *info = Pa_GetStreamInfo(stream_);
		if (info == NULL)
		{
			throw PaException(paInternalError);
			return 0.0;
		}

		return info->sampleRate;
	}

	// -----------------------------------------------------------------------------------

	PaTime Stream::time() const
	{
		return Pa_GetStreamTime(stream_);
	}

	// -----------------------------------------------------------------------------------

	//////
	/// Accessor (const) for PortAudio PaStream pointer, useful for interfacing with 
	/// PortAudio add-ons such as PortMixer for instance. Normally accessing this 
	/// pointer should not be needed as PortAudioCpp aims to provide all of PortAudio's 
	/// functionality.
	//////
	const PaStream *Stream::paStream() const
	{
		return stream_;
	}

	//////
	/// Accessor (non-const) for PortAudio PaStream pointer, useful for interfacing with 
	/// PortAudio add-ons such as PortMixer for instance. Normally accessing this 
	/// pointer should not be needed as PortAudioCpp aims to provide all of PortAudio's 
	/// functionality.
	//////
	PaStream *Stream::paStream()
	{
		return stream_;
	}

	// -----------------------------------------------------------------------------------

} // namespace portaudio
