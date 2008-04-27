#ifndef INCLUDED_PORTAUDIO_BLOCKINGSTREAM_HXX
#define INCLUDED_PORTAUDIO_BLOCKINGSTREAM_HXX

// ---------------------------------------------------------------------------------------

#include "portaudiocpp/Stream.hxx"

// ---------------------------------------------------------------------------------------

namespace portaudio
{



	//////
	/// @brief Stream class for blocking read/write-style input and output.
	//////
	class BlockingStream : public Stream
	{
	public:
		BlockingStream();
		BlockingStream(const StreamParameters &parameters);
		~BlockingStream();

		void open(const StreamParameters &parameters);

		void read(void *buffer, unsigned long numFrames);
		void write(const void *buffer, unsigned long numFrames);

		signed long availableReadSize() const;
		signed long availableWriteSize() const;

	private:
		BlockingStream(const BlockingStream &); // non-copyable
		BlockingStream &operator=(const BlockingStream &); // non-copyable
	};



} // portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_BLOCKINGSTREAM_HXX

