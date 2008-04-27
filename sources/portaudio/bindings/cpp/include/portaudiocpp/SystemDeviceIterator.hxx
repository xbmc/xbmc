#ifndef INCLUDED_PORTAUDIO_SYSTEMDEVICEITERATOR_HXX
#define INCLUDED_PORTAUDIO_SYSTEMDEVICEITERATOR_HXX

// ---------------------------------------------------------------------------------------

#include <iterator>
#include <cstddef>

#include "portaudiocpp/System.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s):
namespace portaudio
{
	class Device;
	class HostApi;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{

	
	//////
	/// @brief Iterator class for iterating through all Devices in a System.
	///
	/// Devices will be iterated by iterating all Devices in each 
	/// HostApi in the System. Compliant with the STL bidirectional 
	/// iterator concept.
	//////
	class System::DeviceIterator
	{
	public:
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef Device value_type;
		typedef ptrdiff_t difference_type;
		typedef Device * pointer;
		typedef Device & reference;

		Device &operator*() const;
		Device *operator->() const;

		DeviceIterator &operator++();
		DeviceIterator operator++(int);
		DeviceIterator &operator--();
		DeviceIterator operator--(int);

		bool operator==(const DeviceIterator &rhs);
		bool operator!=(const DeviceIterator &rhs);

	private:
		friend class System;
		friend class HostApi;
		Device **ptr_;
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_SYSTEMDEVICEITERATOR_HXX

