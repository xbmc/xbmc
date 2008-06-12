#ifndef PA_JACK_H
#define PA_JACK_H

/*
 * $Id:
 * PortAudio Portable Real-Time Audio Library
 * JACK-specific extensions
 *
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
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

/** @file
 * JACK-specific PortAudio API extension header file.
 */
#include "portaudio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Set the JACK client name.
 *
 * During Pa_Initialize, When PA JACK connects as a client of the JACK server, it requests a certain
 * name, which is for instance prepended to port names. By default this name is "PortAudio". The
 * JACK server may append a suffix to the client name, in order to avoid clashes among clients that
 * try to connect with the same name (e.g., different PA JACK clients).
 *
 * This function must be called before Pa_Initialize, otherwise it won't have any effect. Note that
 * the string is not copied, but instead referenced directly, so it must not be freed for as long as
 * PA might need it.
 * @sa PaJack_GetClientName
 */
PaError PaJack_SetClientName( const char* name );

/** Get the JACK client name used by PA JACK.
 *
 * The caller is responsible for freeing the returned pointer.
 */
PaError PaJack_GetClientName(const char** clientName);

#ifdef __cplusplus
}
#endif

#endif
