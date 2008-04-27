#include "portaudiocpp/SystemDeviceIterator.hxx"

namespace portaudio
{
	// -----------------------------------------------------------------------------------

	Device &System::DeviceIterator::operator*() const
	{
		return **ptr_;
	}

	Device *System::DeviceIterator::operator->() const
	{
		return &**this;
	}

	// -----------------------------------------------------------------------------------

	System::DeviceIterator &System::DeviceIterator::operator++()
	{
		++ptr_;
		return *this;
	}

	System::DeviceIterator System::DeviceIterator::operator++(int)
	{
		System::DeviceIterator prev = *this;
		++*this;
		return prev;
	}

	System::DeviceIterator &System::DeviceIterator::operator--()
	{
		--ptr_;
		return *this;
	}

	System::DeviceIterator System::DeviceIterator::operator--(int)
	{
		System::DeviceIterator prev = *this;
		--*this;
		return prev;
	}

	// -----------------------------------------------------------------------------------

	bool System::DeviceIterator::operator==(const System::DeviceIterator &rhs)
	{
		return (ptr_ == rhs.ptr_);
	}

	bool System::DeviceIterator::operator!=(const System::DeviceIterator &rhs)
	{
		return !(*this == rhs);
	}

	// -----------------------------------------------------------------------------------
} // namespace portaudio


