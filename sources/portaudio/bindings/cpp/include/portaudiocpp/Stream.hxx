#ifndef INCLUDED_PORTAUDIO_STREAM_HXX
#define INCLUDED_PORTAUDIO_STREAM_HXX

#include "portaudio.h"

// ---------------------------------------------------------------------------------------

// Forward declaration(s):
namespace portaudio
{
	class StreamParameters;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{


	//////
	/// @brief A Stream represents an active or inactive input and/or output data 
	/// stream in the System.
	/// 
	/// Concrete Stream classes should ensure themselves being in a closed state at 
	/// destruction (i.e. by calling their own close() method in their deconstructor). 
	/// Following good C++ programming practices, care must be taken to ensure no 
	/// exceptions are thrown by the deconstructor of these classes. As a consequence, 
	/// clients need to explicitly call close() to ensure the stream closed successfully.
	///
	/// The Stream object can be used to manipulate the Stream's state. Also, time-constant 
	/// and time-varying information about the Stream can be retreived.
	//////
	class Stream
	{
	public:
		// Opening/closing:
		virtual ~Stream();

		virtual void close();
		bool isOpen() const;

		// Additional set up:
		void setStreamFinishedCallback(PaStreamFinishedCallback *callback);

		// State management:
		void start();
		void stop();
		void abort();

		bool isStopped() const;
		bool isActive() const;

		// Stream info (time-constant, but might become time-variant soon):
		PaTime inputLatency() const;
		PaTime outputLatency() const;
		double sampleRate() const;

		// Stream info (time-varying):
		PaTime time() const;

		// Accessors for PortAudio PaStream, useful for interfacing 
		// with PortAudio add-ons (such as PortMixer) for instance:
		const PaStream *paStream() const;
		PaStream *paStream();

	protected:
		Stream(); // abstract class

		PaStream *stream_;

	private:
		Stream(const Stream &); // non-copyable
		Stream &operator=(const Stream &); // non-copyable
	};


} // namespace portaudio


#endif // INCLUDED_PORTAUDIO_STREAM_HXX

