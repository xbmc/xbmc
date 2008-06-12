#ifndef INCLUDED_PORTAUDIO_DEVICE_HXX
#define INCLUDED_PORTAUDIO_DEVICE_HXX

// ---------------------------------------------------------------------------------------

#include <iterator>

#include "portaudio.h"

#include "portaudiocpp/SampleDataFormat.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s):
namespace portaudio
{
	class System;
	class HostApi;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{

	//////
	/// @brief Class which represents a PortAudio device in the System.
	///
	/// A single physical device in the system may have multiple PortAudio 
	/// Device representations using different HostApi 's though. A Device 
	/// can be half-duplex or full-duplex. A half-duplex Device can be used 
	/// to create a half-duplex Stream. A full-duplex Device can be used to 
	/// create a full-duplex Stream. If supported by the HostApi, two 
	/// half-duplex Devices can even be used to create a full-duplex Stream.
	///
	/// Note that Device objects are very light-weight and can be passed around 
	/// by-value.
	//////
	class Device
	{
	public:
		// query info: name, max in channels, max out channels, 
		// default low/hight input/output latency, default sample rate
		PaDeviceIndex index() const;
		const char *name() const;
		int maxInputChannels() const;
		int maxOutputChannels() const;
		PaTime defaultLowInputLatency() const;
		PaTime defaultHighInputLatency() const;
		PaTime defaultLowOutputLatency() const;
		PaTime defaultHighOutputLatency() const;
		double defaultSampleRate() const;

		bool isInputOnlyDevice() const; // extended
		bool isOutputOnlyDevice() const; // extended
		bool isFullDuplexDevice() const; // extended
		bool isSystemDefaultInputDevice() const; // extended
		bool isSystemDefaultOutputDevice() const; // extended
		bool isHostApiDefaultInputDevice() const; // extended
		bool isHostApiDefaultOutputDevice() const; // extended

		bool operator==(const Device &rhs);
		bool operator!=(const Device &rhs);

		// host api reference
		HostApi &hostApi();
		const HostApi &hostApi() const;

	private:
		PaDeviceIndex index_;
		const PaDeviceInfo *info_;

	private:
		friend class System;
		
		explicit Device(PaDeviceIndex index);
		~Device();

		Device(const Device &); // non-copyable
		Device &operator=(const Device &); // non-copyable
	};

	// -----------------------------------------------------------------------------------

} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_DEVICE_HXX

