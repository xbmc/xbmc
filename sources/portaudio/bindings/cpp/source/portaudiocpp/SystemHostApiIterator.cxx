#include "portaudiocpp/SystemHostApiIterator.hxx"

namespace portaudio
{
	// -----------------------------------------------------------------------------------

	HostApi &System::HostApiIterator::operator*() const
	{
		return **ptr_;
	}

	HostApi *System::HostApiIterator::operator->() const
	{
		return &**this;
	}

	// -----------------------------------------------------------------------------------

	System::HostApiIterator &System::HostApiIterator::operator++()
	{
		++ptr_;
		return *this;
	}

	System::HostApiIterator System::HostApiIterator::operator++(int)
	{
		System::HostApiIterator prev = *this;
		++*this;
		return prev;
	}

	System::HostApiIterator &System::HostApiIterator::operator--()
	{
		--ptr_;
		return *this;
	}

	System::HostApiIterator System::HostApiIterator::operator--(int)
	{
		System::HostApiIterator prev = *this;
		--*this;
		return prev;
	}

	// -----------------------------------------------------------------------------------

	bool System::HostApiIterator::operator==(const System::HostApiIterator &rhs)
	{
		return (ptr_ == rhs.ptr_);
	}

	bool System::HostApiIterator::operator!=(const System::HostApiIterator &rhs)
	{
		return !(*this == rhs);
	}

	// -----------------------------------------------------------------------------------
} // namespace portaudio

