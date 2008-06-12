#ifndef INCLUDED_PORTAUDIO_AUTOSYSTEM_HXX
#define INCLUDED_PORTAUDIO_AUTOSYSTEM_HXX

// ---------------------------------------------------------------------------------------

#include "portaudiocpp/System.hxx"

// ---------------------------------------------------------------------------------------

namespace portaudio
{


	//////
	/// @brief A RAII idiom class to ensure automatic clean-up when an exception is 
	/// raised.
	///
	/// A simple helper class which uses the 'Resource Acquisition is Initialization' 
	/// idiom (RAII). Use this class to initialize/terminate the System rather than 
	/// using System directly. AutoSystem must be created on stack and must be valid 
	/// throughout the time you wish to use PortAudioCpp. Your 'main' function might be 
	/// a good place for it.
	///
	/// To avoid having to type portaudio::System::instance().xyz() all the time, it's usually 
	/// a good idea to make a reference to the System which can be accessed directly.
	/// @verbatim
	/// portaudio::AutoSys autoSys;
	/// portaudio::System &sys = portaudio::System::instance();
	/// @endverbatim
	//////
	class AutoSystem
	{
	public:
		AutoSystem(bool initialize = true)
		{
			if (initialize)
				System::initialize();
		}

		~AutoSystem()
		{
			if (System::exists())
				System::terminate();
		}

		void initialize()
		{
			System::initialize();
		}

		void terminate()
		{
			System::terminate();
		}
	};


} // namespace portaudio

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_AUTOSYSTEM_HXX
