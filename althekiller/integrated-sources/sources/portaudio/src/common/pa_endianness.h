#ifndef PA_ENDIANNESS_H
#define PA_ENDIANNESS_H
/*
 * $Id: pa_endianness.h 1216 2007-06-10 09:26:00Z aknudsen $
 * Portable Audio I/O Library current platform endianness macros
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

 @brief Configure endianness symbols for the target processor.

 Arrange for either the PA_LITTLE_ENDIAN or PA_BIG_ENDIAN preprocessor symbols
 to be defined. The one that is defined reflects the endianness of the target
 platform and may be used to implement conditional compilation of byte-order
 dependent code.

 If either PA_LITTLE_ENDIAN or PA_BIG_ENDIAN is defined already, then no attempt
 is made to override that setting. This may be useful if you have a better way
 of determining the platform's endianness. The autoconf mechanism uses this for
 example.

 A PA_VALIDATE_ENDIANNESS macro is provided to compare the compile time
 and runtime endiannes and raise an assertion if they don't match.
*/


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* If this is an apple, we need to do detect endianness this way */
#if defined(__APPLE__)
    /* we need to do some endian detection that is sensitive to harware arch */
    #if defined(__LITTLE_ENDIAN__)
       #if !defined( PA_LITTLE_ENDIAN )
          #define PA_LITTLE_ENDIAN
       #endif
       #if defined( PA_BIG_ENDIAN )
          #undef PA_BIG_ENDIAN
       #endif
    #else
       #if !defined( PA_BIG_ENDIAN )
          #define PA_BIG_ENDIAN
       #endif
       #if defined( PA_LITTLE_ENDIAN )
          #undef PA_LITTLE_ENDIAN
       #endif
    #endif
#else
    /* this is not an apple, so first check the existing defines, and, failing that,
       detect well-known architechtures. */

    #if defined(PA_LITTLE_ENDIAN) || defined(PA_BIG_ENDIAN)
        /* endianness define has been set externally, such as by autoconf */

        #if defined(PA_LITTLE_ENDIAN) && defined(PA_BIG_ENDIAN)
        #error both PA_LITTLE_ENDIAN and PA_BIG_ENDIAN have been defined externally to pa_endianness.h - only one endianness at a time please
        #endif

    #else
        /* endianness define has not been set externally */

        /* set PA_LITTLE_ENDIAN or PA_BIG_ENDIAN by testing well known platform specific defines */

        #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(LITTLE_ENDIAN) || defined(__i386) || defined(_M_IX86) || defined(__x86_64__)
            #define PA_LITTLE_ENDIAN /* win32, assume intel byte order */
        #else
            #define PA_BIG_ENDIAN
        #endif
    #endif

    #if !defined(PA_LITTLE_ENDIAN) && !defined(PA_BIG_ENDIAN)
        /*
         If the following error is raised, you either need to modify the code above
         to automatically determine the endianness from other symbols defined on your
         platform, or define either PA_LITTLE_ENDIAN or PA_BIG_ENDIAN externally.
        */
        #error pa_endianness.h was unable to automatically determine the endianness of the target platform
    #endif

#endif


/* PA_VALIDATE_ENDIANNESS compares the compile time and runtime endianness,
 and raises an assertion if they don't match. <assert.h> must be included in
 the context in which this macro is used.
*/
#if defined(PA_LITTLE_ENDIAN)
    #define PA_VALIDATE_ENDIANNESS \
    { \
        const long nativeOne = 1; \
        assert( "PortAudio: compile time and runtime endianness don't match" && (((char *)&nativeOne)[0]) == 1 ); \
    }
#elif defined(PA_BIG_ENDIAN)
    #define PA_VALIDATE_ENDIANNESS \
    { \
        const long nativeOne = 1; \
        assert( "PortAudio: compile time and runtime endianness don't match" && (((char *)&nativeOne)[0]) == 0 ); \
    }
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_ENDIANNESS_H */
