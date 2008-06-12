#ifndef INCLUDED_PORTAUDIO_STREAMPARAMETERS_HXX
#define INCLUDED_PORTAUDIO_STREAMPARAMETERS_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{

	//////
	/// @brief The entire set of parameters needed to configure and open 
	/// a Stream.
	///
	/// It contains parameters of input, output and shared parameters. 
	/// Using the isSupported() method, the StreamParameters can be 
	/// checked if opening a Stream using this StreamParameters would 
	/// succeed or not. Accessors are provided to higher-level parameters 
	/// aswell as the lower-level parameters which are mainly intended for 
	/// internal use.
	//////
	class StreamParameters
	{
	public:
		StreamParameters();
		StreamParameters(const DirectionSpecificStreamParameters &inputParameters, 
			const DirectionSpecificStreamParameters &outputParameters, double sampleRate, 
			unsigned long framesPerBuffer, PaStreamFlags flags);

		// Set up for direction-specific:
		void setInputParameters(const DirectionSpecificStreamParameters &parameters);
		void setOutputParameters(const DirectionSpecificStreamParameters &parameters);

		// Set up for common parameters:
		void setSampleRate(double sampleRate);
		void setFramesPerBuffer(unsigned long framesPerBuffer);
		void setFlag(PaStreamFlags flag);
		void unsetFlag(PaStreamFlags flag);
		void clearFlags();

		// Validation:
		bool isSupported() const;

		// Accessors (direction-specific):
		DirectionSpecificStreamParameters &inputParameters();
		const DirectionSpecificStreamParameters &inputParameters() const;
		DirectionSpecificStreamParameters &outputParameters();
		const DirectionSpecificStreamParameters &outputParameters() const;

		// Accessors (common):
		double sampleRate() const;
		unsigned long framesPerBuffer() const;
		PaStreamFlags flags() const;
		bool isFlagSet(PaStreamFlags flag) const;

	private:
		// Half-duplex specific parameters:
		DirectionSpecificStreamParameters inputParameters_;
		DirectionSpecificStreamParameters outputParameters_;

		// Common parameters:
		double sampleRate_;
		unsigned long framesPerBuffer_;
		PaStreamFlags flags_;
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_STREAMPARAMETERS_HXX
