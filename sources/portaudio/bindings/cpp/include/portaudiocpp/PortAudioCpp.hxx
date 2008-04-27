#ifndef INCLUDED_PORTAUDIO_PORTAUDIOCPP_HXX
#define INCLUDED_PORTAUDIO_PORTAUDIOCPP_HXX

// ---------------------------------------------------------------------------------------

//////
/// @mainpage PortAudioCpp
///
///	<h1>PortAudioCpp - A Native C++ Binding of PortAudio V19</h1>
/// <h2>PortAudio</h2>
/// <p>
///   PortAudio is a portable and mature C API for accessing audio hardware. It offers both callback-based and blocking 
///   style input and output, deals with sample data format conversions, dithering and much more. There are a large number 
///   of implementations available for various platforms including Windows MME, Windows DirectX, Windows and MacOS (Classic) 
///   ASIO, MacOS Classic SoundManager, MacOS X CoreAudio, OSS (Linux), Linux ALSA, JACK (MacOS X and Linux) and SGI Irix 
///   AL. Note that, currently not all of these implementations are equally complete or up-to-date (as PortAudio V19 is 
///   still in development). Because PortAudio has a C API, it can easily be called from a variety of other programming 
///   languages.
/// </p>
/// <h2>PortAudioCpp</h2>
/// <p>
///   Although, it is possible to use PortAudio's C API from within a C++ program, this is usually a little awkward 
///   as procedural and object-oriented paradigms need to be mixed. PortAudioCpp aims to resolve this by encapsulating 
///   PortAudio's C API to form an equivalent object-oriented C++ API. It provides a more natural integration of PortAudio 
///   into C++ programs as well as a more structured interface. PortAudio's concepts were preserved as much as possible and 
///   no additional features were added except for some `convenience methods'.
/// </p>
/// <p>
///   PortAudioCpp's main features are:
///   <ul>
///     <li>Structured object model.</li>
///     <li>C++ exception handling instead of C-style error return codes.</li>
///     <li>Handling of callbacks using free functions (C and C++), static functions, member functions or instances of classes 
///     derived from a given interface.</li>
///     <li>STL compliant iterators to host APIs and devices.</li>
///     <li>Some additional convenience functions to more easily set up and use PortAudio.</li>
///   </ul>
/// </p>
/// <p>
///   PortAudioCpp requires a recent version of the PortAudio V19 source code. This can be obtained from CVS or as a snapshot 
///   from the website. The examples also require the ASIO 2 SDK which can be obtained from the Steinberg website. Alternatively, the 
///   examples can easily be modified to compile without needing ASIO.
/// </p>
/// <p>
///   Supported platforms:
///   <ul>
///     <li>Microsoft Visual C++ 6.0, 7.0 (.NET 2002) and 7.1 (.NET 2003).</li>
///     <li>GNU G++ 2.95 and G++ 3.3.</li>
///   </ul>
///   Other platforms should be easily supported as PortAudioCpp is platform-independent and (reasonably) C++ standard compliant.
/// </p>
/// <p>
///   This documentation mainly provides information specific to PortAudioCpp. For a more complete explaination of all of the 
///   concepts used, please consult the PortAudio documentation.
/// </p>
/// <p>
///   PortAudioCpp was developed by Merlijn Blaauw with many great suggestions and help from Ross Bencina. Ludwig Schwardt provided 
///   GNU/Linux build files and checked G++ compatibility. PortAudioCpp may be used under the same licensing, conditions and 
///   warranty as PortAudio. See <a href="http://www.portaudio.com/license.html">the PortAudio license</a> for more details.
/// </p>
/// <h2>Links</h2>
/// <p>
///   <a href="http://www.portaudio.com/">Official PortAudio site.</a><br>
/// </p>
//////

// ---------------------------------------------------------------------------------------

//////
/// @namespace portaudio
///
/// To avoid name collision, everything in PortAudioCpp is in the portaudio 
/// namespace. If this name is too long it's usually pretty safe to use an 
/// alias like ``namespace pa = portaudio;''.
//////

// ---------------------------------------------------------------------------------------

//////
/// @file PortAudioCpp.hxx
/// An include-all header file (for lazy programmers and using pre-compiled headers).
//////

// ---------------------------------------------------------------------------------------

#include "portaudio.h"

#include "portaudiocpp/AutoSystem.hxx"
#include "portaudiocpp/BlockingStream.hxx"
#include "portaudiocpp/CallbackInterface.hxx"
#include "portaudiocpp/CallbackStream.hxx"
#include "portaudiocpp/CFunCallbackStream.hxx"
#include "portaudiocpp/CppFunCallbackStream.hxx"
#include "portaudiocpp/Device.hxx"
#include "portaudiocpp/Exception.hxx"
#include "portaudiocpp/HostApi.hxx"
#include "portaudiocpp/InterfaceCallbackStream.hxx"
#include "portaudiocpp/MemFunCallbackStream.hxx"
#include "portaudiocpp/SampleDataFormat.hxx"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include "portaudiocpp/Stream.hxx"
#include "portaudiocpp/StreamParameters.hxx"
#include "portaudiocpp/System.hxx"
#include "portaudiocpp/SystemDeviceIterator.hxx"
#include "portaudiocpp/SystemHostApiIterator.hxx"

// ---------------------------------------------------------------------------------------

#endif // INCLUDED_PORTAUDIO_PORTAUDIOCPP_HXX
