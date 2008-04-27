#include "portaudiocpp/CallbackStream.hxx"

namespace portaudio
{
	CallbackStream::CallbackStream()
	{
	}

	CallbackStream::~CallbackStream()
	{
	}

	// -----------------------------------------------------------------------------------
	
	double CallbackStream::cpuLoad() const
	{
		return Pa_GetStreamCpuLoad(stream_);
	}

} // namespace portaudio
