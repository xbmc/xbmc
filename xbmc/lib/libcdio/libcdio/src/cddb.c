/*
  $Id: cddb.c,v 1.6 2007/06/16 20:12:16 rocky Exp $

  Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/audio.h>
#include "cddb.h"

/*!
   Returns the sum of the decimal digits in a number. Eg. 1955 = 20
*/
static int
cddb_dec_digit_sum(int n)
{
  int ret=0;
  
  for (;;) {
    ret += n%10;
    n    = n/10;
    if (!n) return ret;
  }
}

/*!
   Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
   consisting of the concatenation of 3 things:
      the sum of the decimal digits of sizes of all tracks, 
      the total length of the disk, and 
      the number of tracks.
*/
u_int32_t
cddb_discid(CdIo_t *p_cdio, track_t i_tracks)
{
  int i,t,n=0;
  msf_t start_msf;
  msf_t msf;
  
  for (i = 1; i <= i_tracks; i++) {
    cdio_get_track_msf(p_cdio, i, &msf);
    n += cddb_dec_digit_sum(cdio_audio_get_msf_seconds(&msf));
  }

  cdio_get_track_msf(p_cdio, 1, &start_msf);
  cdio_get_track_msf(p_cdio, CDIO_CDROM_LEADOUT_TRACK, &msf);
  
  t = cdio_audio_get_msf_seconds(&msf)-cdio_audio_get_msf_seconds(&start_msf);
  
  return ((n % 0xff) << 24 | t << 8 | i_tracks);
}

#ifdef HAVE_CDDB
bool 
init_cddb(CdIo_t *p_cdio, cddb_conn_t **pp_conn, cddb_disc_t **pp_cddb_disc, 
	  error_fn_t errmsg, track_t i_first_track, track_t i_tracks,
	  int *i_cddb_matches)
{
  track_t i;
  
  *pp_conn =  cddb_new();
  *pp_cddb_disc = NULL;
  
  if (!*pp_conn) {
    errmsg("unable to initialize libcddb");
    return false;
  }
  
  if (NULL == cddb_opts.email) 
    cddb_set_email_address(*pp_conn, "me@home");
  else 
    cddb_set_email_address(*pp_conn, cddb_opts.email);
  
  if (NULL == cddb_opts.server) 
    cddb_set_server_name(*pp_conn, "freedb.freedb.org");
  else 
    cddb_set_server_name(*pp_conn, cddb_opts.server);
  
  if (cddb_opts.timeout >= 0) 
    cddb_set_timeout(*pp_conn, cddb_opts.timeout);
  
  cddb_set_server_port(*pp_conn, cddb_opts.port);
  
  if (cddb_opts.http) 
    cddb_http_enable(*pp_conn);
  else 
    cddb_http_disable(*pp_conn);
  
  if (NULL != cddb_opts.cachedir) 
    cddb_cache_set_dir(*pp_conn, cddb_opts.cachedir);
  
  if (cddb_opts.disable_cache) 
    cddb_cache_disable(*pp_conn);
  
  *pp_cddb_disc = cddb_disc_new();
  if (!*pp_cddb_disc) {
    errmsg("unable to create CDDB disc structure");
    cddb_destroy(*pp_conn);
    return false;
  }
  for(i = 0; i < i_tracks; i++) {
    cddb_track_t *t = cddb_track_new(); 
    cddb_track_set_frame_offset(t, 
				cdio_get_track_lba(p_cdio, i+i_first_track));
    cddb_disc_add_track(*pp_cddb_disc, t);
  }
  
  cddb_disc_set_length(*pp_cddb_disc, 
		       cdio_get_track_lba(p_cdio, CDIO_CDROM_LEADOUT_TRACK) 
		       / CDIO_CD_FRAMES_PER_SEC);
  
  if (!cddb_disc_calc_discid(*pp_cddb_disc)) {
    errmsg("libcddb calc discid failed.");
    cddb_destroy(*pp_conn);
    return false;
  }
  
  *i_cddb_matches = cddb_query(*pp_conn, *pp_cddb_disc);
  
  if (-1 == *i_cddb_matches) 
    errmsg(cddb_error_str(cddb_errno(*pp_conn)));

  cddb_read(*pp_conn, *pp_cddb_disc);
  return true;
}
#endif /*HAVE_CDDB*/
