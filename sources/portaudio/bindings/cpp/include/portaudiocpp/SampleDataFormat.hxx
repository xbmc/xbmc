#ifndef INCLUDED_PORTAUDIO_SAMPLEDATAFORMAT_HXX
#define INCLUDED_PORTAUDIO_SAMPLEDATAFORMAT_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

// ---------------------------------------------------------------------------------------

namespace portaudio
{


	//////
	/// @brief PortAudio sample data formats.
	///
	/// Small helper enum to wrap the PortAudio defines.
	//////
	enum SampleDataFormat
	{
		INVALID_FORMAT	= 0,
		FLOAT32			= paFloat32,
		INT32			= paInt32,
		INT24			= paInt24,
		INT16			= paInt16,
		INT8			= paInt8,
		UINT8			= paUInt8
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_SAMPLEDATAFORMAT_HXX
