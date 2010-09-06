#ifndef _SIMD_COMMON_H_
#define _SIMD_COMMON_H_

#if !defined(__GNUC__)// && 0
  #define __SSE2__
#endif

#ifdef __GNUC__
  #ifndef __forceinline
    #define __forceinline __attribute__((__always_inline__)) inline
  #endif
#endif

#ifdef __GNUC__
 #define __inline __forceinline  // GCC needs to force inlining of intrinsics functions
#endif

#include <mmintrin.h>
#include <xmmintrin.h>
#ifdef __SSE2__
  #include <emmintrin.h>
#endif

#ifdef __GNUC__
 #undef __inline
#endif

#ifdef __GNUC__
  #define __align8(t,v) t v __attribute__ ((aligned (8)))
  #define __align16(t,v) t v __attribute__ ((aligned (16)))
#else
  #define __align8(t,v) __declspec(align(8)) t v
  #define __align16(t,v) __declspec(align(16)) t v
#endif

#endif
