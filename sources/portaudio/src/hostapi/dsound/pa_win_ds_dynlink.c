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

#include "pa_win_ds_dynlink.h"

PaWinDsDSoundEntryPoints paWinDsDSoundEntryPoints = { 0, 0, 0, 0, 0, 0, 0 };


static HRESULT WINAPI DummyDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    (void)rclsid; /* unused parameter */
    (void)riid; /* unused parameter */
    (void)ppv; /* unused parameter */
    return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT WINAPI DummyDirectSoundCreate(LPGUID lpcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
{
    (void)lpcGuidDevice; /* unused parameter */
    (void)ppDS; /* unused parameter */
    (void)pUnkOuter; /* unused parameter */
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDirectSoundEnumerateW(LPDSENUMCALLBACKW lpDSEnumCallback, LPVOID lpContext)
{
    (void)lpDSEnumCallback; /* unused parameter */
    (void)lpContext; /* unused parameter */
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDirectSoundEnumerateA(LPDSENUMCALLBACKA lpDSEnumCallback, LPVOID lpContext)
{
    (void)lpDSEnumCallback; /* unused parameter */
    (void)lpContext; /* unused parameter */
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDirectSoundCaptureCreate(LPGUID lpcGUID, LPDIRECTSOUNDCAPTURE *lplpDSC, LPUNKNOWN pUnkOuter)
{
    (void)lpcGUID; /* unused parameter */
    (void)lplpDSC; /* unused parameter */
    (void)pUnkOuter; /* unused parameter */
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW lpDSCEnumCallback, LPVOID lpContext)
{
    (void)lpDSCEnumCallback; /* unused parameter */
    (void)lpContext; /* unused parameter */
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA lpDSCEnumCallback, LPVOID lpContext)
{
    (void)lpDSCEnumCallback; /* unused parameter */
    (void)lpContext; /* unused parameter */
    return E_NOTIMPL;
}


void PaWinDs_InitializeDSoundEntryPoints(void)
{
    paWinDsDSoundEntryPoints.hInstance_ = LoadLibrary("dsound.dll");
    if( paWinDsDSoundEntryPoints.hInstance_ != NULL )
    {
        paWinDsDSoundEntryPoints.DllGetClassObject =
                (HRESULT (WINAPI *)(REFCLSID, REFIID , LPVOID *))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DllGetClassObject" );
        if( paWinDsDSoundEntryPoints.DllGetClassObject == NULL )
            paWinDsDSoundEntryPoints.DllGetClassObject = DummyDllGetClassObject;

        paWinDsDSoundEntryPoints.DirectSoundCreate =
                (HRESULT (WINAPI *)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DirectSoundCreate" );
        if( paWinDsDSoundEntryPoints.DirectSoundCreate == NULL )
            paWinDsDSoundEntryPoints.DirectSoundCreate = DummyDirectSoundCreate;

        paWinDsDSoundEntryPoints.DirectSoundEnumerateW =
                (HRESULT (WINAPI *)(LPDSENUMCALLBACKW, LPVOID))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DirectSoundEnumerateW" );
        if( paWinDsDSoundEntryPoints.DirectSoundEnumerateW == NULL )
            paWinDsDSoundEntryPoints.DirectSoundEnumerateW = DummyDirectSoundEnumerateW;

        paWinDsDSoundEntryPoints.DirectSoundEnumerateA =
                (HRESULT (WINAPI *)(LPDSENUMCALLBACKA, LPVOID))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DirectSoundEnumerateA" );
        if( paWinDsDSoundEntryPoints.DirectSoundEnumerateA == NULL )
            paWinDsDSoundEntryPoints.DirectSoundEnumerateA = DummyDirectSoundEnumerateA;

        paWinDsDSoundEntryPoints.DirectSoundCaptureCreate =
                (HRESULT (WINAPI *)(LPGUID, LPDIRECTSOUNDCAPTURE *, LPUNKNOWN))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DirectSoundCaptureCreate" );
        if( paWinDsDSoundEntryPoints.DirectSoundCaptureCreate == NULL )
            paWinDsDSoundEntryPoints.DirectSoundCaptureCreate = DummyDirectSoundCaptureCreate;

        paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateW =
                (HRESULT (WINAPI *)(LPDSENUMCALLBACKW, LPVOID))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DirectSoundCaptureEnumerateW" );
        if( paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateW == NULL )
            paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateW = DummyDirectSoundCaptureEnumerateW;

        paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateA =
                (HRESULT (WINAPI *)(LPDSENUMCALLBACKA, LPVOID))
                GetProcAddress( paWinDsDSoundEntryPoints.hInstance_, "DirectSoundCaptureEnumerateA" );
        if( paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateA == NULL )
            paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateA = DummyDirectSoundCaptureEnumerateA;
    }
    else
    {
        /* initialize with dummy entry points to make live easy when ds isn't present */
        paWinDsDSoundEntryPoints.DirectSoundCreate = DummyDirectSoundCreate;
        paWinDsDSoundEntryPoints.DirectSoundEnumerateW = DummyDirectSoundEnumerateW;
        paWinDsDSoundEntryPoints.DirectSoundEnumerateA = DummyDirectSoundEnumerateA;
        paWinDsDSoundEntryPoints.DirectSoundCaptureCreate = DummyDirectSoundCaptureCreate;
        paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateW = DummyDirectSoundCaptureEnumerateW;
        paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateA = DummyDirectSoundCaptureEnumerateA;
    }
}


void PaWinDs_TerminateDSoundEntryPoints(void)
{
    if( paWinDsDSoundEntryPoints.hInstance_ != NULL )
    {
        /* ensure that we crash reliably if the entry points arent initialised */
        paWinDsDSoundEntryPoints.DirectSoundCreate = 0;
        paWinDsDSoundEntryPoints.DirectSoundEnumerateW = 0;
        paWinDsDSoundEntryPoints.DirectSoundEnumerateA = 0;
        paWinDsDSoundEntryPoints.DirectSoundCaptureCreate = 0;
        paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateW = 0;
        paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateA = 0;

        FreeLibrary( paWinDsDSoundEntryPoints.hInstance_ );
        paWinDsDSoundEntryPoints.hInstance_ = NULL;
    }
}