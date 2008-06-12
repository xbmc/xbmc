#include "portaudiocpp/StreamParameters.hxx"

#include <cstddef>

#include "portaudiocpp/Device.hxx"

namespace portaudio
{
	// -----------------------------------------------------------------------------------

	//////
	/// Default constructor; does nothing.
	//////
	StreamParameters::StreamParameters()
	{
	}

	//////
	/// Sets up the all parameters needed to open either a half-duplex or full-duplex Stream.
	///
	/// @param inputParameters The parameters for the input direction of the to-be opened 
	/// Stream or DirectionSpecificStreamParameters::null() for an output-only Stream.
	/// @param outputParameters The parameters for the output direction of the to-be opened
	/// Stream or DirectionSpecificStreamParameters::null() for an input-only Stream.
	/// @param sampleRate The to-be opened Stream's sample rate in Hz.
	/// @param framesPerBuffer The number of frames per buffer for a CallbackStream, or 
	/// the preferred buffer granularity for a BlockingStream.
	/// @param flags The flags for the to-be opened Stream; default paNoFlag.
	//////
	StreamParameters::StreamParameters(const DirectionSpecificStreamParameters &inputParameters, 
		const DirectionSpecificStreamParameters &outputParameters, double sampleRate, unsigned long framesPerBuffer, 
		PaStreamFlags flags) : inputParameters_(inputParameters), outputParameters_(outputParameters), 
		sampleRate_(sampleRate), framesPerBuffer_(framesPerBuffer), flags_(flags)
	{
	}

	// -----------------------------------------------------------------------------------

	//////
	/// Sets the requested sample rate. If this sample rate isn't supported by the hardware, the 
	/// Stream will fail to open. The real-life sample rate used might differ slightly due to 
	/// imperfections in the sound card hardware; use Stream::sampleRate() to retreive the 
	/// best known estimate for this value.
	//////
	void StreamParameters::setSampleRate(double sampleRate)
	{
		sampleRate_ = sampleRate;
	}

	//////
	/// Either the number of frames per buffer for a CallbackStream, or 
	/// the preferred buffer granularity for a BlockingStream. See PortAudio 
	/// documentation.
	//////
	void StreamParameters::setFramesPerBuffer(unsigned long framesPerBuffer)
	{
		framesPerBuffer_ = framesPerBuffer;
	}

	//////
	/// Sets the specified flag or does nothing when the flag is already set. Doesn't 
	/// `unset' any previously existing flags (use clearFlags() for that).
	//////
	void StreamParameters::setFlag(PaStreamFlags flag)
	{
		flags_ |= flag;
	}

	//////
	/// Unsets the specified flag or does nothing if the flag isn't set. Doesn't affect 
	/// any other flags.
	//////
	void StreamParameters::unsetFlag(PaStreamFlags flag)
	{
		flags_ &= ~flag;
	}

	//////
	/// Clears or `unsets' all set flags.
	//////
	void StreamParameters::clearFlags()
	{
		flags_ = paNoFlag;
	}

	// -----------------------------------------------------------------------------------

	void StreamParameters::setInputParameters(const DirectionSpecificStreamParameters &parameters)
	{
		inputParameters_ = parameters;
	}

	void StreamParameters::setOutputParameters(const DirectionSpecificStreamParameters &parameters)
	{
		outputParameters_ = parameters;
	}

	// -----------------------------------------------------------------------------------

	bool StreamParameters::isSupported() const
	{
		return (Pa_IsFormatSupported(inputParameters_.paStreamParameters(), 
			outputParameters_.paStreamParameters(), sampleRate_) == paFormatIsSupported);
	}

	// -----------------------------------------------------------------------------------

	double StreamParameters::sampleRate() const
	{
		return sampleRate_;
	}

	unsigned long StreamParameters::framesPerBuffer() const
	{
		return framesPerBuffer_;
	}

	//////
	/// Returns all currently set flags as a binary combined 
	/// integer value (PaStreamFlags). Use isFlagSet() to 
	/// avoid dealing with the bitmasks.
	//////
	PaStreamFlags StreamParameters::flags() const
	{
		return flags_;
	}

	//////
	/// Returns true if the specified flag is currently set 
	/// or false if it isn't.
	//////
	bool StreamParameters::isFlagSet(PaStreamFlags flag) const
	{
		return ((flags_ & flag) != 0);
	}

	// -----------------------------------------------------------------------------------

	DirectionSpecificStreamParameters &StreamParameters::inputParameters()
	{
		return inputParameters_;
	}

	const DirectionSpecificStreamParameters &StreamParameters::inputParameters() const
	{
		return inputParameters_;
	}

	DirectionSpecificStreamParameters &StreamParameters::outputParameters()
	{
		return outputParameters_;
	}

	const DirectionSpecificStreamParameters &StreamParameters::outputParameters() const
	{
		return outputParameters_;
	}

	// -----------------------------------------------------------------------------------
} // namespace portaudio





