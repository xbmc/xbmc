#include "portaudiocpp/AsioDeviceAdapter.hxx"

#include "portaudio.h"
#include "pa_asio.h"

#include "portaudiocpp/Device.hxx"
#include "portaudiocpp/HostApi.hxx"
#include "portaudiocpp/Exception.hxx"

namespace portaudio
{
	AsioDeviceAdapter::AsioDeviceAdapter(Device &device)
	{
		if (device.hostApi().typeId() != paASIO)
			throw PaCppException(PaCppException::UNABLE_TO_ADAPT_DEVICE);

		device_ = &device;

		PaError err = PaAsio_GetAvailableLatencyValues(device_->index(), &minBufferSize_, &maxBufferSize_, 
			&preferredBufferSize_, &granularity_);

		if (err != paNoError)
			throw PaException(err);

	}

	Device &AsioDeviceAdapter::device()
	{
		return *device_;
	}

	long AsioDeviceAdapter::minBufferSize() const
	{
		return minBufferSize_;
	}

	long AsioDeviceAdapter::maxBufferSize() const
	{
		return maxBufferSize_;
	}

	long AsioDeviceAdapter::preferredBufferSize() const
	{
		return preferredBufferSize_;
	}

	long AsioDeviceAdapter::granularity() const
	{
		return granularity_;
	}

	void AsioDeviceAdapter::showControlPanel(void *systemSpecific)
	{
		PaError err = PaAsio_ShowControlPanel(device_->index(), systemSpecific);

		if (err != paNoError)
			throw PaException(err);
	}

	const char *AsioDeviceAdapter::inputChannelName(int channelIndex) const
	{
		const char *channelName;
		PaError err = PaAsio_GetInputChannelName(device_->index(), channelIndex, &channelName);

		if (err != paNoError)
			throw PaException(err);

		return channelName;
	}

	const char *AsioDeviceAdapter::outputChannelName(int channelIndex) const
	{
		const char *channelName;
		PaError err = PaAsio_GetOutputChannelName(device_->index(), channelIndex, &channelName);

		if (err != paNoError)
			throw PaException(err);

		return channelName;
	}
}


