/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/* 
 * Compile with: gcc -lasound -osetAlsaVolumes setAlsaVolumes.c
 * (libasound2-dev needed)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

int main(int argc, char *argv[])
{
	long volume = 90;
	int verbose = 0;

	int err;
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	char *card = "default";
	int aChar;
	long min=0, max=0;

	opterr = 0;
     
	while ((aChar = getopt (argc, argv, "v")) != -1) {
		switch (aChar) {
		case 'v':
			verbose = 1;
			printf(" Verbose mode ON\n");
			break;
		default:
			abort();
		}
	}

	if ( optind < argc )
		volume = atol(argv[optind]);

        printf("Setting volumes at %ld%%\n", volume);

	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		fprintf(stderr, "Mixer %s open error: %s", card, snd_strerror(err));
		return err;
	}
	if ((err = snd_mixer_attach(handle, card)) < 0) {
		fprintf(stderr, "Mixer attach %s error: %s", card, snd_strerror(err));
		snd_mixer_close(handle);
		return err;
	}
	if( (err=snd_mixer_selem_register(handle,NULL,NULL)) < 0 ){
		fprintf(stderr, "ALSA: snd_mixer_selem_register failed: %d\n",err);
		snd_mixer_close(handle);
		return err;
	}
	if( (err=snd_mixer_load(handle)) < 0 ){
		fprintf(stderr, "ALSA: snd_mixer_load failed: %d\n",err);
		snd_mixer_close(handle);
		return err;
	}

	snd_mixer_selem_id_alloca(&sid);
	for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem)) {
		snd_mixer_selem_get_id(elem, sid);

		if ( verbose ) {
			const char *controlName;
			controlName = snd_mixer_selem_id_get_name(sid);
			printf("Name = %s\n", controlName);
		}

		if (snd_mixer_selem_has_playback_volume(elem)) {
			if ( verbose ) 
				 printf("    Has playback volume\n");
			snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

			if( (err=snd_mixer_selem_set_playback_volume_all(elem, volume*max/100)) < 0 ) {
				fprintf(stderr, "ALSA: snd_mixer_selem_set_playback_volume_all failed: %d\n",err);
			} else {
				if ( verbose ) 
					printf("    ** Volume increased to %ld\n", volume);
			}
		}
		if (snd_mixer_selem_has_playback_switch(elem)) {
			if ( verbose ) 
				printf("    Has playback switch\n");
			
			if( (err=snd_mixer_selem_set_playback_switch_all(elem, 1)) < 0 ) {
				fprintf(stderr, "ALSA: snd_mixer_selem_set_playback_switch_all failed: %d\n",err);
			} else {
				if ( verbose ) 
					printf("    ** Unmuted\n");
			}
		}
		if (snd_mixer_selem_has_capture_volume(elem)) {
			if ( verbose ) 
				printf("    Has capture volume\n");
			
			if( (err=snd_mixer_selem_set_capture_volume_all(elem, 0)) < 0 ) {
				fprintf(stderr, "ALSA: snd_mixer_selem_set_capture_volume_all failed: %d\n",err);
			} else {
				if ( verbose ) 
					printf("    ** Volume decreased to 0\n");
			}
		}
		if (snd_mixer_selem_has_capture_switch(elem)) {
			if ( verbose ) 
				 printf("    Has capture switch\n");

			if( (err=snd_mixer_selem_set_capture_switch_all(elem, 0)) < 0 ) {
				fprintf(stderr, "ALSA: snd_mixer_selem_set_capture_switch_all failed: %d\n",err);
			} else  {
				if ( verbose ) 
					printf("    ** Muted\n");
			}
		}
	}

	snd_mixer_close(handle);

	return 0;
}
