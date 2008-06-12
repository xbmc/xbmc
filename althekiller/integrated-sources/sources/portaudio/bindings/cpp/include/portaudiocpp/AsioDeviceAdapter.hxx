#ifndef INCLUDED_PORTAUDIO_ASIODEVICEADAPTER_HXX
#define INCLUDED_PORTAUDIO_ASIODEVICEADAPTER_HXX

namespace portaudio
{

	// Forward declaration(s):
	class Device;

	// Declaration(s):
	//////
	/// @brief Adapts the given Device to an ASIO specific extension.
	///
	/// Deleting the AsioDeviceAdapter does not affect the underlaying 
	/// Device.
	//////
	class AsioDeviceAdapter
	{
	public:
		AsioDeviceAdapter(Device &device);

		Device &device();

		long minBufferSize() const;
		long maxBufferSize() const;
		long preferredBufferSize() const;
		long granularity() const;

		void showControlPanel(void *systemSpecific);

		const char *inputChannelName(int channelIndex) const;
		const char *outputChannelName(int channelIndex) const;

	private:
		Device *device_;

		long minBufferSize_;
		long maxBufferSize_;
		long preferredBufferSize_;
		long granularity_;
	};
}

#endif // INCLUDED_PORTAUDIO_ASIODEVICEADAPTER_HXX
