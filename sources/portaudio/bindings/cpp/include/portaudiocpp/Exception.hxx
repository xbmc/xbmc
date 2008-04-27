#ifndef INCLUDED_PORTAUDIO_EXCEPTION_HXX
#define INCLUDED_PORTAUDIO_EXCEPTION_HXX

// ---------------------------------------------------------------------------------------

#include <exception>

#include "portaudio.h"

// ---------------------------------------------------------------------------------------

namespace portaudio
{

	//////
	/// @brief Base class for all exceptions PortAudioCpp can throw.
	///
	/// Class is derived from std::exception.
	//////
	class Exception : public std::exception
	{
	public:
		virtual ~Exception() throw() {}

		virtual const char *what() const throw() = 0;
	};
	
	// -----------------------------------------------------------------------------------

	//////
	/// @brief Wrapper for PortAudio error codes to C++ exceptions.
	///
	/// It wraps up PortAudio's error handling mechanism using 
	/// C++ exceptions and is derived from std::exception for 
	/// easy exception handling and to ease integration with 
	/// other code.
	///
	/// To know what exceptions each function may throw, look up 
	/// the errors that can occure in the PortAudio documentation 
	/// for the equivalent functions.
	///
	/// Some functions are likely to throw an exception (such as 
	/// Stream::open(), etc) and these should always be called in 
	/// try{} catch{} blocks and the thrown exceptions should be 
	/// handled properly (ie. the application shouldn't just abort, 
	/// but merely display a warning dialog to the user or something).
	/// However nearly all functions in PortAudioCpp are capable 
	/// of throwing exceptions. When a function like Stream::isStopped() 
	/// throws an exception, it's such an exceptional state that it's 
	/// not likely that it can be recovered. PaExceptions such as these 
	/// can ``safely'' be left to be handled by some outer catch-all-like 
	/// mechanism for unrecoverable errors.
	//////
	class PaException : public Exception
	{
	public:
		explicit PaException(PaError error);

		const char *what() const throw();

		PaError paError() const;
		const char *paErrorText() const;

		bool isHostApiError() const; // extended
		long lastHostApiError() const;
		const char *lastHostApiErrorText() const;

		bool operator==(const PaException &rhs) const;
		bool operator!=(const PaException &rhs) const;

	private:
		PaError error_;
 	};

	// -----------------------------------------------------------------------------------

	//////
	/// @brief Exceptions specific to PortAudioCpp (ie. exceptions which do not have an 
	/// equivalent PortAudio error code).
	//////
	class PaCppException : public Exception
	{
	public:
		enum ExceptionSpecifier
		{
			UNABLE_TO_ADAPT_DEVICE
		};

		PaCppException(ExceptionSpecifier specifier);

		const char *what() const throw();

		ExceptionSpecifier specifier() const;

		bool operator==(const PaCppException &rhs) const;
		bool operator!=(const PaCppException &rhs) const;

	private:
		ExceptionSpecifier specifier_;
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_EXCEPTION_HXX

