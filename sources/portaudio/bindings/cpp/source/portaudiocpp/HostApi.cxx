#include "portaudiocpp/HostApi.hxx"

#include "portaudiocpp/System.hxx"
#include "portaudiocpp/Device.hxx"
#include "portaudiocpp/SystemDeviceIterator.hxx"
#include "portaudiocpp/Exception.hxx"

namespace portaudio
{

	// -----------------------------------------------------------------------------------

	HostApi::HostApi(PaHostApiIndex index) : devices_(NULL)
	{
		try
		{
			info_ = Pa_GetHostApiInfo(index);

			// Create and populate devices array:
			int numDevices = deviceCount();

			devices_ = new Device*[numDevices];

			for (int i = 0; i < numDevices; ++i)
			{
				PaDeviceIndex deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(index, i);

				if (deviceIndex < 0)
				{
					throw PaException(deviceIndex);
				}

				devices_[i] = &System::instance().deviceByIndex(deviceIndex);
			}
		}
		catch (const std::exception &e)
		{
			// Delete any (partially) constructed objects (deconstructor isn't called):
			delete[] devices_; // devices_ is either NULL or valid

			// Re-throw exception:
			throw e;
		}
	}

	HostApi::~HostApi()
	{
		// Destroy devices array:
		delete[] devices_;
	}

	// -----------------------------------------------------------------------------------

	PaHostApiTypeId HostApi::typeId() const
	{
		return info_->type;
	}

	PaHostApiIndex HostApi::index() const
	{
		PaHostApiIndex index = Pa_HostApiTypeIdToHostApiIndex(typeId());

		if (index < 0)
			throw PaException(index);

		return index;
	}

	const char *HostApi::name() const
	{
		return info_->name;
	}

	int HostApi::deviceCount() const
	{
		return info_->deviceCount;
	}

	// -----------------------------------------------------------------------------------

	HostApi::DeviceIterator HostApi::devicesBegin()
	{
		DeviceIterator tmp;
		tmp.ptr_ = &devices_[0]; // begin (first element)
		return tmp;
	}

	HostApi::DeviceIterator HostApi::devicesEnd()
	{
		DeviceIterator tmp;
		tmp.ptr_ = &devices_[deviceCount()]; // end (one past last element)
		return tmp;
	}

	// -----------------------------------------------------------------------------------

	Device &HostApi::defaultInputDevice() const
	{
		return System::instance().deviceByIndex(info_->defaultInputDevice);
	}

	Device &HostApi::defaultOutputDevice() const
	{
		return System::instance().deviceByIndex(info_->defaultOutputDevice);
	}

	// -----------------------------------------------------------------------------------

	bool HostApi::operator==(const HostApi &rhs) const
	{
		return (typeId() == rhs.typeId());
	}

	bool HostApi::operator!=(const HostApi &rhs) const
	{
		return !(*this == rhs);
	}

	// -----------------------------------------------------------------------------------

} // namespace portaudio
