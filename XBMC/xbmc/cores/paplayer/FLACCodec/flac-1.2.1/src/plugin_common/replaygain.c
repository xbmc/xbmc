/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 * Copyright (C) 2003  Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "replaygain.h"
#include "FLAC/ordinals.h"
#include "FLAC/metadata.h"
#include "share/grabbag.h"

FLAC__bool FLAC_plugin__replaygain_get_from_file(const char *filename,
                                                 double *reference, FLAC__bool *reference_set,
                                                 double *track_gain, FLAC__bool *track_gain_set,
                                                 double *album_gain, FLAC__bool *album_gain_set,
                                                 double *track_peak, FLAC__bool *track_peak_set,
                                                 double *album_peak, FLAC__bool *album_peak_set)
{
	FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
	FLAC__bool ret = false;

	*track_gain_set = *album_gain_set = *track_peak_set = *album_peak_set = false;

	if(0 != iterator) {
		if(FLAC__metadata_simple_iterator_init(iterator, filename, /*read_only=*/true, /*preserve_file_stats=*/true)) {
			FLAC__bool got_vorbis_comments = false;
			ret = true;
			do {
				if(FLAC__metadata_simple_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
					FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iterator);
					if(0 != block) {
						if(grabbag__replaygain_load_from_vorbiscomment(block, /*album_mode=*/false, /*strict=*/true, reference, track_gain, track_peak)) {
							*reference_set = *track_gain_set = *track_peak_set = true;
						}
						if(grabbag__replaygain_load_from_vorbiscomment(block, /*album_mode=*/true, /*strict=*/true, reference, album_gain, album_peak)) {
							*reference_set = *album_gain_set = *album_peak_set = true;
						}
						FLAC__metadata_object_delete(block);
						got_vorbis_comments = true;
					}
				}
			} while (!got_vorbis_comments && FLAC__metadata_simple_iterator_next(iterator));
		}
		FLAC__metadata_simple_iterator_delete(iterator);
	}
	return ret;
}
