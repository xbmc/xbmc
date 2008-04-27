#ifndef INCLUDED_PORTAUDIO_SYSTEMHOSTAPIITERATOR_HXX
#define INCLUDED_PORTAUDIO_SYSTEMHOSTAPIITERATOR_HXX

// ---------------------------------------------------------------------------------------

#include <iterator>
#include <cstddef>

#include "portaudiocpp/System.hxx"

// ---------------------------------------------------------------------------------------

// Forward declaration(s):
namespace portaudio
{
	class HostApi;
}

// ---------------------------------------------------------------------------------------

// Declaration(s):
namespace portaudio
{


	//////
	/// @brief Iterator class for iterating through all HostApis in a System.
	///
	/// Compliant with the STL bidirectional iterator concept.
	//////
	class System::HostApiIterator
	{
	public:
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef Device value_type;
		typedef ptrdiff_t difference_type;
		typedef HostApi * pointer;
		typedef HostApi & reference;

		HostApi &operator*() const;
		HostApi *operator->() const;

		HostApiIterator &operator++();
		HostApiIterator operator++(int);
		HostApiIterator &operator--();
		HostApiIterator operator--(int);

		bool operator==(const HostApiIterator &rhs);
		bool operator!=(const HostApiIterator &rhs);

	private:
		friend class System;
		HostApi **ptr_;
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_SYSTEMHOSTAPIITERATOR_HXX
