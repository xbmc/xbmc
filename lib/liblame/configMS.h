/* The number of bytes in a double.  */
#define SIZEOF_DOUBLE 8

/* The number of bytes in a float.  */
#define SIZEOF_FLOAT 4

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a long double.  */
#define SIZEOF_LONG_DOUBLE 12

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a unsigned int.  */
#define SIZEOF_UNSIGNED_INT 4

/* The number of bytes in a unsigned long.  */
#define SIZEOF_UNSIGNED_LONG 4

/* The number of bytes in a unsigned short.  */
#define SIZEOF_UNSIGNED_SHORT 2

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H

/* Name of package */
#define PACKAGE "lame"

/* Define if compiler has function prototypes */
#define PROTOTYPES 1

/* enable VBR bitrate histogram */
#define BRHIST 1

/* IEEE754 compatible machine */
#define TAKEHIRO_IEEE754_HACK 1

/* faster log implementation with less but enough precission */
#define USE_FAST_LOG 1

#define HAVE_STRCHR
#define HAVE_MEMCPY

#if defined(_MSC_VER) || defined(__BORLANDC__)
#pragma warning( disable : 4305 )
	typedef __int8  int8_t;
	typedef __int16 int16_t;
	typedef __int32 int32_t;
	typedef __int64 int64_t;

	typedef unsigned __int8  uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;

	typedef float  float32_t;
	typedef double float64_t;
#elif defined (__GNUC__)
#define __int8_t_defined
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long

#define int8_t signed char
#define int16_t signed short
#define int32_t signed int
#define int64_t signed long long
#endif

typedef long double ieee854_float80_t;
typedef double      ieee754_float64_t;
typedef float       ieee754_float32_t;

#ifdef HAVE_MPGLIB
# define DECODE_ON_THE_FLY 1
#endif

#ifdef LAME_ACM
/* memory hacking for driver purposes */
#define calloc(x,y) acm_Calloc(x,y)
#define free(x)     acm_Free(x)
#define malloc(x)   acm_Malloc(x)

#include <stddef.h>
void *acm_Calloc( size_t num, size_t size );
void *acm_Malloc( size_t size );
void acm_Free( void * mem);
#endif /* LAME_ACM */

#define LAME_LIBRARY_BUILD


#ifdef HAVE_NASM
    #if (defined(__ICL) && (__ICL >= 450))
        #define HAVE_XMMINTRIN_H
    #elif defined(_MSC_VER)
        #include <malloc.h>
        #ifdef _mm_malloc
            #define HAVE_XMMINTRIN_H
        #endif
    #endif
#endif

#ifdef _M_X64
        #define HAVE_XMMINTRIN_H
#endif
