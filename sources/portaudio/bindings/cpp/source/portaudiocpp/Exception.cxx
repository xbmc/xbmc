#include "portaudiocpp/Exception.hxx"

namespace portaudio
{
	// -----------------------------------------------------------------------------------
	// PaException:
	// -----------------------------------------------------------------------------------

	//////
	///  Wraps a PortAudio error into a PortAudioCpp PaException.
	//////
	PaException::PaException(PaError error) : error_(error)
	{
	}

	// -----------------------------------------------------------------------------------

	//////
	/// Alias for paErrorText(), to have std::exception compliance.
	//////
	const char *PaException::what() const throw()
	{
		return paErrorText();
	}

	// -----------------------------------------------------------------------------------

	//////
	/// Returns the PortAudio error code (PaError).
	//////
	PaError PaException::paError() const
	{
		return error_;
	}

	//////
	/// Returns the error as a (zero-terminated) text string.
	//////
	const char *PaException::paErrorText() const
	{
		return Pa_GetErrorText(error_);
	}

	//////
	/// Returns true is the error is a HostApi error.
	//////
	bool PaException::isHostApiError() const
	{
		return (error_ == paUnanticipatedHostError);
	}

	//////
	/// Returns the last HostApi error (which is the current one if 
	/// isHostApiError() returns true) as an error code.
	//////
	long PaException::lastHostApiError() const
	{
		return Pa_GetLastHostErrorInfo()->errorCode;
	}

	//////
	/// Returns the last HostApi error (which is the current one if 
	/// isHostApiError() returns true) as a (zero-terminated) text 
	/// string, if it's available.
	//////
	const char *PaException::lastHostApiErrorText() const
	{
		return Pa_GetLastHostErrorInfo()->errorText;
	}

	// -----------------------------------------------------------------------------------

	bool PaException::operator==(const PaException &rhs) const
	{
		return (error_ == rhs.error_);
	}

	bool PaException::operator!=(const PaException &rhs) const
	{
		return !(*this == rhs);
	}

	// -----------------------------------------------------------------------------------
	// PaCppException:
	// -----------------------------------------------------------------------------------
	
	PaCppException::PaCppException(ExceptionSpecifier specifier) : specifier_(specifier)
	{
	}

	const char *PaCppException::what() const throw()
	{
		switch (specifier_)
		{
			case UNABLE_TO_ADAPT_DEVICE:
			{
				return "Unable to adapt the given device to the specified host api specific device extension";
			}
		}

		return "Unknown exception";
	}

	PaCppException::ExceptionSpecifier PaCppException::specifier() const
	{
		return specifier_;
	}

	bool PaCppException::operator==(const PaCppException &rhs) const
	{
		return (specifier_ == rhs.specifier_);
	}

	bool PaCppException::operator!=(const PaCppException &rhs) const
	{
		return !(*this == rhs);
	}

	// -----------------------------------------------------------------------------------

} // namespace portaudio


