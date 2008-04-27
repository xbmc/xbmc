#ifndef INCLUDED_PORTAUDIO_SINGLEDIRECTIONSTREAMPARAMETERS_HXX
#define INCLUDED_PORTAUDIO_SINGLEDIRECTIONSTREAMPARAMETERS_HXX

// ---------------------------------------------------------------------------------------

#include <cstddef>

#include "portaudio.h"

#include "portaudiocpp/System.hxx"
#include "portaudiocpp/SampleDataFormat.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s):
namespace portaudio
{
	class Device;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{

	//////
	/// @brief All parameters for one direction (either in or out) of a Stream. Together with 
	/// parameters common to both directions, two DirectionSpecificStreamParameters can make up 
	/// a StreamParameters object which contains all parameters for a Stream.
	//////
	class DirectionSpecificStreamParameters
	{
	public:
		static DirectionSpecificStreamParameters null();

		DirectionSpecificStreamParameters();
		DirectionSpecificStreamParameters(const Device &device, int numChannels, SampleDataFormat format, 
			bool interleaved, PaTime suggestedLatency, void *hostApiSpecificStreamInfo);

		// Set up methods:
		void setDevice(const Device &device);
		void setNumChannels(int numChannels);

		void setSampleFormat(SampleDataFormat format, bool interleaved = true);
		void setHostApiSpecificSampleFormat(PaSampleFormat format, bool interleaved = true);

		void setSuggestedLatency(PaTime latency);

		void setHostApiSpecificStreamInfo(void *streamInfo);

		// Accessor methods:
		PaStreamParameters *paStreamParameters();
		const PaStreamParameters *paStreamParameters() const;

		Device &device() const;
		int numChannels() const;

		SampleDataFormat sampleFormat() const;
		bool isSampleFormatInterleaved() const;
		bool isSampleFormatHostApiSpecific() const;
		PaSampleFormat hostApiSpecificSampleFormat() const;

		PaTime suggestedLatency() const;

		void *hostApiSpecificStreamInfo() const;
	
	private:
		PaStreamParameters paStreamParameters_;
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_SINGLEDIRECTIONSTREAMPARAMETERS_HXX
