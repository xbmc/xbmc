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
 * resample.inc - Resampling helper template.         / / \  \
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



void dumb_reset_resampler(DUMB_RESAMPLER *resampler, SRCTYPE *src, int src_channels, long pos, long start, long end)
{
	int i;
	resampler->src = src;
	resampler->pos = pos;
	resampler->subpos = 0;
	resampler->start = start;
	resampler->end = end;
	resampler->dir = 1;
	resampler->pickup = NULL;
	resampler->pickup_data = NULL;
	resampler->min_quality = 0;
	resampler->max_quality = DUMB_RQ_N_LEVELS - 1;
	for (i = 0; i < src_channels*3; i++) resampler->X[i] = 0;
	resampler->overshot = -1;
}



DUMB_RESAMPLER *dumb_start_resampler(SRCTYPE *src, int src_channels, long pos, long start, long end)
{
	DUMB_RESAMPLER *resampler = malloc(sizeof(*resampler));
	if (!resampler) return NULL;
	dumb_reset_resampler(resampler, src, src_channels, pos, start, end);
	return resampler;
}



/* Create mono source resampler. */
#define SUFFIX2 _1
#define SRC_CHANNELS 1
#define DIVIDE_BY_SRC_CHANNELS(x) (x)
#define COPYSRC(dstarray, dstindex, srcarray, srcindex) (dstarray)[dstindex] = (srcarray)[srcindex]
#define COPYSRC2(dstarray, dstindex, condition, srcarray, srcindex) (dstarray)[dstindex] = condition ? (srcarray)[srcindex] : 0
#define MONO_DEST_VOLUME_PARAMETERS float volume
#define MONO_DEST_VOLUME_VARIABLES vol
#define MONO_DEST_VOLUME_ZEROS 0
#define SET_MONO_DEST_VOLUME_VARIABLES vol = (int)floor(volume * 65536.0 + 0.5)
#define MONO_DEST_VOLUMES_ARE_ZERO (vol == 0)
#define MONO_DEST_MIX_ALIAS(op, offset) *dst++ op ALIAS(x[offset], vol)
#define STEREO_DEST_MIX_ALIAS(op, offset) { \
	int xm = x[offset]; \
	*dst++ op ALIAS(xm, lvol); \
	*dst++ op ALIAS(xm, rvol); \
}
#define MONO_DEST_MIX_LINEAR(op, o0, o1) *dst++ op MULSC(LINEAR(x[o0], x[o1]), vol)
#define STEREO_DEST_MIX_LINEAR(op, o0, o1) { \
	int xm = LINEAR(x[o0], x[o1]); \
	*dst++ op MULSC(xm, lvol); \
	*dst++ op MULSC(xm, rvol); \
}
#define MONO_DEST_MIX_CUBIC(op, x0, x3, o0, o1, o2, o3) *dst++ op CUBICVOL(CUBIC(x0[o0], x[o1], x[o2], x3[o3]), vol)
#define STEREO_DEST_MIX_CUBIC(op, x0, x3, o0, o1, o2, o3) { \
	int xm = CUBIC(x0[o0], x[o1], x[o2], x3[o3]); \
	*dst++ op CUBICVOL(xm, lvol); \
	*dst++ op CUBICVOL(xm, rvol); \
}
#include "resamp2.inc"

/* Create stereo source resampler. */
#define SUFFIX2 _2
#define SRC_CHANNELS 2
#define DIVIDE_BY_SRC_CHANNELS(x) ((x) >> 1)
#define COPYSRC(dstarray, dstindex, srcarray, srcindex) { \
	(dstarray)[(dstindex)*2] = (srcarray)[(srcindex)*2]; \
	(dstarray)[(dstindex)*2+1] = (srcarray)[(srcindex)*2+1]; \
}
#define COPYSRC2(dstarray, dstindex, condition, srcarray, srcindex) { \
	if (condition) { \
		(dstarray)[(dstindex)*2] = (srcarray)[(srcindex)*2]; \
		(dstarray)[(dstindex)*2+1] = (srcarray)[(srcindex)*2+1]; \
	} else { \
		(dstarray)[(dstindex)*2] = 0; \
		(dstarray)[(dstindex)*2+1] = 0; \
	} \
}
#define MONO_DEST_VOLUME_PARAMETERS float volume_left, float volume_right
#define MONO_DEST_VOLUME_VARIABLES lvol, rvol
#define MONO_DEST_VOLUME_ZEROS 0, 0
#define SET_MONO_DEST_VOLUME_VARIABLES { \
	lvol = (int)floor(volume_left * 65536.0 + 0.5); \
	rvol = (int)floor(volume_right * 65536.0 + 0.5); \
}
#define MONO_DEST_VOLUMES_ARE_ZERO (lvol == 0 && rvol == 0)
#define MONO_DEST_MIX_ALIAS(op, offset) *dst++ op ALIAS(x[(offset)*2], lvol) + ALIAS(x[(offset)*2+1], rvol)
#define STEREO_DEST_MIX_ALIAS(op, offset) { \
	*dst++ op ALIAS(x[(offset)*2], lvol); \
	*dst++ op ALIAS(x[(offset)*2+1], rvol); \
}
#define MONO_DEST_MIX_LINEAR(op, o0, o1) *dst++ op MULSC(LINEAR(x[(o0)*2], x[(o1)*2]), lvol) + MULSC(LINEAR(x[(o0)*2+1], x[(o1)*2+1]), rvol)
#define STEREO_DEST_MIX_LINEAR(op, o0, o1) { \
	*dst++ op MULSC(LINEAR(x[(o0)*2], x[(o1)*2]), lvol); \
	*dst++ op MULSC(LINEAR(x[(o0)*2+1], x[(o1)*2+1]), rvol); \
}
#define MONO_DEST_MIX_CUBIC(op, x0, x3, o0, o1, o2, o3) *dst++ op \
	CUBICVOL(CUBIC(x0[(o0)*2], x[(o1)*2], x[(o2)*2], x3[(o3)*2]), lvol) + \
	CUBICVOL(CUBIC(x0[(o0)*2+1], x[(o1)*2+1], x[(o2)*2+1], x3[(o3)*2+1]), rvol)
#define STEREO_DEST_MIX_CUBIC(op, x0, x3, o0, o1, o2, o3) { \
	*dst++ op CUBICVOL(CUBIC(x0[(o0)*2], x[(o1)*2], x[(o2)*2], x3[(o3)*2]), lvol); \
	*dst++ op CUBICVOL(CUBIC(x0[(o0)*2+1], x[(o1)*2+1], x[(o2)*2+1], x3[(o3)*2+1]), rvol); \
}
#include "resamp2.inc"



void dumb_end_resampler(DUMB_RESAMPLER *resampler)
{
	if (resampler)
		free(resampler);
}



#undef CUBICVOL
#undef CUBIC
#undef LINEAR
#undef ALIAS
#undef SRCBITS
#undef SRCTYPE
#undef SUFFIX
