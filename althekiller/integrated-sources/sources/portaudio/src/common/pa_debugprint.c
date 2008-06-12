/*
 * $Id: pa_log.c $
 * Portable Audio I/O Library Multi-Host API front end
 * Validate function parameters and manage multiple host APIs.
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2006 Ross Bencina, Phil Burk
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
 @ingroup common_src

 @brief Implements log function.

    PaUtil_SetLogPrintFunction can be user called to replace the provided
	DefaultLogPrint function, which writes to stderr.
	One can NOT pass var_args across compiler/dll boundaries as it is not
	"byte code/abi portable". So the technique used here is to allocate a local
	a static array, write in it, then callback the user with a pointer to its
	start.

    @todo Consider allocating strdump using dynamic allocation.
    @todo Consider reentrancy and possibly corrupted strdump buffer.
*/


#include <stdio.h>
#include <stdarg.h>

#include "pa_debugprint.h"



static PaUtilLogCallback userCB=0;


void PaUtil_SetDebugPrintFunction(PaUtilLogCallback cb)
{
    userCB = cb;
}

/*
 If your platform doesn’t have vsnprintf, you are stuck with a
 VERY dangerous alternative, vsprintf (with no n)
 */

#if (_MSC_VER) && (_MSC_VER <= 1400)
#define VSNPRINTF  _vsnprintf
#else
#define VSNPRINTF  vsnprintf
#endif

#define SIZEDUMP 1024

static char strdump[SIZEDUMP];

void PaUtil_DebugPrint( const char *format, ... )
{

    if (userCB)
    {
        va_list ap;
        va_start( ap, format );
        VSNPRINTF( strdump, SIZEDUMP, format, ap );
        userCB(strdump);
        va_end( ap ); 
    }
    else
    {
        va_list ap;
        va_start( ap, format );
        vfprintf( stderr, format, ap );
        va_end( ap );
        fflush( stderr );
    }

}
