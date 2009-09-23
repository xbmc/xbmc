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
 * rendsig.c - Wrappers to render samples from        / / \  \
 *             the signals in a DUH.                 | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>

#include "dumb.h"
#include "internal/dumb.h"



struct DUH_SIGRENDERER
{
	DUH_SIGTYPE_DESC *desc;

	sigrenderer_t *sigrenderer;

	int n_channels;

	long pos;
	int subpos;

	DUH_SIGRENDERER_SAMPLE_ANALYSER_CALLBACK callback;
	void *callback_data;
};



DUH_SIGRENDERER *duh_start_sigrenderer(DUH *duh, int sig, int n_channels, long pos)
{
	DUH_SIGRENDERER *sigrenderer;

	DUH_SIGNAL *signal;
	DUH_START_SIGRENDERER proc;

	if (!duh)
		return NULL;

	if ((unsigned int)sig >= (unsigned int)duh->n_signals)
		return NULL;

	signal = duh->signal[sig];
	if (!signal)
		return NULL;

	sigrenderer = malloc(sizeof(*sigrenderer));
	if (!sigrenderer)
		return NULL;

	sigrenderer->desc = signal->desc;

	proc = sigrenderer->desc->start_sigrenderer;

	if (proc) {
		duh->signal[sig] = NULL;
		sigrenderer->sigrenderer = (*proc)(duh, signal->sigdata, n_channels, pos);
		duh->signal[sig] = signal;

		if (!sigrenderer->sigrenderer) {
			free(sigrenderer);
			return NULL;
		}
	} else
		sigrenderer->sigrenderer = NULL;

	sigrenderer->n_channels = n_channels;

	sigrenderer->pos = pos;
	sigrenderer->subpos = 0;

	sigrenderer->callback = NULL;

	return sigrenderer;
}



#include <stdio.h>
void duh_sigrenderer_set_callback(
	DUH_SIGRENDERER *sigrenderer,
	DUH_SIGRENDERER_CALLBACK callback, void *data
)
{
	(void)sigrenderer;
	(void)callback;
	(void)data;
	fprintf(stderr,
		"Call to deprecated function duh_sigrenderer_set_callback(). The callback\n"
		"was not installed. See dumb/docs/deprec.txt for how to fix this.\n");
}



void duh_sigrenderer_set_analyser_callback(
	DUH_SIGRENDERER *sigrenderer,
	DUH_SIGRENDERER_ANALYSER_CALLBACK callback, void *data
)
{
	(void)sigrenderer;
	(void)callback;
	(void)data;
	fprintf(stderr,
		"Call to deprecated function duh_sigrenderer_set_analyser_callback(). The\n"
		"callback was not installed. See dumb/docs/deprec.txt for how to fix this.\n");
}



void duh_sigrenderer_set_sample_analyser_callback(
	DUH_SIGRENDERER *sigrenderer,
	DUH_SIGRENDERER_SAMPLE_ANALYSER_CALLBACK callback, void *data
)
{
	if (sigrenderer) {
		sigrenderer->callback = callback;
		sigrenderer->callback_data = data;
	}
}



int duh_sigrenderer_get_n_channels(DUH_SIGRENDERER *sigrenderer)
{
	return sigrenderer ? sigrenderer->n_channels : 0;
}



long duh_sigrenderer_get_position(DUH_SIGRENDERER *sigrenderer)
{
	return sigrenderer ? sigrenderer->pos : -1;
}



void duh_sigrenderer_set_sigparam(
	DUH_SIGRENDERER *sigrenderer,
	unsigned char id, long value
)
{
	DUH_SIGRENDERER_SET_SIGPARAM proc;

	if (!sigrenderer) return;

	proc = sigrenderer->desc->sigrenderer_set_sigparam;
	if (proc)
		(*proc)(sigrenderer->sigrenderer, id, value);
	else
		TRACE("Parameter #%d = %ld for signal %c%c%c%c, which does not take parameters.\n",
			(int)id,
			value,
			(int)(sigrenderer->desc->type >> 24),
			(int)(sigrenderer->desc->type >> 16),
			(int)(sigrenderer->desc->type >> 8),
			(int)(sigrenderer->desc->type));
}



long duh_sigrenderer_generate_samples(
	DUH_SIGRENDERER *sigrenderer,
	float volume, float delta,
	long size, sample_t **samples
)
{
	long rendered;
	LONG_LONG t;

	if (!sigrenderer) return 0;

	rendered = (*sigrenderer->desc->sigrenderer_generate_samples)
				(sigrenderer->sigrenderer, volume, delta, size, samples);

	if (rendered) {
		if (sigrenderer->callback)
			(*sigrenderer->callback)(sigrenderer->callback_data,
				(const sample_t *const *)samples, sigrenderer->n_channels, rendered);

		t = sigrenderer->subpos + (LONG_LONG)(delta * 65536.0 + 0.5) * rendered;

		sigrenderer->pos += (long)(t >> 16);
		sigrenderer->subpos = (int)t & 65535;
	}

	return rendered;
}



/* DEPRECATED */
long duh_sigrenderer_get_samples(
	DUH_SIGRENDERER *sigrenderer,
	float volume, float delta,
	long size, sample_t **samples
)
{
	sample_t **s;
	long rendered;
	long i;
	int j;
	if (!samples) return duh_sigrenderer_generate_samples(sigrenderer, volume, delta, size, NULL);
	s = allocate_sample_buffer(sigrenderer->n_channels, size);
	if (!s) return 0;
	dumb_silence(s[0], sigrenderer->n_channels * size);
	rendered = duh_sigrenderer_generate_samples(sigrenderer, volume, delta, size, s);
	for (j = 0; j < sigrenderer->n_channels; j++)
		for (i = 0; i < rendered; i++)
			samples[j][i] += s[0][i*sigrenderer->n_channels+j];
	destroy_sample_buffer(s);
	return rendered;
}



/* DEPRECATED */
long duh_render_signal(
	DUH_SIGRENDERER *sigrenderer,
	float volume, float delta,
	long size, sample_t **samples
)
{
	sample_t **s;
	long rendered;
	long i;
	int j;
	if (!samples) return duh_sigrenderer_generate_samples(sigrenderer, volume, delta, size, NULL);
	s = allocate_sample_buffer(sigrenderer->n_channels, size);
	if (!s) return 0;
	dumb_silence(s[0], sigrenderer->n_channels * size);
	rendered = duh_sigrenderer_generate_samples(sigrenderer, volume, delta, size, s);
	for (j = 0; j < sigrenderer->n_channels; j++)
		for (i = 0; i < rendered; i++)
			samples[j][i] += s[0][i*sigrenderer->n_channels+j] >> 8;
	destroy_sample_buffer(s);
	return rendered;
}



void duh_sigrenderer_get_current_sample(DUH_SIGRENDERER *sigrenderer, float volume, sample_t *samples)
{
	if (sigrenderer)
		(*sigrenderer->desc->sigrenderer_get_current_sample)(sigrenderer->sigrenderer, volume, samples);
}



void duh_end_sigrenderer(DUH_SIGRENDERER *sigrenderer)
{
	if (sigrenderer) {
		if (sigrenderer->desc->end_sigrenderer)
			if (sigrenderer->sigrenderer)
				(*sigrenderer->desc->end_sigrenderer)(sigrenderer->sigrenderer);

		free(sigrenderer);
	}
}



DUH_SIGRENDERER *duh_encapsulate_raw_sigrenderer(sigrenderer_t *vsigrenderer, DUH_SIGTYPE_DESC *desc, int n_channels, long pos)
{
	DUH_SIGRENDERER *sigrenderer;

	if (desc->start_sigrenderer && !vsigrenderer) return NULL;

	sigrenderer = malloc(sizeof(*sigrenderer));
	if (!sigrenderer) {
		if (desc->end_sigrenderer)
			if (vsigrenderer)
				(*desc->end_sigrenderer)(vsigrenderer);
		return NULL;
	}

	sigrenderer->desc = desc;
	sigrenderer->sigrenderer = vsigrenderer;

	sigrenderer->n_channels = n_channels;

	sigrenderer->pos = pos;
	sigrenderer->subpos = 0;

	sigrenderer->callback = NULL;

	return sigrenderer;
}



sigrenderer_t *duh_get_raw_sigrenderer(DUH_SIGRENDERER *sigrenderer, long type)
{
	if (sigrenderer && sigrenderer->desc->type == type)
		return sigrenderer->sigrenderer;

	return NULL;
}



#if 0
// This function is disabled because we don't know whether we want to destroy
// the sigrenderer if the type doesn't match. We don't even know if we need
// the function at all. Who would want to keep an IT_SIGRENDERER (for
// instance) without keeping the DUH_SIGRENDERER?
sigrenderer_t *duh_decompose_to_raw_sigrenderer(DUH_SIGRENDERER *sigrenderer, long type)
{
	if (sigrenderer && sigrenderer->desc->type == type) {



	if (sigrenderer) {
		if (sigrenderer->desc->end_sigrenderer)
			if (sigrenderer->sigrenderer)
				(*sigrenderer->desc->end_sigrenderer)(sigrenderer->sigrenderer);

		free(sigrenderer);
	}






		return sigrenderer->sigrenderer;
	}

	return NULL;
}
#endif
