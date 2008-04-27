/*
 * Interface for dynamically loading directsound and providing a dummy
 * implementation if it isn't present.
 *
 * Author: Ross Bencina (some portions Phil Burk & Robert Marsanyi)
 *
 * For PortAudio Portable Real-Time Audio Library
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2006 Phil Burk, Robert Marsanyi and Ross Bencina
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

/**
 @file
 @ingroup hostaip_src
*/

#ifndef INCLUDED_PA_DSOUND_DYNLINK_H
#define INCLUDED_PA_DSOUND_DYNLINK_H

/* on Borland compilers, WIN32 doesn't seem to be defined by default, which
    breaks dsound.h. Adding the define here fixes the problem. - rossb. */
#ifdef __BORLANDC__
#if !defined(WIN32)
#define WIN32
#endif
#endif

/*
  We are only using DX3 in here, no need to polute the namespace - davidv
*/
#define DIRECTSOUND_VERSION 0x0300
#include <dsound.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef struct
{
    HINSTANCE hInstance_;
    
    HRESULT (WINAPI *DllGetClassObject)(REFCLSID , REFIID , LPVOID *);

    HRESULT (WINAPI *DirectSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
    HRESULT (WINAPI *DirectSoundEnumerateW)(LPDSENUMCALLBACKW, LPVOID);
    HRESULT (WINAPI *DirectSoundEnumerateA)(LPDSENUMCALLBACKA, LPVOID);

    HRESULT (WINAPI *DirectSoundCaptureCreate)(LPGUID, LPDIRECTSOUNDCAPTURE *, LPUNKNOWN);
    HRESULT (WINAPI *DirectSoundCaptureEnumerateW)(LPDSENUMCALLBACKW, LPVOID);
    HRESULT (WINAPI *DirectSoundCaptureEnumerateA)(LPDSENUMCALLBACKA, LPVOID);
}PaWinDsDSoundEntryPoints;

extern PaWinDsDSoundEntryPoints paWinDsDSoundEntryPoints;

void PaWinDs_InitializeDSoundEntryPoints(void);
void PaWinDs_TerminateDSoundEntryPoints(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INCLUDED_PA_DSOUND_DYNLINK_H */
