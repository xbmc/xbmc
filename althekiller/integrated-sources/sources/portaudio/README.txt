README for PortAudio

/*
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * Copyright (c) 1999-2006 Phil Burk and Ross Bencina
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */


PortAudio is a portable audio I/O library designed for cross-platform
support of audio. It uses either a callback mechanism to request audio 
processing, or blocking read/write calls to buffer data between the 
native audio subsystem and the client. Audio can be processed in various 
formats, including 32 bit floating point, and will be converted to the 
native format internally.

Documentation:
	Documentation is available in "/doc/html/index.html"
	Also see "src/common/portaudio.h" for API spec.
	Also see http://www.portaudio.com/docs/
	And see "tests/patest_saw.c" for an example.

For information on compiling programs with PortAudio, please see the
tutorial at:

  http://portaudio.com/trac/wiki/TutorialDir/TutorialStart
  

Important Files and Folders:
    include/portaudio.h     = header file for PortAudio API. Specifies API.	
    src/common/             = platform independant code, host independant 
                              code for all implementations.
    src/os                  = os specific (but host api neutral) code
    src/hostapi             = implementations for different host apis

    pablio                  = simple blocking read/write interface
    

Host API Implementations:
    src/hostapi/alsa        = Advanced Linux Sound Architecture (ALSA)
    src/hostapi/asihpi      = AudioScience HPI
    src/hostapi/asio        = ASIO for Windows and Macintosh
    src/hostapi/coreaudio   = Macintosh Core Audio for OS X
    src/hostapi/dsound      = Windows Direct Sound
    src/hostapi/jack        = JACK Audio Connection Kit
    src/hostapi/oss         = Unix Open Sound System (OSS)
    src/hostapi/wasapi      = Windows Vista WASAPI
    src/hostapi/wdmks       = Windows WDM Kernel Streaming
    src/hostapi/wmme        = Windows MME (most widely supported)


Test Programs:
    tests/pa_fuzz.c         = guitar fuzz box
    tests/pa_devs.c         = print a list of available devices
    tests/pa_minlat.c       = determine minimum latency for your machine
    tests/paqa_devs.c       = self test that opens all devices
    tests/paqa_errs.c       = test error detection and reporting
    tests/patest_clip.c     = hear a sine wave clipped and unclipped
    tests/patest_dither.c   = hear effects of dithering (extremely subtle)
    tests/patest_pink.c     = fun with pink noise
    tests/patest_record.c   = record and playback some audio
    tests/patest_maxsines.c = how many sine waves can we play? Tests Pa_GetCPULoad().
    tests/patest_sine.c     = output a sine wave in a simple PA app
    tests/patest_sync.c     = test syncronization of audio and video
    tests/patest_wire.c     = pass input to output, wire simulator
