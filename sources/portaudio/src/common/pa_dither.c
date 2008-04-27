/*
 * $Id: pa_dither.c 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library triangular dither generator
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Phil Burk, Ross Bencina
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

 @brief Functions for generating dither noise
*/


#include "pa_dither.h"
#include "pa_types.h"

#define PA_DITHER_BITS_   (15)


void PaUtil_InitializeTriangularDitherState( PaUtilTriangularDitherGenerator *state )
{
    state->previous = 0;
    state->randSeed1 = 22222;
    state->randSeed2 = 5555555;
}


signed long PaUtil_Generate16BitTriangularDither( PaUtilTriangularDitherGenerator *state )
{
    signed long current, highPass;

    /* Generate two random numbers. */
    state->randSeed1 = (state->randSeed1 * 196314165) + 907633515;
    state->randSeed2 = (state->randSeed2 * 196314165) + 907633515;

    /* Generate triangular distribution about 0.
     * Shift before adding to prevent overflow which would skew the distribution.
     * Also shift an extra bit for the high pass filter. 
     */
#define DITHER_SHIFT_  ((SIZEOF_LONG*8 - PA_DITHER_BITS_) + 1)
    current = (((signed long)state->randSeed1)>>DITHER_SHIFT_) +
              (((signed long)state->randSeed2)>>DITHER_SHIFT_);

    /* High pass filter to reduce audibility. */
    highPass = current - state->previous;
    state->previous = current;
    return highPass;
}


/* Multiply by PA_FLOAT_DITHER_SCALE_ to get a float between -2.0 and +1.99999 */
#define PA_FLOAT_DITHER_SCALE_  (1.0f / ((1<<PA_DITHER_BITS_)-1))
static const float const_float_dither_scale_ = PA_FLOAT_DITHER_SCALE_;

float PaUtil_GenerateFloatTriangularDither( PaUtilTriangularDitherGenerator *state )
{
    signed long current, highPass;

    /* Generate two random numbers. */
    state->randSeed1 = (state->randSeed1 * 196314165) + 907633515;
    state->randSeed2 = (state->randSeed2 * 196314165) + 907633515;

    /* Generate triangular distribution about 0.
     * Shift before adding to prevent overflow which would skew the distribution.
     * Also shift an extra bit for the high pass filter. 
     */
#define DITHER_SHIFT_  ((SIZEOF_LONG*8 - PA_DITHER_BITS_) + 1)
    current = (((signed long)state->randSeed1)>>DITHER_SHIFT_) +
              (((signed long)state->randSeed2)>>DITHER_SHIFT_);

    /* High pass filter to reduce audibility. */
    highPass = current - state->previous;
    state->previous = current;
    return ((float)highPass) * const_float_dither_scale_;
}


/*
The following alternate dither algorithms (from musicdsp.org) could be
considered
*/

/*Noise shaped dither  (March 2000)
-------------------

This is a simple implementation of highpass triangular-PDF dither with
2nd-order noise shaping, for use when truncating floating point audio
data to fixed point.

The noise shaping lowers the noise floor by 11dB below 5kHz (@ 44100Hz
sample rate) compared to triangular-PDF dither. The code below assumes
input data is in the range +1 to -1 and doesn't check for overloads!

To save time when generating dither for multiple channels you can do
things like this:  r3=(r1 & 0x7F)<<8; instead of calling rand() again.



  int   r1, r2;                //rectangular-PDF random numbers
  float s1, s2;                //error feedback buffers
  float s = 0.5f;              //set to 0.0f for no noise shaping
  float w = pow(2.0,bits-1);   //word length (usually bits=16)
  float wi= 1.0f/w;            
  float d = wi / RAND_MAX;     //dither amplitude (2 lsb)
  float o = wi * 0.5f;         //remove dc offset
  float in, tmp;
  int   out;


//for each sample...

  r2=r1;                               //can make HP-TRI dither by
  r1=rand();                           //subtracting previous rand()
    
  in += s * (s1 + s1 - s2);            //error feedback
  tmp = in + o + d * (float)(r1 - r2); //dc offset and dither 
  
  out = (int)(w * tmp);                //truncate downwards
  if(tmp<0.0f) out--;                  //this is faster than floor()

  s2 = s1;                            
  s1 = in - wi * (float)out;           //error



-- 
paul.kellett@maxim.abel.co.uk
http://www.maxim.abel.co.uk
*/


/*
16-to-8-bit first-order dither

Type : First order error feedforward dithering code
References : Posted by Jon Watte

Notes : 
This is about as simple a dithering algorithm as you can implement, but it's
likely to sound better than just truncating to N bits.

Note that you might not want to carry forward the full difference for infinity.
It's probably likely that the worst performance hit comes from the saturation
conditionals, which can be avoided with appropriate instructions on many DSPs
and integer SIMD type instructions, or CMOV.

Last, if sound quality is paramount (such as when going from > 16 bits to 16
bits) you probably want to use a higher-order dither function found elsewhere
on this site. 


Code : 
// This code will down-convert and dither a 16-bit signed short 
// mono signal into an 8-bit unsigned char signal, using a first 
// order forward-feeding error term dither. 

#define uchar unsigned char 

void dither_one_channel_16_to_8( short * input, uchar * output, int count, int * memory ) 
{ 
  int m = *memory; 
  while( count-- > 0 ) { 
    int i = *input++; 
    i += m; 
    int j = i + 32768 - 128; 
    uchar o; 
    if( j < 0 ) { 
      o = 0; 
    } 
    else if( j > 65535 ) { 
      o = 255; 
    } 
    else { 
      o = (uchar)((j>>8)&0xff); 
    } 
    m = ((j-32768+128)-i); 
    *output++ = o; 
  } 
  *memory = m; 
} 
*/
