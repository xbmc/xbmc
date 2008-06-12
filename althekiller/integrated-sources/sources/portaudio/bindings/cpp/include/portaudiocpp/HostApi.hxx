#ifndef INCLUDED_PORTAUDIO_HOSTAPI_HXX
#define INCLUDED_PORTAUDIO_HOSTAPI_HXX

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/System.hxx"

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
	/// @brief HostApi represents a host API (usually type of driver) in the System.
	///
	/// A single System can support multiple HostApi's each one typically having 
	/// a set of Devices using that HostApi (usually driver type). All Devices in 
	/// the HostApi can be enumerated and the default input/output Device for this 
	/// HostApi can be retreived.
	//////
	class HostApi
	{
	public:
		typedef System::DeviceIterator DeviceIterator;

		// query info: id, name, numDevices
		PaHostApiTypeId typeId() const;
		PaHostApiIndex index() const;
		const char *name() const;
		int deviceCount() const;

		// iterate devices
		DeviceIterator devicesBegin();
		DeviceIterator devicesEnd();

		// default devices
		Device &defaultInputDevice() const;
		Device &defaultOutputDevice() const;

		// comparison operators
		bool operator==(const HostApi &rhs) const;
		bool operator!=(const HostApi &rhs) const;

	private:
		const PaHostApiInfo *info_;
		Device **devices_;

	private:
		friend class System;

		explicit HostApi(PaHostApiIndex index);
		~HostApi();

		HostApi(const HostApi &); // non-copyable
		HostApi &operator=(const HostApi &); // non-copyable
	};


}

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_HOSTAPI_HXX

