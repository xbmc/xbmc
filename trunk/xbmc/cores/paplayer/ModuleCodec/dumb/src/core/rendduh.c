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
 * rendduh.c - Functions for rendering a DUH into     / / \  \
 *             an end-user sample format.            | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>
#include <limits.h>

#include "dumb.h"
#include "internal/dumb.h"



/* On the x86, we can use some tricks to speed stuff up */
#if (defined _MSC_VER) || (defined __DJGPP__) || (defined __MINGW__)
// Can't we detect Linux and other x86 platforms here? :/

#define FAST_MID(var, min, max) {                  \
	var -= (min);                                  \
	var &= (~var) >> (sizeof(var) * CHAR_BIT - 1); \
	var += (min);                                  \
	var -= (max);                                  \
	var &= var >> (sizeof(var) * CHAR_BIT - 1);    \
	var += (max);                                  \
}

#define CONVERT8(src, pos, signconv) {       \
	signed int f = (src + 0x8000) >> 16;     \
	FAST_MID(f, -128, 127);                  \
	((char*)sptr)[pos] = (char)f ^ signconv; \
}

#define CONVERT16(src, pos, signconv) {          \
	signed int f = (src + 0x80) >> 8;            \
	FAST_MID(f, -32768, 32767);                  \
	((short*)sptr)[pos] = (short)(f ^ signconv); \
}

#else

#define CONVERT8(src, pos, signconv)		  \
{											  \
	signed int f = (src + 0x8000) >> 16;	  \
	f = MID(-128, f, 127);					  \
	((char *)sptr)[pos] = (char)f ^ signconv; \
}



#define CONVERT16(src, pos, signconv)			  \
{												  \
	signed int f = (src + 0x80) >> 8;			  \
	f = MID(-32768, f, 32767);					  \
	((short *)sptr)[pos] = (short)(f ^ signconv); \
}

#endif



/* DEPRECATED */
DUH_SIGRENDERER *duh_start_renderer(DUH *duh, int n_channels, long pos)
{
	return duh_start_sigrenderer(duh, 0, n_channels, pos);
}



long duh_render(
	DUH_SIGRENDERER *sigrenderer,
	int bits, int unsign,
	float volume, float delta,
	long size, void *sptr
)
{
	long n;

	sample_t **sampptr;

	int n_channels;

	ASSERT(bits == 8 || bits == 16);
	ASSERT(sptr);

	if (!sigrenderer)
		return 0;

	n_channels = duh_sigrenderer_get_n_channels(sigrenderer);

	ASSERT(n_channels > 0);
	/* This restriction will be removed when need be. At the moment, tightly
	 * optimised loops exist for exactly one or two channels.
	 */
	ASSERT(n_channels <= 2);

	sampptr = allocate_sample_buffer(n_channels, size);

	if (!sampptr)
		return 0;

	dumb_silence(sampptr[0], n_channels * size);

	size = duh_sigrenderer_generate_samples(sigrenderer, volume, delta, size, sampptr);

	if (bits == 16) {
		int signconv = unsign ? 0x8000 : 0x0000;

		for (n = 0; n < size * n_channels; n++) {
			CONVERT16(sampptr[0][n], n, signconv);
		}
	} else {
		char signconv = unsign ? 0x80 : 0x00;

		for (n = 0; n < size * n_channels; n++) {
			CONVERT8(sampptr[0][n], n, signconv);
		}
	}

	destroy_sample_buffer(sampptr);

	return size;
}



/* DEPRECATED */
int duh_renderer_get_n_channels(DUH_SIGRENDERER *dr)
{
	return duh_sigrenderer_get_n_channels(dr);
}



/* DEPRECATED */
long duh_renderer_get_position(DUH_SIGRENDERER *dr)
{
	return duh_sigrenderer_get_position(dr);
}



/* DEPRECATED */
void duh_end_renderer(DUH_SIGRENDERER *dr)
{
	duh_end_sigrenderer(dr);
}



/* DEPRECATED */
DUH_SIGRENDERER *duh_renderer_encapsulate_sigrenderer(DUH_SIGRENDERER *sigrenderer)
{
	return sigrenderer;
}



/* DEPRECATED */
DUH_SIGRENDERER *duh_renderer_get_sigrenderer(DUH_SIGRENDERER *dr)
{
	return dr;
}



/* DEPRECATED */
DUH_SIGRENDERER *duh_renderer_decompose_to_sigrenderer(DUH_SIGRENDERER *dr)
{
	return dr;
}
