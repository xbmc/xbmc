#ifndef MATHTOOLS_H
#define MATHTOOLS_H


#define _double2fixmagic (68719476736.0*1.5)
/* 2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor */
#define _shiftamt 16
/* 16.16 fixed point representation */

#if BigEndian_
#define iexp_				0
#define iman_				1
#else
#define iexp_				1
#define iman_				0
#endif /* BigEndian_ */

/* TODO: this optimization is very efficient: put it again when all works
#ifdef HAVE_MMX
#define F2I(dbl,i) {double d = dbl + _double2fixmagic; i = ((int*)&d)[iman_] >> _shiftamt;}
#else*/
#define F2I(dbl,i) i=(int)dbl;
/*#endif*/

#if 0
#define SINCOS(f,s,c) \
  __asm__ __volatile__ ("fsincos" : "=t" (c), "=u" (s) : "0" (f))
#else
#define SINCOS(f,s,c) {s=sin(f);c=cos(f);}
#endif

extern float sin256[256];
extern float cos256[256];

#endif

