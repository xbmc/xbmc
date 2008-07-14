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

#ifndef FLAC__PLUGIN_COMMON__REPLAYGAIN_H
#define FLAC__PLUGIN_COMMON__REPLAYGAIN_H

#include "FLAC/ordinals.h"

FLAC__bool FLAC_plugin__replaygain_get_from_file(const char *filename,
                                           double *reference, FLAC__bool *reference_set,
                                           double *track_gain, FLAC__bool *track_gain_set,
                                           double *album_gain, FLAC__bool *album_gain_set,
                                           double *track_peak, FLAC__bool *track_peak_set,
                                           double *album_peak, FLAC__bool *album_peak_set);

#endif
