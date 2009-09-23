/*
  $Id: cddb.h,v 1.4 2007/06/16 20:12:16 rocky Exp $

  Copyright (C) 2005, 2007 Rocky Bernstein <rocky@gnu.org>
  
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

typedef struct cddb_opts_s 
{
  char          *email;  /* email to report to CDDB server. */
  char          *server; /* CDDB server to contact */
  int            port;   /* port number to contact CDDB server. */
  int            http;   /* 1 if use http proxy */
  int            timeout;
  bool           disable_cache; /* If set the below is meaningless. */
  char          *cachedir;
} cddb_opts_t;

cddb_opts_t cddb_opts;

/*!
   Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
   consisting of the concatenation of 3 things:
      the sum of the decimal digits of sizes of all tracks, 
      the total length of the disk, and 
      the number of tracks.
*/
u_int32_t cddb_discid(CdIo_t *p_cdio, track_t i_tracks);

#ifdef HAVE_CDDB
#include <cddb/cddb.h>

typedef void (*error_fn_t) (const char *msg);

bool init_cddb(CdIo_t *p_cdio, cddb_conn_t **pp_conn, 
	       cddb_disc_t **pp_cddb_disc, error_fn_t errmsg,  
	       track_t i_first_track, track_t i_tracks, int *i_cddb_matches);
#endif
