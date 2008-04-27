#ifndef PA_CONVERTERS_H
#define PA_CONVERTERS_H
/*
 * $Id: pa_converters.h 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library sample conversion mechanism
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

 @brief Conversion functions used to convert buffers of samples from one
 format to another.
*/


#include "portaudio.h"  /* for PaSampleFormat */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


struct PaUtilTriangularDitherGenerator;


/** Choose an available sample format which is most appropriate for
 representing the requested format. If the requested format is not available
 higher quality formats are considered before lower quality formates.
 @param availableFormats A variable containing the logical OR of all available
 formats.
 @param format The desired format.
 @return The most appropriate available format for representing the requested
 format.
*/
PaSampleFormat PaUtil_SelectClosestAvailableFormat(
        PaSampleFormat availableFormats, PaSampleFormat format );


/* high level conversions functions for use by implementations */


/** The generic sample converter prototype. Sample converters convert count
    samples from sourceBuffer to destinationBuffer. The actual type of the data
    pointed to by these parameters varys for different converter functions.
    @param destinationBuffer A pointer to the first sample of the destination.
    @param destinationStride An offset between successive destination samples
    expressed in samples (not bytes.) It may be negative.
    @param sourceBuffer A pointer to the first sample of the source.
    @param sourceStride An offset between successive source samples
    expressed in samples (not bytes.) It may be negative.
    @param count The number of samples to convert.
    @param ditherState State information used to calculate dither. Converters
    that do not perform dithering will ignore this parameter, in which case
    NULL or invalid dither state may be passed.
*/
typedef void PaUtilConverter(
    void *destinationBuffer, signed int destinationStride,
    void *sourceBuffer, signed int sourceStride,
    unsigned int count, struct PaUtilTriangularDitherGenerator *ditherGenerator );


/** Find a sample converter function for the given source and destinations
    formats and flags (clip and dither.)
    @return
    A pointer to a PaUtilConverter which will perform the requested
    conversion, or NULL if the given format conversion is not supported.
    For conversions where clipping or dithering is not necessary, the
    clip and dither flags are ignored and a non-clipping or dithering
    version is returned.
    If the source and destination formats are the same, a function which
    copies data of the appropriate size will be returned.
*/
PaUtilConverter* PaUtil_SelectConverter( PaSampleFormat sourceFormat,
        PaSampleFormat destinationFormat, PaStreamFlags flags );


/** The generic buffer zeroer prototype. Buffer zeroers copy count zeros to
    destinationBuffer. The actual type of the data pointed to varys for
    different zeroer functions.
    @param destinationBuffer A pointer to the first sample of the destination.
    @param destinationStride An offset between successive destination samples
    expressed in samples (not bytes.) It may be negative.
    @param count The number of samples to zero.
*/
typedef void PaUtilZeroer(
    void *destinationBuffer, signed int destinationStride, unsigned int count );

    
/** Find a buffer zeroer function for the given destination format.
    @return
    A pointer to a PaUtilZeroer which will perform the requested
    zeroing.
*/
PaUtilZeroer* PaUtil_SelectZeroer( PaSampleFormat destinationFormat );

/*----------------------------------------------------------------------------*/
/* low level functions and data structures which may be used for
    substituting conversion functions */


/** The type used to store all sample conversion functions.
    @see paConverters;
*/
typedef struct{
    PaUtilConverter *Float32_To_Int32;
    PaUtilConverter *Float32_To_Int32_Dither;
    PaUtilConverter *Float32_To_Int32_Clip;
    PaUtilConverter *Float32_To_Int32_DitherClip;

    PaUtilConverter *Float32_To_Int24;
    PaUtilConverter *Float32_To_Int24_Dither;
    PaUtilConverter *Float32_To_Int24_Clip;
    PaUtilConverter *Float32_To_Int24_DitherClip;
    
    PaUtilConverter *Float32_To_Int16;
    PaUtilConverter *Float32_To_Int16_Dither;
    PaUtilConverter *Float32_To_Int16_Clip;
    PaUtilConverter *Float32_To_Int16_DitherClip;

    PaUtilConverter *Float32_To_Int8;
    PaUtilConverter *Float32_To_Int8_Dither;
    PaUtilConverter *Float32_To_Int8_Clip;
    PaUtilConverter *Float32_To_Int8_DitherClip;

    PaUtilConverter *Float32_To_UInt8;
    PaUtilConverter *Float32_To_UInt8_Dither;
    PaUtilConverter *Float32_To_UInt8_Clip;
    PaUtilConverter *Float32_To_UInt8_DitherClip;

    PaUtilConverter *Int32_To_Float32;
    PaUtilConverter *Int32_To_Int24;
    PaUtilConverter *Int32_To_Int24_Dither;
    PaUtilConverter *Int32_To_Int16;
    PaUtilConverter *Int32_To_Int16_Dither;
    PaUtilConverter *Int32_To_Int8;
    PaUtilConverter *Int32_To_Int8_Dither;
    PaUtilConverter *Int32_To_UInt8;
    PaUtilConverter *Int32_To_UInt8_Dither;

    PaUtilConverter *Int24_To_Float32;
    PaUtilConverter *Int24_To_Int32;
    PaUtilConverter *Int24_To_Int16;
    PaUtilConverter *Int24_To_Int16_Dither;
    PaUtilConverter *Int24_To_Int8;
    PaUtilConverter *Int24_To_Int8_Dither;
    PaUtilConverter *Int24_To_UInt8;
    PaUtilConverter *Int24_To_UInt8_Dither;

    PaUtilConverter *Int16_To_Float32;
    PaUtilConverter *Int16_To_Int32;
    PaUtilConverter *Int16_To_Int24;
    PaUtilConverter *Int16_To_Int8;
    PaUtilConverter *Int16_To_Int8_Dither;
    PaUtilConverter *Int16_To_UInt8;
    PaUtilConverter *Int16_To_UInt8_Dither;

    PaUtilConverter *Int8_To_Float32;
    PaUtilConverter *Int8_To_Int32;
    PaUtilConverter *Int8_To_Int24;
    PaUtilConverter *Int8_To_Int16;
    PaUtilConverter *Int8_To_UInt8;
    
    PaUtilConverter *UInt8_To_Float32;
    PaUtilConverter *UInt8_To_Int32;
    PaUtilConverter *UInt8_To_Int24;
    PaUtilConverter *UInt8_To_Int16;
    PaUtilConverter *UInt8_To_Int8;

    PaUtilConverter *Copy_8_To_8;       /* copy without any conversion */
    PaUtilConverter *Copy_16_To_16;     /* copy without any conversion */
    PaUtilConverter *Copy_24_To_24;     /* copy without any conversion */
    PaUtilConverter *Copy_32_To_32;     /* copy without any conversion */
} PaUtilConverterTable;


/** A table of pointers to all required converter functions.
    PaUtil_SelectConverter() uses this table to lookup the appropriate
    conversion functions. The fields of this structure are initialized
    with default conversion functions. Fields may be NULL, indicating that
    no conversion function is available. User code may substitue optimised
    conversion functions by assigning different function pointers to
    these fields.

    @note
    If the PA_NO_STANDARD_CONVERTERS preprocessor variable is defined,
    PortAudio's standard converters will not be compiled, and all fields
    of this structure will be initialized to NULL. In such cases, users
    should supply their own conversion functions if the require PortAudio
    to open a stream that requires sample conversion.

    @see PaUtilConverterTable, PaUtilConverter, PaUtil_SelectConverter
*/
extern PaUtilConverterTable paConverters;


/** The type used to store all buffer zeroing functions.
    @see paZeroers;
*/
typedef struct{
    PaUtilZeroer *ZeroU8; /* unsigned 8 bit, zero == 128 */
    PaUtilZeroer *Zero8;
    PaUtilZeroer *Zero16;
    PaUtilZeroer *Zero24;
    PaUtilZeroer *Zero32;
} PaUtilZeroerTable;


/** A table of pointers to all required zeroer functions.
    PaUtil_SelectZeroer() uses this table to lookup the appropriate
    conversion functions. The fields of this structure are initialized
    with default conversion functions. User code may substitue optimised
    conversion functions by assigning different function pointers to
    these fields.

    @note
    If the PA_NO_STANDARD_ZEROERS preprocessor variable is defined,
    PortAudio's standard zeroers will not be compiled, and all fields
    of this structure will be initialized to NULL. In such cases, users
    should supply their own zeroing functions for the sample sizes which
    they intend to use.

    @see PaUtilZeroerTable, PaUtilZeroer, PaUtil_SelectZeroer
*/
extern PaUtilZeroerTable paZeroers;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_CONVERTERS_H */
