/*
 * $Id: pa_unix_hostapis.c 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library UNIX initialization table
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
 @ingroup unix_src
*/

#include "pa_hostapi.h"

PaError PaJack_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaAlsa_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
PaError PaOSS_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
/* Added for IRIX, Pieter, oct 2, 2003: */
PaError PaSGI_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
/* Linux AudioScience HPI */
PaError PaAsiHpi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );


PaUtilHostApiInitializer *paHostApiInitializers[] =
    {
#ifdef PA_USE_OSS
        PaOSS_Initialize,
#endif

#ifdef PA_USE_ALSA
        PaAlsa_Initialize,
#endif

#ifdef PA_USE_JACK
        PaJack_Initialize,
#endif
                    /* Added for IRIX, Pieter, oct 2, 2003: */
#ifdef PA_USE_SGI 
        PaSGI_Initialize,
#endif

#ifdef PA_USE_ASIHPI
        PaAsiHpi_Initialize,
#endif
        0   /* NULL terminated array */
    };

int paDefaultHostApiIndex = 0;
