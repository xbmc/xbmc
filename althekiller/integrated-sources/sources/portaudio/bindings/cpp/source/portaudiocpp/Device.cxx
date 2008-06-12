#include "portaudiocpp/Device.hxx"

#include <cstddef>

#include "portaudiocpp/HostApi.hxx"
#include "portaudiocpp/System.hxx"
#include "portaudiocpp/Exception.hxx"

namespace portaudio
{

		// -------------------------------------------------------------------------------

		Device::Device(PaDeviceIndex index) : index_(index)
		{
			if (index == paNoDevice)
				info_ = NULL;
			else
				info_ = Pa_GetDeviceInfo(index);
		}

		Device::~Device()
		{
		}

		PaDeviceIndex Device::index() const
		{
			return index_;
		}

		const char *Device::name() const
		{
			if (info_ == NULL)
				return "";

			return info_->name;
		}

		int Device::maxInputChannels() const
		{
			if (info_ == NULL)
				return 0;

			return info_->maxInputChannels;
		}

		int Device::maxOutputChannels() const
		{
			if (info_ == NULL)
				return 0;

			return info_->maxOutputChannels;
		}

		PaTime Device::defaultLowInputLatency() const
		{
			if (info_ == NULL)
				return static_cast<PaTime>(0.0);

			return info_->defaultLowInputLatency;
		}

		PaTime Device::defaultHighInputLatency() const
		{
			if (info_ == NULL)
				return static_cast<PaTime>(0.0);

			return info_->defaultHighInputLatency;
		}

		PaTime Device::defaultLowOutputLatency() const
		{
			if (info_ == NULL)
				return static_cast<PaTime>(0.0);

			return info_->defaultLowOutputLatency;
		}

		PaTime Device::defaultHighOutputLatency() const
		{
			if (info_ == NULL)
				return static_cast<PaTime>(0.0);

			return info_->defaultHighOutputLatency;
		}

		double Device::defaultSampleRate() const
		{
			if (info_ == NULL)
				return 0.0;

			return info_->defaultSampleRate;
		}

		// -------------------------------------------------------------------------------

		bool Device::isInputOnlyDevice() const
		{
			return (maxOutputChannels() == 0);
		}

		bool Device::isOutputOnlyDevice() const
		{
			return (maxInputChannels() == 0);
		}

		bool Device::isFullDuplexDevice() const
		{
			return (maxInputChannels() > 0 && maxOutputChannels() > 0);
		}

		bool Device::isSystemDefaultInputDevice() const
		{
			return (System::instance().defaultInputDevice() == *this);
		}

		bool Device::isSystemDefaultOutputDevice() const
		{
			return (System::instance().defaultOutputDevice() == *this);
		}

		bool Device::isHostApiDefaultInputDevice() const
		{
			return (hostApi().defaultInputDevice() == *this);
		}

		bool Device::isHostApiDefaultOutputDevice() const
		{
			return (hostApi().defaultOutputDevice() == *this);
		}

		// -------------------------------------------------------------------------------

		bool Device::operator==(const Device &rhs)
		{
			return (index_ == rhs.index_);
		}

		bool Device::operator!=(const Device &rhs)
		{
			return !(*this == rhs);
		}

		// -------------------------------------------------------------------------------

		HostApi &Device::hostApi()
		{
			// NOTE: will cause an exception when called for the null device
			if (info_ == NULL)
				throw PaException(paInternalError);

			return System::instance().hostApiByIndex(info_->hostApi);
		}

		const HostApi &Device::hostApi() const
		{
			// NOTE; will cause an exception when called for the null device
			if (info_ == NULL)
				throw PaException(paInternalError);

			return System::instance().hostApiByIndex(info_->hostApi);
		}

		// -------------------------------------------------------------------------------

} // namespace portaudio


