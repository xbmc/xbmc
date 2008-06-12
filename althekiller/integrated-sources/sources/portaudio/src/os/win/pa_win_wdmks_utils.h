#ifndef PA_WIN_WDMKS_UTILS_H
#define PA_WIN_WDMKS_UTILS_H

/*
 * PortAudio Portable Real-Time Audio Library
 * Windows WDM KS utilities
 *
 * Copyright (c) 1999 - 2007 Ross Bencina, Andrew Baldwin
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
 @brief Utilities for working with the Windows WDM KS API
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
    Query for the maximum number of channels supported by any pin of the
    specified device. Returns 0 if the query fails for any reason.

    @param wcharDevicePath A system level PnP interface path, supplied as a WCHAR unicode string.
    Declard as void* to avoid introducing a dependency on wchar_t here.

    @param isInput A flag specifying whether to query for input (non-zero) or output (zero) channels.
*/
int PaWin_WDMKS_QueryFilterMaximumChannelCount( void *wcharDevicePath, int isInput );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_WIN_WDMKS_UTILS_H */