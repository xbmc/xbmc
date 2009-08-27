/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * resample.c - Resampling helpers.                   / / \  \
 *                                                   | <  /   \_
 * By Bob and entheh.                                |  \/ /\   /
 *                                                    \_  /  > /
 * In order to find a good trade-off between            | \ / /
 * speed and accuracy in this code, some tests          |  ' /
 * were carried out regarding the behaviour of           \__/
 * long long ints with gcc. The following code
 * was tested:
 *
 * int a, b, c;
 * c = ((long long)a * b) >> 16;
 *
 * DJGPP GCC Version 3.0.3 generated the following assembly language code for
 * the multiplication and scaling, leaving the 32-bit result in EAX.
 *
 * movl  -8(%ebp), %eax    ; read one int into EAX
 * imull -4(%ebp)          ; multiply by the other; result goes in EDX:EAX
 * shrdl $16, %edx, %eax   ; shift EAX right 16, shifting bits in from EDX
 *
 * Note that a 32*32->64 multiplication is performed, allowing for high
 * accuracy. On the Pentium 2 and above, shrdl takes two cycles (generally),
 * so it is a minor concern when four multiplications are being performed
 * (the cubic resampler). On the Pentium MMX and earlier, it takes four or
 * more cycles, so this method is unsuitable for use in the low-quality
 * resamplers.
 *
 * Since "long long" is a gcc-specific extension, we use LONG_LONG instead,
 * defined in dumb.h. We may investigate later what code MSVC generates, but
 * if it seems too slow then we suggest you use a good compiler.
 *
 * FIXME: these comments are somewhat out of date now.
 */

#include <math.h>
#include "dumb.h"



/* Compile with -DHEAVYDEBUG if you want to make sure the pick-up function is
 * called when it should be. There will be a considerable performance hit,
 * since at least one condition has to be tested for every sample generated.
 */
#ifdef HEAVYDEBUG
#define HEAVYASSERT(cond) ASSERT(cond)
#else
#define HEAVYASSERT(cond)
#endif



/* A global variable for controlling resampling quality wherever a local
 * specification doesn't override it. The following values are valid:
 *
 *  0 - DUMB_RQ_ALIASING - fastest
 *  1 - DUMB_RQ_LINEAR
 *  2 - DUMB_RQ_CUBIC    - nicest
 *
 * Values outside the range 0-2 will behave the same as the nearest
 * value within the range.
 */
int dumb_resampling_quality = DUMB_RQ_CUBIC;



//#define MULSC(a, b) ((int)((LONG_LONG)(a) * (b) >> 16))
//#define MULSC(a, b) ((a) * ((b) >> 2) >> 14)
#define MULSC(a, b) ((int)((LONG_LONG)((a) << 4) * ((b) << 12) >> 32))
#define MULSC16(a, b) ((int)((LONG_LONG)((a) << 12) * ((b) << 12) >> 32))



/* Executes the content 'iterator' times.
 * Clobbers the 'iterator' variable.
 * The loop is unrolled by four.
 */
#define LOOP4(iterator, CONTENT) \
{ \
	if ((iterator) & 2) { \
		CONTENT; \
		CONTENT; \
	} \
	if ((iterator) & 1) { \
		CONTENT; \
	} \
	(iterator) >>= 2; \
	while (iterator) { \
		CONTENT; \
		CONTENT; \
		CONTENT; \
		CONTENT; \
		(iterator)--; \
	} \
}



#define PASTERAW(a, b) a ## b /* This does not expand macros in b ... */
#define PASTE(a, b) PASTERAW(a, b) /* ... but b is expanded during this substitution. */

#define X PASTE(x.x, SRCBITS)



/* Cubic resampler: look-up tables
 *
 * a = 1.5*x1 - 1.5*x2 + 0.5*x3 - 0.5*x0
 * b = 2*x2 + x0 - 2.5*x1 - 0.5*x3
 * c = 0.5*x2 - 0.5*x0
 * d = x1
 *
 * x = a*t*t*t + b*t*t + c*t + d
 *   = (-0.5*x0 + 1.5*x1 - 1.5*x2 + 0.5*x3) * t*t*t +
 *     (   1*x0 - 2.5*x1 + 2  *x2 - 0.5*x3) * t*t +
 *     (-0.5*x0          + 0.5*x2         ) * t +
 *     (            1*x1                  )
 *   = (-0.5*t*t*t + 1  *t*t - 0.5*t    ) * x0 +
 *     ( 1.5*t*t*t - 2.5*t*t         + 1) * x1 +
 *     (-1.5*t*t*t + 2  *t*t + 0.5*t    ) * x2 +
 *     ( 0.5*t*t*t - 0.5*t*t            ) * x3
 *   = A0(t) * x0 + A1(t) * x1 + A2(t) * x2 + A3(t) * x3
 *
 * A0, A1, A2 and A3 stay within the range [-1,1].
 * In the tables, they are scaled with 14 fractional bits.
 *
 * Turns out we don't need to store A2 and A3; they are symmetrical to A1 and A0.
 *
 * TODO: A0 and A3 stay very small indeed. Consider different scale/resolution?
 */

static short cubicA0[1025], cubicA1[1025];

static void init_cubic(void)
{
	unsigned int t; /* 3*1024*1024*1024 is within range if it's unsigned */
	static int done = 0;
	if (done) return;
	done = 1;
	for (t = 0; t < 1025; t++) {
		/* int casts to pacify warnings about negating unsigned values */
		cubicA0[t] = -(int)(  t*t*t >> 17) + (int)(  t*t >> 6) - (int)(t << 3);
		cubicA1[t] =  (int)(3*t*t*t >> 17) - (int)(5*t*t >> 7)                 + (int)(1 << 14);
	}
}



/* Create resamplers for 24-in-32-bit source samples. */

/* #define SUFFIX
 * MSVC warns if we try to paste a null SUFFIX, so instead we define
 * special macros for the function names that don't bother doing the
 * corresponding paste. The more generic definitions are further down.
 */
#define process_pickup PASTE(process_pickup, SUFFIX2)
#define dumb_resample PASTE(PASTE(dumb_resample, SUFFIX2), SUFFIX3)
#define dumb_resample_get_current_sample PASTE(PASTE(dumb_resample_get_current_sample, SUFFIX2), SUFFIX3)

#define SRCTYPE sample_t
#define SRCBITS 24
#define ALIAS(x, vol) MULSC(x, vol)
#define LINEAR(x0, x1) (x0 + MULSC(x1 - x0, subpos))
/*
#define SET_CUBIC_COEFFICIENTS(x0, x1, x2, x3) { \
	a = (3 * (x1 - x2) + (x3 - x0)) >> 1; \
	b = ((x2 << 2) + (x0 << 1) - (5 * x1 + x3)) >> 1; \
	c = (x2 - x0) >> 1; \
}
#define CUBIC(d) MULSC(MULSC(MULSC(MULSC(a, subpos) + b, subpos) + c, subpos) + d, vol)
*/
#define CUBIC(x0, x1, x2, x3) ( \
	MULSC(x0, cubicA0[subpos >> 6] << 2) + \
	MULSC(x1, cubicA1[subpos >> 6] << 2) + \
	MULSC(x2, cubicA1[1 + (subpos >> 6 ^ 1023)] << 2) + \
	MULSC(x3, cubicA0[1 + (subpos >> 6 ^ 1023)] << 2))
#define CUBICVOL(x, vol) MULSC(x, vol)
#include "resample.inc"

/* Undefine the simplified macros. */
#undef dumb_resample_get_current_sample
#undef dumb_resample
#undef process_pickup


/* Now define the proper ones that use SUFFIX. */
#define dumb_reset_resampler PASTE(dumb_reset_resampler, SUFFIX)
#define dumb_start_resampler PASTE(dumb_start_resampler, SUFFIX)
#define process_pickup PASTE(PASTE(process_pickup, SUFFIX), SUFFIX2)
#define dumb_resample PASTE(PASTE(PASTE(dumb_resample, SUFFIX), SUFFIX2), SUFFIX3)
#define dumb_resample_get_current_sample PASTE(PASTE(PASTE(dumb_resample_get_current_sample, SUFFIX), SUFFIX2), SUFFIX3)
#define dumb_end_resampler PASTE(dumb_end_resampler, SUFFIX)

/* Create resamplers for 16-bit source samples. */
#define SUFFIX _16
#define SRCTYPE short
#define SRCBITS 16
#define ALIAS(x, vol) (x * vol >> 8)
#define LINEAR(x0, x1) ((x0 << 8) + MULSC16(x1 - x0, subpos))
/*
#define SET_CUBIC_COEFFICIENTS(x0, x1, x2, x3) { \
	a = (3 * (x1 - x2) + (x3 - x0)) << 7; \
	b = ((x2 << 2) + (x0 << 1) - (5 * x1 + x3)) << 7; \
	c = (x2 - x0) << 7; \
}
#define CUBIC(d) MULSC(MULSC(MULSC(MULSC(a, subpos) + b, subpos) + c, subpos) + (d << 8), vol)
*/
#define CUBIC(x0, x1, x2, x3) ( \
	x0 * cubicA0[subpos >> 6] + \
	x1 * cubicA1[subpos >> 6] + \
	x2 * cubicA1[1 + (subpos >> 6 ^ 1023)] + \
	x3 * cubicA0[1 + (subpos >> 6 ^ 1023)])
#define CUBICVOL(x, vol) (int)((LONG_LONG)(x) * (vol << 10) >> 32)
#include "resample.inc"

/* Create resamplers for 8-bit source samples. */
#define SUFFIX _8
#define SRCTYPE signed char
#define SRCBITS 8
#define ALIAS(x, vol) (x * vol)
#define LINEAR(x0, x1) ((x0 << 16) + (x1 - x0) * subpos)
/*
#define SET_CUBIC_COEFFICIENTS(x0, x1, x2, x3) { \
	a = 3 * (x1 - x2) + (x3 - x0); \
	b = ((x2 << 2) + (x0 << 1) - (5 * x1 + x3)) << 15; \
	c = (x2 - x0) << 15; \
}
#define CUBIC(d) MULSC(MULSC(MULSC((a * subpos >> 1) + b, subpos) + c, subpos) + (d << 16), vol)
*/
#define CUBIC(x0, x1, x2, x3) (( \
	x0 * cubicA0[subpos >> 6] + \
	x1 * cubicA1[subpos >> 6] + \
	x2 * cubicA1[1 + (subpos >> 6 ^ 1023)] + \
	x3 * cubicA0[1 + (subpos >> 6 ^ 1023)]) << 6)
#define CUBICVOL(x, vol) (int)((LONG_LONG)(x) * (vol << 12) >> 32)
#include "resample.inc"


#undef dumb_reset_resampler
#undef dumb_start_resampler
#undef process_pickup
#undef dumb_resample
#undef dumb_resample_get_current_sample
#undef dumb_end_resampler



void dumb_reset_resampler_n(int n, DUMB_RESAMPLER *resampler, void *src, int src_channels, long pos, long start, long end)
{
	if (n == 8)
		dumb_reset_resampler_8(resampler, src, src_channels, pos, start, end);
	else if (n == 16)
		dumb_reset_resampler_16(resampler, src, src_channels, pos, start, end);
	else
		dumb_reset_resampler(resampler, src, src_channels, pos, start, end);
}



DUMB_RESAMPLER *dumb_start_resampler_n(int n, void *src, int src_channels, long pos, long start, long end)
{
	if (n == 8)
		return dumb_start_resampler_8(src, src_channels, pos, start, end);
	else if (n == 16)
		return dumb_start_resampler_16(src, src_channels, pos, start, end);
	else
		return dumb_start_resampler(src, src_channels, pos, start, end);
}



long dumb_resample_n_1_1(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, float volume, float delta)
{
	if (n == 8)
		return dumb_resample_8_1_1(resampler, dst, dst_size, volume, delta);
	else if (n == 16)
		return dumb_resample_16_1_1(resampler, dst, dst_size, volume, delta);
	else
		return dumb_resample_1_1(resampler, dst, dst_size, volume, delta);
}



long dumb_resample_n_1_2(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, float volume_left, float volume_right, float delta)
{
	if (n == 8)
		return dumb_resample_8_1_2(resampler, dst, dst_size, volume_left, volume_right, delta);
	else if (n == 16)
		return dumb_resample_16_1_2(resampler, dst, dst_size, volume_left, volume_right, delta);
	else
		return dumb_resample_1_2(resampler, dst, dst_size, volume_left, volume_right, delta);
}



long dumb_resample_n_2_1(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, float volume_left, float volume_right, float delta)
{
	if (n == 8)
		return dumb_resample_8_2_1(resampler, dst, dst_size, volume_left, volume_right, delta);
	else if (n == 16)
		return dumb_resample_16_2_1(resampler, dst, dst_size, volume_left, volume_right, delta);
	else
		return dumb_resample_2_1(resampler, dst, dst_size, volume_left, volume_right, delta);
}



long dumb_resample_n_2_2(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, float volume_left, float volume_right, float delta)
{
	if (n == 8)
		return dumb_resample_8_2_2(resampler, dst, dst_size, volume_left, volume_right, delta);
	else if (n == 16)
		return dumb_resample_16_2_2(resampler, dst, dst_size, volume_left, volume_right, delta);
	else
		return dumb_resample_2_2(resampler, dst, dst_size, volume_left, volume_right, delta);
}



void dumb_resample_get_current_sample_n_1_1(int n, DUMB_RESAMPLER *resampler, float volume, sample_t *dst)
{
	if (n == 8)
		dumb_resample_get_current_sample_8_1_1(resampler, volume, dst);
	else if (n == 16)
		dumb_resample_get_current_sample_16_1_1(resampler, volume, dst);
	else
		dumb_resample_get_current_sample_1_1(resampler, volume, dst);
}



void dumb_resample_get_current_sample_n_1_2(int n, DUMB_RESAMPLER *resampler, float volume_left, float volume_right, sample_t *dst)
{
	if (n == 8)
		dumb_resample_get_current_sample_8_1_2(resampler, volume_left, volume_right, dst);
	else if (n == 16)
		dumb_resample_get_current_sample_16_1_2(resampler, volume_left, volume_right, dst);
	else
		dumb_resample_get_current_sample_1_2(resampler, volume_left, volume_right, dst);
}



void dumb_resample_get_current_sample_n_2_1(int n, DUMB_RESAMPLER *resampler, float volume_left, float volume_right, sample_t *dst)
{
	if (n == 8)
		dumb_resample_get_current_sample_8_2_1(resampler, volume_left, volume_right, dst);
	else if (n == 16)
		dumb_resample_get_current_sample_16_2_1(resampler, volume_left, volume_right, dst);
	else
		dumb_resample_get_current_sample_2_1(resampler, volume_left, volume_right, dst);
}



void dumb_resample_get_current_sample_n_2_2(int n, DUMB_RESAMPLER *resampler, float volume_left, float volume_right, sample_t *dst)
{
	if (n == 8)
		dumb_resample_get_current_sample_8_2_2(resampler, volume_left, volume_right, dst);
	else if (n == 16)
		dumb_resample_get_current_sample_16_2_2(resampler, volume_left, volume_right, dst);
	else
		dumb_resample_get_current_sample_2_2(resampler, volume_left, volume_right, dst);
}



void dumb_end_resampler_n(int n, DUMB_RESAMPLER *resampler)
{
	if (n == 8)
		dumb_end_resampler_8(resampler);
	else if (n == 16)
		dumb_end_resampler_16(resampler);
	else
		dumb_end_resampler(resampler);
}
