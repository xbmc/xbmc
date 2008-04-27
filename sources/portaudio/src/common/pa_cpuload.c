/*
 * $Id: pa_cpuload.c 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library CPU Load measurement functions
 * Portable CPU load measurement facility.
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 2002 Ross Bencina
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

 @brief Functions to assist in measuring the CPU utilization of a callback
 stream. Used to implement the Pa_GetStreamCpuLoad() function.

 @todo Dynamically calculate the coefficients used to smooth the CPU Load
 Measurements over time to provide a uniform characterisation of CPU Load
 independent of rate at which PaUtil_BeginCpuLoadMeasurement /
 PaUtil_EndCpuLoadMeasurement are called.
*/


#include "pa_cpuload.h"

#include <assert.h>

#include "pa_util.h"   /* for PaUtil_GetTime() */


void PaUtil_InitializeCpuLoadMeasurer( PaUtilCpuLoadMeasurer* measurer, double sampleRate )
{
    assert( sampleRate > 0 );

    measurer->samplingPeriod = 1. / sampleRate;
    measurer->averageLoad = 0.;
}

void PaUtil_ResetCpuLoadMeasurer( PaUtilCpuLoadMeasurer* measurer )
{
    measurer->averageLoad = 0.;
}

void PaUtil_BeginCpuLoadMeasurement( PaUtilCpuLoadMeasurer* measurer )
{
    measurer->measurementStartTime = PaUtil_GetTime();
}


void PaUtil_EndCpuLoadMeasurement( PaUtilCpuLoadMeasurer* measurer, unsigned long framesProcessed )
{
    double measurementEndTime, secondsFor100Percent, measuredLoad;

    if( framesProcessed > 0 ){
        measurementEndTime = PaUtil_GetTime();

        assert( framesProcessed > 0 );
        secondsFor100Percent = framesProcessed * measurer->samplingPeriod;

        measuredLoad = (measurementEndTime - measurer->measurementStartTime) / secondsFor100Percent;

        /* Low pass filter the calculated CPU load to reduce jitter using a simple IIR low pass filter. */
        /** FIXME @todo these coefficients shouldn't be hardwired */
#define LOWPASS_COEFFICIENT_0   (0.9)
#define LOWPASS_COEFFICIENT_1   (0.99999 - LOWPASS_COEFFICIENT_0)

        measurer->averageLoad = (LOWPASS_COEFFICIENT_0 * measurer->averageLoad) +
                               (LOWPASS_COEFFICIENT_1 * measuredLoad);
    }
}


double PaUtil_GetCpuLoad( PaUtilCpuLoadMeasurer* measurer )
{
    return measurer->averageLoad;
}
