/*
	This file was a scribble area during the early phases of development.

	It's out of date but will probably hang around untill all the content 
	has been cleaned out or found a new home.

	Here's some tasks to undertake:
		o- make sure that the @brief comments in each file are at least
			as good as the file descriptions below, then delete the file
			descriptions below.

		o- make sure that the coding guidelines below appear in the
			pa_proposals style guide, then delete from below.

		o- verify and move the TODO items into TRAC
*/


FILES:

portaudio.h
    public api header file

pa_front.c
    implements the interface defined in portaudio.h. manages multiple host apis.
    validates function parameters before calling through to host apis. tracks
    open streams and closes them at Pa_Terminate().

pa_util.h 
    declares utility functions for use my implementations. including utility
    functions which must be implemented separately for each platform.

pa_hostapi.h
    hostapi representation structure used to interface between pa_front.c
    and implementations
    
pa_stream.c/h
    stream interface and representation structures and helper functions
    used to interface between pa_front.c and implementations

pa_cpuload.c/h
    source and header for cpu load calculation facility

pa_trace.c/h
    source and header for debug trace log facility

pa_converters.c/h
    sample buffer conversion facility

pa_dither.c/h
    dither noise generator

pa_process.c/h
    callback buffer processing facility including interleave and block adaption

pa_allocation.c/h
    allocation context for tracking groups of allocations

pa_skeleton.c
    an skeleton implementation showing how the common code can be used.

pa_win_util.c
    Win32 implementation of platform specific PaUtil functions (memory allocation,
    usec clock, Pa_Sleep().)  The file will be used with all Win32 host APIs.
    
pa_win_hostapis.c
    contains the paHostApiInitializers array and an implementation of
    Pa_GetDefaultHostApi() for win32 builds.

pa_win_wmme.c
    Win32 host api implementation for the windows multimedia extensions audio API.

pa_win_wmme.h
    public header file containing interfaces to mme-specific functions and the
    deviceInfo data structure.


CODING GUIDELINES:

naming conventions:
    #defines begin with PA_
    #defines local to a file end with _
    global utility variables begin with paUtil
    global utility types begin with PaUtil  (including function types)
    global utility functions begin with PaUtil_
    static variables end with _
    static constants begin with const and end with _
    static funtions have no special prefix/suffix

In general, implementations should declare all of their members static,
except for their initializer which should be exported. All exported names
should be preceeded by Pa<MN>_ where MN is the module name, for example
the windows mme initializer should be named PaWinWmme_Initialize().

Every host api should define an initializer which returns an error code
and a PaHostApiInterface*. The initializer should only return an error other
than paNoError if it encounters an unexpected and fatal error (memory allocation
error for example). In general, there may be conditions under which it returns
a NULL interface pointer and also returns paNoError. For example, if the ASIO
implementation detects that ASIO is not installed, it should return a
NULL interface, and paNoError.

Platform-specific shared functions should begin with Pa<PN>_ where PN is the
platform name. eg. PaWin_ for windows, PaUnix_ for unix.

The above two conventions should also be followed whenever it is necessary to
share functions accross multiple source files.

Two utilities for debug messages are provided. The PA_DEBUG macro defined in
pa_implementation.h provides a simple way to print debug messages to stderr.
Due to real-time performance issues, PA_DEBUG may not be suitable for use
within the portaudio processing callback, or in other threads. In such cases
the event tracing facility provided in pa_trace.h may be more appropriate.

If PA_LOG_API_CALLS is defined, all calls to the public PortAudio API
will be logged to stderr along with parameter and return values.


TODO: (these need to be turned into TRAC items)
    write some new tests to exercise the multi-host-api functions

    write (doxygen) documentation for pa_trace (phil?)

    create a global configuration file which documents which PA_ defines can be
    used for configuration

    need a coding standard for comment formatting

    write style guide document (ross)
