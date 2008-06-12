/*
 * $Id: pa_win_hostapis.c 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library Windows initialization table
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Ross Bencina, Phil Burk
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
 @ingroup win_src

    Win32 host API initialization function table.

    @todo Consider using PA_USE_WMME etc instead of PA_NO_WMME. This is what
    the Unix version does, we should consider being consistent.
*/


#include "pa_hostapi.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

PaError PaSkeleton_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaWinMme_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaWinDs_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaAsio_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaWinWdm_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaWinWasapi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );

#ifdef __cplusplus
}
#endif /* __cplusplus */


PaUtilHostApiInitializer *paHostApiInitializers[] =
    {

#ifndef PA_NO_WMME
        PaWinMme_Initialize,
#endif

#ifndef PA_NO_DS
        PaWinDs_Initialize,
#endif

#ifndef PA_NO_ASIO
        PaAsio_Initialize,
#endif

/*
#ifndef PA_NO_WASAPI
		PaWinWasapi_Initialize,
#endif

#ifndef PA_NO_WDMKS
       PaWinWdm_Initialize,
#endif
*/

        PaSkeleton_Initialize, /* just for testing */

        0   /* NULL terminated array */
    };


int paDefaultHostApiIndex = 0;

