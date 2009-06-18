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
 * itmisc.c - Miscellaneous functions relating        / / \  \
 *            to module files.                       | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"
#include "internal/it.h"



DUMB_IT_SIGDATA *duh_get_it_sigdata(DUH *duh)
{
	return duh_get_raw_sigdata(duh, 0, SIGTYPE_IT);
}



const unsigned char *dumb_it_sd_get_song_message(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->song_message : NULL;
}



int dumb_it_sd_get_n_orders(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->n_orders : 0;
}



int dumb_it_sd_get_n_samples(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->n_samples : 0;
}



int dumb_it_sd_get_n_instruments(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->n_instruments : 0;
}



const unsigned char *dumb_it_sd_get_sample_name(DUMB_IT_SIGDATA *sd, int i)
{
	ASSERT(sd && sd->sample && i >= 0 && i < sd->n_samples);
	return sd->sample[i].name;
}



const unsigned char *dumb_it_sd_get_sample_filename(DUMB_IT_SIGDATA *sd, int i)
{
	ASSERT(sd && sd->sample && i >= 0 && i < sd->n_samples);
	return sd->sample[i].filename;
}



const unsigned char *dumb_it_sd_get_instrument_name(DUMB_IT_SIGDATA *sd, int i)
{
	ASSERT(sd && sd->instrument && i >= 0 && i < sd->n_instruments);
	return sd->instrument[i].name;
}



const unsigned char *dumb_it_sd_get_instrument_filename(DUMB_IT_SIGDATA *sd, int i)
{
	ASSERT(sd && sd->instrument && i >= 0 && i < sd->n_instruments);
	return sd->instrument[i].filename;
}



int dumb_it_sd_get_initial_global_volume(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->global_volume : 0;
}



void dumb_it_sd_set_initial_global_volume(DUMB_IT_SIGDATA *sd, int gv)
{
	if (sd) sd->global_volume = gv;
}



int dumb_it_sd_get_mixing_volume(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->mixing_volume : 0;
}



void dumb_it_sd_set_mixing_volume(DUMB_IT_SIGDATA *sd, int mv)
{
	if (sd) sd->mixing_volume = mv;
}



int dumb_it_sd_get_initial_speed(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->speed : 0;
}



void dumb_it_sd_set_initial_speed(DUMB_IT_SIGDATA *sd, int speed)
{
	if (sd) sd->speed = speed;
}



int dumb_it_sd_get_initial_tempo(DUMB_IT_SIGDATA *sd)
{
	return sd ? sd->tempo : 0;
}



void dumb_it_sd_set_initial_tempo(DUMB_IT_SIGDATA *sd, int tempo)
{
	if (sd) sd->tempo = tempo;
}



int dumb_it_sd_get_initial_channel_volume(DUMB_IT_SIGDATA *sd, int channel)
{
	ASSERT(channel >= 0 && channel < DUMB_IT_N_CHANNELS);
	return sd ? sd->channel_volume[channel] : 0;
}

void dumb_it_sd_set_initial_channel_volume(DUMB_IT_SIGDATA *sd, int channel, int volume)
{
	ASSERT(channel >= 0 && channel < DUMB_IT_N_CHANNELS);
	if (sd) sd->channel_volume[channel] = volume;
}



int dumb_it_sr_get_current_order(DUMB_IT_SIGRENDERER *sr)
{
	return sr ? sr->order : -1;
}



int dumb_it_sr_get_current_row(DUMB_IT_SIGRENDERER *sr)
{
	return sr ? sr->row : -1;
}



int dumb_it_sr_get_global_volume(DUMB_IT_SIGRENDERER *sr)
{
	return sr ? sr->globalvolume : 0;
}



void dumb_it_sr_set_global_volume(DUMB_IT_SIGRENDERER *sr, int gv)
{
	if (sr) sr->globalvolume = gv;
}



int dumb_it_sr_get_tempo(DUMB_IT_SIGRENDERER *sr)
{
	return sr ? sr->tempo : 0;
}



void dumb_it_sr_set_tempo(DUMB_IT_SIGRENDERER *sr, int tempo)
{
	if (sr) sr->tempo = tempo;
}



int dumb_it_sr_get_speed(DUMB_IT_SIGRENDERER *sr)
{
	return sr ? sr->speed : 0;
}



void dumb_it_sr_set_speed(DUMB_IT_SIGRENDERER *sr, int speed)
{
	if (sr) sr->speed = speed;
}



int dumb_it_sr_get_channel_volume(DUMB_IT_SIGRENDERER *sr, int channel)
{
	return sr ? sr->channel[channel].channelvolume : 0;
}



void dumb_it_sr_set_channel_volume(DUMB_IT_SIGRENDERER *sr, int channel, int volume)
{
	if (sr) sr->channel[channel].channelvolume = volume;
}



void dumb_it_sr_set_channel_muted(DUMB_IT_SIGRENDERER *sr, int channel, int muted)
{
	if (sr) {
		if (muted)
			sr->channel[channel].flags |= IT_CHANNEL_MUTED;
		else
			sr->channel[channel].flags &= ~IT_CHANNEL_MUTED;
	}
}



int dumb_it_sr_get_channel_muted(DUMB_IT_SIGRENDERER *sr, int channel)
{
	return sr ? (sr->channel[channel].flags & IT_CHANNEL_MUTED) != 0 : 0;
}
