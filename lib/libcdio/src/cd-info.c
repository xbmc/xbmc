/*
    $Id: cd-info.c,v 1.111 2005/01/27 03:10:06 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 1996, 1997, 1998  Gerd Knorr <kraxel@bytesex.org>
         and Heiko Eiﬂfeldt <heiko@hexco.de>

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
/*
  CD Info - prints various information about a CD, and detects the type of 
  the CD.
 
*/

#include "util.h"

#ifdef _XBOX
#include "xtl.h"
#endif

#ifdef HAVE_CDDB
#include <cddb/cddb.h>
#endif

#ifdef HAVE_VCDINFO
#include <libvcd/logging.h>
#include <libvcd/files.h>
#include <libvcd/info.h>
#endif

#include <cdio/bytesex.h>
#include <cdio/ds.h>
#include <cdio/util.h>
#include <cdio/cd_types.h>
#include <cdio/cdtext.h>
#include <cdio/iso9660.h>
#include <cdio/scsi_mmc.h>

#include "cdio_assert.h"

#include <fcntl.h>
#ifdef __linux__
# include <linux/version.h>
# include <linux/cdrom.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,50)
#  include <linux/ucdrom.h>
# endif
#endif
 
#include <errno.h>

#define STRONG "__________________________________\n"
#define NORMAL ""

#if CDIO_IOCTL_FINISHED
struct cdrom_multisession  ms;
struct cdrom_subchnl       sub;
#endif

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  int            no_tracks;
  int            no_ioctl;
  int            no_analysis;
  char          *access_mode; /* Access method driver should use for control */
#ifdef HAVE_CDDB
  int            no_cddb;     /* If set the below are meaningless. */
  char          *cddb_email;  /* email to report to CDDB server. */
  char          *cddb_server; /* CDDB server to contact */
  int            cddb_port;   /* port number to contact CDDB server. */
  int            cddb_http;   /* 1 if use http proxy */
  int            cddb_timeout;
  bool           cddb_disable_cache; /* If set the below is meaningless. */
  char          *cddb_cachedir;
#endif
  int            no_vcd;
  int            show_dvd;
  int            no_device;
  int            no_disc_mode;
  uint32_t       debug_level;
  int            version_only;
  int            silent;
  int            no_header;
  int            no_joliet;
  int            print_iso9660;
  int            list_drives;
  source_image_t source_image;
} opts;
     
/* Configuration option codes */
enum {
  
  OP_SOURCE_UNDEF,
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_CDRDAO,
  OP_SOURCE_NRG ,
  OP_SOURCE_DEVICE,
  
  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

char *temp_str;


/* Parse a all options. */
static bool
parse_options (int argc, const char *argv[])
{
  int opt;
  char *psz_my_source;
  
  struct poptOption optionsTable[] = {
    {"access-mode",       'a', POPT_ARG_STRING, &opts.access_mode, 0,
     "Set CD access methed"},
    
    {"debug",       'd', POPT_ARG_INT, &opts.debug_level, 0,
     "Set debugging to LEVEL"},
    
    {"no-tracks",   'T', POPT_ARG_NONE, &opts.no_tracks, 0,
     "Don't show track information"},
    
    {"no-analyze",  'A', POPT_ARG_NONE, &opts.no_analysis, 0,
     "Don't filesystem analysis"},
    
#ifdef HAVE_CDDB
    {"no-cddb",     '\0', POPT_ARG_NONE, &opts.no_cddb, 0,
     "Don't look up audio CDDB information or print that"},
    
    {"cddb-port",   'P', POPT_ARG_INT, &opts.cddb_port, 8880,
     "CDDB port number to use (default 8880)"},
    
    {"cddb-http",   'H', POPT_ARG_NONE, &opts.cddb_http, 0,
     "Lookup CDDB via HTTP proxy (default no proxy)"},
    
    {"cddb-server", '\0', POPT_ARG_STRING, &opts.cddb_server, 0,
     "CDDB server to contact for information (default: freedb.freedb.org)"},
    
    {"cddb-cache",  '\0', POPT_ARG_STRING, &opts.cddb_cachedir, 0,
     "Location of CDDB cache directory (default ~/.cddbclient)"},
    
    {"cddb-email",  '\0', POPT_ARG_STRING, &opts.cddb_email, 0,
     "Email address to give CDDB server (default me@home"},
    
    {"no-cddb-cache", '\0', POPT_ARG_NONE, &opts.cddb_disable_cache, 0,
     "Lookup CDDB via HTTP proxy (default no proxy)"},
    
    {"cddb-timeout",  '\0', POPT_ARG_INT, &opts.cddb_timeout, 0,
     "CDDB timeout value in seconds (default 10 seconds)"},
#endif
  
    {"no-device-info", '\0', POPT_ARG_NONE, &opts.no_device, 0,
     "Don't show device info, just CD info"},
    
    {"no-disc-mode", '\0', POPT_ARG_NONE, &opts.no_disc_mode, 0,
     "Don't show disc-mode info"},
    
    {"dvd",   '\0', POPT_ARG_NONE, &opts.show_dvd, 0,
     "Attempt to give DVD information if a DVD is found."},

#ifdef HAVE_VCDINFO
    {"no-vcd",   'v', POPT_ARG_NONE, &opts.no_vcd, 0,
     "Don't look up Video CD information"},
#else 
    {"no-vcd",   'v', POPT_ARG_NONE, &opts.no_vcd, 1,
     "Don't look up Video CD information - for this build, this is always set"},
#endif
    {"no-ioctl",  'I', POPT_ARG_NONE,  &opts.no_ioctl, 0,
     "Don't show ioctl() information"},
    
    {"bin-file", 'b', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_BIN, "set \"bin\" CD-ROM disk image file as source", "FILE"},
    
    {"cue-file", 'c', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_CUE, "set \"cue\" CD-ROM disk image file as source", "FILE"},
    
    {"nrg-file", 'N', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_NRG, "set Nero CD-ROM disk image file as source", "FILE"},
    
    {"toc-file", 't', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_CDRDAO, "set cdrdao CD-ROM disk image file as source", "FILE"},
    
    {"input", 'i', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_AUTO,
     "set source and determine if \"bin\" image or device", "FILE"},
    
    {"iso9660",  '\0', POPT_ARG_NONE, &opts.print_iso9660, 0,
     "print directory contents of any ISO-9660 filesystems"},
    
    {"cdrom-device", 'C', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_DEVICE,
     "set CD-ROM device as source", "DEVICE"},
    
    {"list-drives",   'l', POPT_ARG_NONE, &opts.list_drives, 0,
     "Give a list of CD-drives" },
    
    {"no-header", '\0', POPT_ARG_NONE, &opts.no_header, 
     0, "Don't display header and copyright (for regression testing)"},
    
#ifdef HAVE_JOLIET    
    {"no-joliet", '\0', POPT_ARG_NONE, &opts.no_joliet, 
     0, "Don't use Joliet extensions"},
#endif /*HAVE_JOLIET*/
    
    {"quiet",       'q', POPT_ARG_NONE, &opts.silent, 0,
     "Don't produce warning output" },
    
    {"version", 'V', POPT_ARG_NONE, &opts.version_only, 0,
     "display version and copyright information and exit"},
    POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
  };
  poptContext optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = poptGetNextOpt (optCon)) != -1) {
    switch (opt) {
      
    case OP_SOURCE_AUTO:
    case OP_SOURCE_BIN: 
    case OP_SOURCE_CUE: 
    case OP_SOURCE_CDRDAO: 
    case OP_SOURCE_NRG: 
    case OP_SOURCE_DEVICE: 
      if (opts.source_image != INPUT_UNKNOWN) {
	report(stderr, "%s: another source type option given before.\n", 
		program_name);
	report(stderr, "%s: give only one source type option.\n", 
		program_name);
	break;
      } 

      /* For all input sources which are not a DEVICE, we need to make
	 a copy of the string; for a DEVICE the fill-out routine makes
	 the copy.
       */
      if (OP_SOURCE_DEVICE != opt) 
	source_name = strdup(psz_my_source);
      
      switch (opt) {
      case OP_SOURCE_BIN: 
	opts.source_image  = INPUT_BIN;
	break;
      case OP_SOURCE_CUE: 
	opts.source_image  = INPUT_CUE;
	break;
      case OP_SOURCE_CDRDAO: 
	opts.source_image  = INPUT_CDRDAO;
	break;
      case OP_SOURCE_NRG: 
	opts.source_image  = INPUT_NRG;
	break;
      case OP_SOURCE_AUTO:
	opts.source_image  = INPUT_AUTO;
	break;
      case OP_SOURCE_DEVICE: 
	opts.source_image  = INPUT_DEVICE;
	source_name = fillout_device_name(source_name);
	break;
      }
      break;
      
    default:
      poptFreeContext(optCon);
      return false;
    }
  }
  {
    const char *remaining_arg = poptGetArg(optCon);
    if ( remaining_arg != NULL) {
      if (opts.source_image != INPUT_UNKNOWN) {
	report(stderr, "%s: Source '%s' given as an argument of an option and as "
		 "unnamed option '%s'\n", 
		 program_name, psz_my_source, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
      }
      
      if (opts.source_image == INPUT_DEVICE)
	source_name = fillout_device_name(remaining_arg);
      else 
	source_name = strdup(remaining_arg);
      
      if ( (poptGetArgs(optCon)) != NULL) {
	report(stderr, "%s: Source specified in previously %s and %s\n", 
		 program_name, psz_my_source, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
	
      }
    }
  }
  
  poptFreeContext(optCon);
  return true;
}

/* ------------------------------------------------------------------------ */
/* CDDB                                                                     */

/* 
   Returns the sum of the decimal digits in a number. Eg. 1955 = 20
*/
static int
cddb_dec_digit_sum(int n)
{
  int ret=0;
  
  for (;;) {
    ret += n%10;
    n    = n/10;
    if (!n)
      return ret;
  }
}

/* Return the number of seconds (discarding frame portion) of an MSF */
static inline unsigned int
msf_seconds(msf_t *msf) 
{
  return cdio_from_bcd8(msf->m)*CDIO_CD_SECS_PER_MIN + cdio_from_bcd8(msf->s);
}

/* 
   Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
   consisting of the concatenation of 3 things:
      the sum of the decimal digits of sizes of all tracks, 
      the total length of the disk, and 
      the number of tracks.
*/
static unsigned long
cddb_discid(CdIo_t *p_cdio, int i_tracks)
{
  int i,t,n=0;
  msf_t start_msf;
  msf_t msf;
  
  for (i = 1; i <= i_tracks; i++) {
    cdio_get_track_msf(p_cdio, i, &msf);
    n += cddb_dec_digit_sum(msf_seconds(&msf));
  }

  cdio_get_track_msf(p_cdio, 1, &start_msf);
  cdio_get_track_msf(p_cdio, CDIO_CDROM_LEADOUT_TRACK, &msf);
  
  t = msf_seconds(&msf) - msf_seconds(&start_msf);
  
  return ((n % 0xff) << 24 | t << 8 | i_tracks);
}


/* CDIO logging routines */

static cdio_log_handler_t gl_default_cdio_log_handler = NULL;
#ifdef HAVE_CDDB
static cddb_log_handler_t gl_default_cddb_log_handler = NULL;
#endif
#ifdef HAVE_VCDINFO
static vcd_log_handler_t gl_default_vcd_log_handler = NULL;
#endif

static void 
_log_handler (cdio_log_level_t level, const char message[])
{
  if (level == CDIO_LOG_DEBUG && opts.debug_level < 2)
    return;

  if (level == CDIO_LOG_INFO  && opts.debug_level < 1)
    return;
  
  if (level == CDIO_LOG_WARN  && opts.silent)
    return;
  
  gl_default_cdio_log_handler (level, message);
}

static void 
print_cdtext_track_info(CdIo_t *p_cdio, track_t i_track, const char *psz_msg) {
   cdtext_t *p_cdtext = cdio_get_cdtext(p_cdio, i_track);

  if (NULL != p_cdtext) {
    cdtext_field_t i;
    
    printf("%s\n", psz_msg);
    
    for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
      if (p_cdtext->field[i]) {
	printf("\t%s: %s\n", cdtext_field2str(i), p_cdtext->field[i]);
      }
    }
  }
  cdtext_destroy(p_cdtext);
}
    
static void 
print_cdtext_info(CdIo_t *p_cdio, track_t i_tracks, track_t i_first_track) {
  track_t i_last_track = i_first_track+i_tracks;
  
  print_cdtext_track_info(p_cdio, 0, "\nCD-TEXT for Disc:");
  for ( ; i_first_track < i_last_track; i_first_track++ ) {
    char msg[50];
    sprintf(msg, "CD-TEXT for Track %d:", i_first_track);
    print_cdtext_track_info(p_cdio, i_first_track, msg);
  }
}

#ifdef HAVE_CDDB
static void 
print_cddb_info(CdIo_t *p_cdio, track_t i_tracks, track_t i_first_track) {
  int i, matches;
  
  cddb_conn_t *conn =  cddb_new();
  cddb_disc_t *disc = NULL;

  if (!conn) {
    report(stderr,  "%s: unable to initialize libcddb\n", program_name);
    goto cddb_destroy;
  }

  if (NULL == opts.cddb_email) 
    cddb_set_email_address(conn, "me@home");
  else 
    cddb_set_email_address(conn, opts.cddb_email);

  if (NULL == opts.cddb_server) 
    cddb_set_server_name(conn, "freedb.freedb.org");
  else 
    cddb_set_server_name(conn, opts.cddb_server);

  if (opts.cddb_timeout >= 0) 
    cddb_set_timeout(conn, opts.cddb_timeout);

  cddb_set_server_port(conn, opts.cddb_port);

  if (opts.cddb_http) 
    cddb_http_enable(conn);
  else 
    cddb_http_disable(conn);

  if (NULL != opts.cddb_cachedir) 
    cddb_cache_set_dir(conn, opts.cddb_cachedir);
    
  if (opts.cddb_disable_cache) 
    cddb_cache_disable(conn);
    
  disc = cddb_disc_new();
  if (!disc) {
    report(stderr, "%s: unable to create CDDB disc structure", program_name);
    goto cddb_destroy;
  }
  for(i = 0; i < i_tracks; i++) {
    cddb_track_t *t = cddb_track_new(); 
    t->frame_offset = cdio_get_track_lba(p_cdio, i+i_first_track);
    cddb_disc_add_track(disc, t);
  }
    
  disc->length = 
    cdio_get_track_lba(p_cdio, CDIO_CDROM_LEADOUT_TRACK) 
    / CDIO_CD_FRAMES_PER_SEC;

  if (!cddb_disc_calc_discid(disc)) {
    report(stderr, "%s: libcddb calc discid failed.\n", 
	    program_name);
    goto cddb_destroy;
  }

  matches = cddb_query(conn, disc);

  if (-1 == matches) 
  {
    sprintf(temp_msg, "%s: %s\n", program_name, cddb_error_str(cddb_errno(conn)));
	report(stdout, temp_msg);
  }
  else {
    sprintf(temp_msg, "%s: Found %d matches in CDDB\n", program_name, matches);
	report(stdout, temp_msg);
    for (i=1; i<=matches; i++) {
      cddb_read(conn, disc);
      cddb_disc_print(disc);
      cddb_query_next(conn, disc);
    }
  }
  
  cddb_disc_destroy(disc);
  cddb_destroy:
  cddb_destroy(conn);
}
#endif

#ifdef HAVE_VCDINFO
static void 
print_vcd_info(driver_id_t driver) {
  vcdinfo_open_return_t open_rc;
  vcdinfo_obj_t *p_vcd = NULL;
  open_rc = vcdinfo_open(&p_vcd, &source_name, driver, NULL);
  switch (open_rc) {
  case VCDINFO_OPEN_VCD: 
    if (vcdinfo_get_format_version (p_vcd) == VCD_TYPE_INVALID) {
      report(stderr, "VCD format detection failed");
      vcdinfo_close(p_vcd);
      return;
    }
    report (stdout, "Format      : %s\n", 
	    vcdinfo_get_format_version_str(p_vcd));
    report (stdout, "Album       : `%.16s'\n",  vcdinfo_get_album_id(p_vcd));
    report (stdout, "Volume count: %d\n",   vcdinfo_get_volume_count(p_vcd));
    report (stdout, "volume number: %d\n",  vcdinfo_get_volume_num(p_vcd));

    break;
  case VCDINFO_OPEN_ERROR:
    report( stderr, "Error in Video CD opening of %s\n", source_name );
    break;
  case VCDINFO_OPEN_OTHER:
    report( stderr, "Even though we thought this was a Video CD, "
	     " further inspection says it is not.\n");
    break;
  }
  if (p_vcd) vcdinfo_close(p_vcd);
    
}
#endif 

static void
print_iso9660_recurse (CdIo_t *p_cdio, const char pathname[], 
		       cdio_fs_anal_t fs, 
		       bool b_mode2)
{
  CdioList_t *entlist;
  CdioList_t *dirlist =  _cdio_list_new ();
  CdioListNode_t *entnode;
  uint8_t i_joliet_level;

  i_joliet_level = (opts.no_joliet) 
    ? 0
    : cdio_get_joliet_level(p_cdio);

  entlist = iso9660_fs_readdir (p_cdio, pathname, b_mode2);
    
  sprintf (temp_msg, "%s:\n", pathname);
  report(stdout, temp_msg);

  if (NULL == entlist) {
    report( stderr, "Error getting above directory information\n" );
    return;
  }

  /* Iterate over files in this directory */
  
  _CDIO_LIST_FOREACH (entnode, entlist)
    {
      iso9660_stat_t *statbuf = _cdio_list_node_data (entnode);
      char *iso_name = statbuf->filename;
      char _fullname[4096] = { 0, };
      char translated_name[MAX_ISONAME+1];
#define DATESTR_SIZE 30
      char date_str[DATESTR_SIZE];

      iso9660_name_translate_ext(iso_name, translated_name, i_joliet_level);
      
      snprintf (_fullname, sizeof (_fullname), "%s%s", pathname, 
		iso_name);
  
      strncat (_fullname, "/", sizeof (_fullname));

      if (statbuf->type == _STAT_DIR
          && strcmp (iso_name, ".") 
          && strcmp (iso_name, ".."))
        _cdio_list_append (dirlist, strdup (_fullname));

      if (fs & CDIO_FS_ANAL_XA) {
	sprintf (temp_msg, "  %c %s %d %d [fn %.2d] [LSN %6lu] ",
		 (statbuf->type == _STAT_DIR) ? 'd' : '-',
		 iso9660_get_xa_attr_str (statbuf->xa.attributes),
		 uint16_from_be (statbuf->xa.user_id),
		 uint16_from_be (statbuf->xa.group_id),
		 statbuf->xa.filenum,
		 (long unsigned int) statbuf->lsn);
    report(stdout, temp_msg);
	
	if (uint16_from_be(statbuf->xa.attributes) & XA_ATTR_MODE2FORM2) {
	  sprintf (temp_msg, "%9u (%9u)",
		  (unsigned int) statbuf->secsize * M2F2_SECTOR_SIZE,
		  (unsigned int) statbuf->size);
      report(stdout, temp_msg);
	} else {
	  printf ("%9u", (unsigned int) statbuf->size);
	}
      }
      strftime(date_str, DATESTR_SIZE, "%b %d %Y %H:%M ", &statbuf->tm);
      sprintf (temp_msg, " %s %s\n", date_str, translated_name);
      report(stdout, temp_msg);
    }

  _cdio_list_free (entlist, true);

  sprintf (temp_msg, "\n");
  report(stdout, temp_msg);

  /* Now recurse over the directories. */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);

      print_iso9660_recurse (p_cdio, _fullname, fs, b_mode2);
    }

  _cdio_list_free (dirlist, true);
}

static void
print_iso9660_fs (CdIo_t *p_cdio, cdio_fs_anal_t fs, 
		  track_format_t track_format)
{
  bool b_mode2 = false;
  iso_extension_mask_t iso_extension_mask = ISO_EXTENSION_ALL;

  if (fs & CDIO_FS_ANAL_XA) track_format = TRACK_FORMAT_XA;

  if (opts.no_joliet) {
    iso_extension_mask &= ~ISO_EXTENSION_JOLIET;
  }

  if ( !iso9660_fs_read_superblock(p_cdio, iso_extension_mask) ) 
    return;
  
  b_mode2 = ( TRACK_FORMAT_CDI == track_format 
	       || TRACK_FORMAT_XA == track_format );
  
  {
    sprintf (temp_msg, "ISO9660 filesystem\n");
    report(stdout, temp_msg);
    print_iso9660_recurse (p_cdio, "/", fs, b_mode2);
  }
}

#define print_vd_info(title, fn)		\
  psz_str = fn(&pvd);				\
  if (psz_str) {				\
    report(stdout, title ": %s\n", psz_str);	\
    free(psz_str);				\
    psz_str = NULL;				\
  }

static void
print_analysis(int ms_offset, cdio_iso_analysis_t cdio_iso_analysis, 
	       cdio_fs_anal_t fs, int first_data, unsigned int num_audio, 
	       track_t i_tracks, track_t i_first_track, 
	       track_format_t track_format, CdIo_t *p_cdio)
{
  int need_lf;
  
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_AUDIO:
    if (num_audio > 0) {
      sprintf(temp_msg, "Audio CD, CDDB disc ID is %08lx\n", 
	     cddb_discid(p_cdio, i_tracks));
      report(stdout, temp_msg);
#ifdef HAVE_CDDB
      if (!opts.no_cddb) print_cddb_info(p_cdio, i_tracks, i_first_track);
#endif      
      print_cdtext_info(p_cdio, i_tracks, i_first_track);
    }
    break;
  case CDIO_FS_ISO_9660:
    sprintf(temp_msg, "CD-ROM with ISO 9660 filesystem");
    report(stdout, temp_msg);
    if (fs & CDIO_FS_ANAL_JOLIET) {
      sprintf(temp_msg, " and joliet extension level %d", cdio_iso_analysis.joliet_level);
      report(stdout, temp_msg);
    }
    if (fs & CDIO_FS_ANAL_ROCKRIDGE)
	{
      sprintf(temp_msg, " and rockridge extensions");
      report(stdout, temp_msg);
	}
    sprintf(temp_msg, "\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_ISO_9660_INTERACTIVE:
    sprintf(temp_msg,"CD-ROM with CD-RTOS and ISO 9660 filesystem\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_HIGH_SIERRA:
    sprintf(temp_msg, "CD-ROM with High Sierra filesystem\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_INTERACTIVE:
    sprintf(temp_msg, "CD-Interactive%s\n", num_audio > 0 ? "/Ready" : "");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_HFS:
    sprintf(temp_msg, "CD-ROM with Macintosh HFS\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_ISO_HFS:
    sprintf(temp_msg, "CD-ROM with both Macintosh HFS and ISO 9660 filesystem\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_UFS:
    sprintf(temp_msg, "CD-ROM with Unix UFS\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_EXT2:
    sprintf(temp_msg, "CD-ROM with GNU/Linux EXT2 (native) filesystem\n");
    report(stdout, temp_msg);
	  break;
  case CDIO_FS_3DO:
    sprintf(temp_msg, "CD-ROM with Panasonic 3DO filesystem\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_UDFX:
    sprintf(temp_msg, "CD-ROM with UDFX filesystem\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_UNKNOWN:
    sprintf(temp_msg, "CD-ROM with unknown filesystem\n");
    report(stdout, temp_msg);
    break;
  case CDIO_FS_XISO:
    sprintf(temp_msg, "CD-ROM with Microsoft X-BOX XISO filesystem\n");
    report(stdout, temp_msg);
    break;
  }
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_ISO_9660:
  case CDIO_FS_ISO_9660_INTERACTIVE:
  case CDIO_FS_ISO_HFS:
  case CDIO_FS_ISO_UDF:
    sprintf(temp_msg,"ISO 9660: %i blocks, label `%.32s'\n",
	   cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);
    report(stdout, temp_msg);

    {
      iso9660_pvd_t pvd;

      if ( iso9660_fs_read_pvd(p_cdio, &pvd) ) {
	sprintf(temp_msg, "Application: %s\n", 
		iso9660_get_application_id(&pvd));
    report(stdout, temp_msg);
	sprintf(temp_msg, "Preparer   : %s\n", iso9660_get_preparer_id(&pvd));
    report(stdout, temp_msg);
	sprintf(temp_msg, "Publisher  : %s\n", iso9660_get_publisher_id(&pvd));
    report(stdout, temp_msg);
	sprintf(temp_msg, "System     : %s\n", iso9660_get_system_id(&pvd));
    report(stdout, temp_msg);
	sprintf(temp_msg, "Volume     : %s\n", iso9660_get_volume_id(&pvd));
    report(stdout, temp_msg);
	sprintf(temp_msg, "Volume Set : %s\n", iso9660_get_volumeset_id(&pvd));
    report(stdout, temp_msg);
      }
      
    }
    
    if (opts.print_iso9660) 
      print_iso9660_fs(p_cdio, fs, track_format);
    
    break;
  }
  
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_UDF:
  case CDIO_FS_ISO_UDF:
    report(stdout, "UDF: version %x.%2.2x\n",
	    cdio_iso_analysis.UDFVerMajor, cdio_iso_analysis.UDFVerMinor);
    report(stdout, temp_msg);
    break;
  default: ;
  }

  need_lf = 0;
  if (first_data == 1 && num_audio > 0) {
    need_lf =1;
	sprintf(temp_msg, "mixed mode CD   ");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_XA) {
    need_lf =1;
	sprintf(temp_msg, "XA sectors   ");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_MULTISESSION) {
    need_lf =1;
	sprintf(temp_msg, "Multisession, offset = %i   ", ms_offset);
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_HIDDEN_TRACK) {
    need_lf =1;
	sprintf(temp_msg, "Hidden Track   ");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_PHOTO_CD) {
    need_lf =1; 
	sprintf(temp_msg, "%sPhoto CD   ", 
		      num_audio > 0 ? " Portfolio " : "");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_CDTV) {
    need_lf =1;
	sprintf(temp_msg, "Commodore CDTV   ");
	report(stdout, temp_msg);
  }
  if (first_data > 1) {
    need_lf =1;
	sprintf(temp_msg, "CD-Plus/Extra   ");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_BOOTABLE) {
    need_lf =1;
	sprintf(temp_msg, "bootable CD   ");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_VIDEOCD && num_audio == 0) {
    need_lf =1;
	sprintf(temp_msg, "Video CD   ");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_SVCD) {
    need_lf =1;
	sprintf(temp_msg, "Super Video CD (SVCD) or Chaoji Video CD (CVD)");
	report(stdout, temp_msg);
  }
  if (fs & CDIO_FS_ANAL_CVD) {
    need_lf =1;
	sprintf(temp_msg, "Chaoji Video CD (CVD)");
	report(stdout, temp_msg);
  }
  if (need_lf) {
	  sprintf(temp_msg, "\n");
	  report(stdout, temp_msg);
  }
#ifdef HAVE_VCDINFO
  if (fs & (CDIO_FS_ANAL_VIDEOCD|CDIO_FS_ANAL_CVD|CDIO_FS_ANAL_SVCD)) 
    if (!opts.no_vcd) {
      sprintf(temp_msg, "\n");
	  report(stdout, temp_msg);
      print_vcd_info(cdio_get_driver_id(p_cdio));
    }
#endif    

}

/* Initialize global variables. */
static void 
init(void) 
{
  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);
#ifdef HAVE_CDDB
  

  gl_default_cddb_log_handler = 
    cddb_log_set_handler ((cddb_log_handler_t) _log_handler);
#endif

#ifdef HAVE_VCDINFO
  gl_default_vcd_log_handler =
    vcd_log_set_handler ((vcd_log_handler_t) _log_handler);
#endif

  /* Default option values. */
  opts.silent        = false;
  opts.list_drives   = false;
  opts.no_header     = false;
  opts.no_joliet     = false;
  opts.no_device     = 0;
  opts.no_disc_mode  = 0;
  opts.debug_level   = 0;
  opts.no_tracks     = 0;
  opts.print_iso9660 = 0;
#ifdef HAVE_CDDB
  opts.no_cddb       = 0;
  opts.cddb_port     = 8880;
  opts.cddb_http     = 0;
  opts.cddb_cachedir = NULL;
  opts.cddb_server   = NULL;
  opts.cddb_timeout = -1;
  opts.cddb_disable_cache = false;
#endif
#ifdef HAVE_VCDINFO
  opts.no_vcd        = 0;
#else
  opts.no_vcd        = 1;
#endif
  opts.no_ioctl      = 0;
  opts.no_analysis   = 0;
  opts.source_image  = INPUT_UNKNOWN;
  opts.access_mode   = NULL;
}

/* ------------------------------------------------------------------------ */

int
main(int argc, const char *argv[])
{

  CdIo_t                *p_cdio=NULL;
  cdio_fs_anal_t      fs=CDIO_FS_AUDIO;
  int i;
  lsn_t               start_track_lsn;      /* lsn of first track */
  lsn_t               data_start     =  0;  /* start of data area */
  int                 ms_offset      =  0;
  track_t             i_tracks       =  0;
  track_t             i_first_track  =  0;
  unsigned int        num_audio      =  0;  /* # of audio tracks */
  unsigned int        num_data       =  0;  /* # of data tracks */
  int                 first_data     = -1;  /* # of first data track */
  int                 first_audio    = -1;  /* # of first audio track */
  cdio_iso_analysis_t cdio_iso_analysis; 
  char                *media_catalog_number;  
  discmode_t          discmode = CDIO_DISC_MODE_NO_INFO;
      
  memset(&cdio_iso_analysis, 0, sizeof(cdio_iso_analysis));
  init();

#ifdef _XBOX
  /* argc and argv are both NULL, so just set some arbitrary values... */
  opts.silent        = false;
  opts.no_header     = false;
  opts.debug_level   = 0;
  opts.no_tracks     = 0;
  opts.no_ioctl      = 0;
  opts.no_analysis   = 0;
  opts.source_image  = IMAGE_UNKNOWN;
    
  source_name = strdup("\\\\.\\D:");

//  IoDeleteSymbolicLink(&DSymbolicLinkName);
//  IoCreateSymbolicLink(&DSymbolicLinkName, &DDeviceName);
//  IoCreateSymbolicLink(&FSymbolicLinkName, &FDeviceName);
//  IoCreateSymbolicLink(&ESymbolicLinkName, &EDeviceName);

  opts.source_image = IMAGE_DEVICE;

#else
  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);

  print_version(program_name, CDIO_VERSION, opts.no_header, opts.version_only);
#endif

  if (opts.debug_level == 3) {
    cdio_loglevel_default = CDIO_LOG_INFO;
#ifdef HAVE_VCDINFO
    vcd_loglevel_default = VCD_LOG_INFO;
#endif
  } else if (opts.debug_level >= 4) {
    cdio_loglevel_default = CDIO_LOG_DEBUG;
#ifdef HAVE_VCDINFO
    vcd_loglevel_default = VCD_LOG_DEBUG;
#endif
  }

  p_cdio = open_input(source_name, opts.source_image, opts.access_mode);

  if (p_cdio==NULL) {
    if (source_name) {
#ifdef _WIN32
	sprintf(temp_msg, "%s: Error in opening device driver for input %s.\n", 
		program_name, source_name);
	report(stderr, temp_msg);
	myexit(p_cdio, EXIT_FAILURE);		     
#else
      err_exit("%s: Error in opening device driver for input %s.\n", 
	       program_name, source_name);
#endif
    } else {
#ifdef _WIN32
	sprintf(temp_msg, "%s: Error in opening device driver for unspecified input.\n", 
		program_name);
	report(stderr, temp_msg);
	myexit(p_cdio, EXIT_FAILURE);		     
#else
      err_exit("%s: Error in opening device driver for unspecified input.\n", 
	       program_name);
#endif
    }
    
  } 

  if (source_name==NULL) {
    source_name=strdup(cdio_get_arg(p_cdio, "source"));
    if (NULL == source_name) {
#ifdef _WIN32
	sprintf(temp_msg, "%s: No input device given/found\n", 
		program_name);
	report(stderr, temp_msg);
	myexit(p_cdio, EXIT_FAILURE);		     
#else
      err_exit("%s: No input device given/found\n", program_name);
#endif
    }
  } 

  if (0 == opts.silent) {
    sprintf(temp_msg, "CD location   : %s\n",   source_name);
	report(stdout, temp_msg);
    sprintf(temp_msg, "CD driver name: %s\n",   cdio_get_driver_name(p_cdio));
	report(stdout, temp_msg);
    sprintf(temp_msg, "   access mode: %s\n\n", cdio_get_arg(p_cdio, "access-mode"));
	report(stdout, temp_msg);
  } 

  if (0 == opts.no_device) {
    cdio_drive_read_cap_t  i_read_cap;
    cdio_drive_write_cap_t i_write_cap;
    cdio_drive_misc_cap_t  i_misc_cap;
    cdio_hwinfo_t          hwinfo;
    if (cdio_get_hwinfo(p_cdio, &hwinfo)) {
      sprintf(temp_msg, "%-28s: %s\n%-28s: %s\n%-28s: %s\n",
	     "Vendor"  , hwinfo.psz_vendor, 
	     "Model"   , hwinfo.psz_model, 
	     "Revision", hwinfo.psz_revision);
	  report(stdout, temp_msg);
    }
    cdio_get_drive_cap(p_cdio, &i_read_cap, &i_write_cap, &i_misc_cap);
    print_drive_capabilities(i_read_cap, i_write_cap, i_misc_cap);
  }

  if (opts.list_drives) {
    driver_id_t driver_id = DRIVER_DEVICE;
    char ** device_list = cdio_get_devices_ret(&driver_id);
    char ** d = device_list;

    sprintf(temp_msg, "list of devices found:\n");
	report(stdout, temp_msg);
    if (NULL != d) {
      for ( ; *d != NULL ; d++ ) {
	CdIo_t *p_cdio = cdio_open(source_name, driver_id); 
	cdio_hwinfo_t hwinfo;
	sprintf(temp_msg, "Drive %s\n", *d);
	report(stdout, temp_msg);
	if (scsi_mmc_get_hwinfo(p_cdio, &hwinfo)) {
	  sprintf(temp_msg, "%-8s: %s\n%-8s: %s\n%-8s: %s\n",
		 "Vendor"  , hwinfo.psz_vendor, 
		 "Model"   , hwinfo.psz_model, 
		 "Revision", hwinfo.psz_revision);
	  report(stdout, temp_msg);
	}
	if (p_cdio) cdio_destroy(p_cdio);
      }
    }
    cdio_free_device_list(device_list);
    if (device_list) free(device_list);
  }

  report(stdout, STRONG "\n");
  

  discmode = cdio_get_discmode(p_cdio);
  if ( 0 == opts.no_disc_mode ) {
    sprintf(temp_msg, "Disc mode is listed as: %s\n", 
	   discmode2str[discmode]);
    report(stdout, temp_msg);
  }

  if (discmode_is_dvd(discmode) && !opts.show_dvd) {
    sprintf(temp_msg, "No further information currently given for DVDs.\n");
    report(stdout, temp_msg);
    sprintf(temp_msg, "Use --dvd to override.\n");
    report(stdout, temp_msg);
    myexit(p_cdio, EXIT_SUCCESS);
  }
  
  i_first_track = cdio_get_first_track_num(p_cdio);

  if (CDIO_INVALID_TRACK == i_first_track) {
#ifdef _WIN32
	sprintf(temp_msg, "Can't get first track number. I give up%s.\n", 
		"");
    report(stderr, temp_msg);
	myexit(p_cdio, EXIT_FAILURE);		     
#else
    err_exit("Can't get first track number. I give up%s.\n", "");
#endif
  }

  i_tracks = cdio_get_num_tracks(p_cdio);

  if (CDIO_INVALID_TRACK == i_tracks) {
#ifdef _WIN32
	sprintf(temp_msg, "Can't get number of tracks. I give up.%s\n", 
		"");
    report(stderr, temp_msg);
	myexit(p_cdio, EXIT_FAILURE);		     
#else
    err_exit("Can't get number of tracks. I give up.%s\n", "");
#endif
  }

  if (!opts.no_tracks) {
    sprintf(temp_msg, "CD-ROM Track List (%i - %i)\n" NORMAL, 
	   i_first_track, i_tracks);
    report(stdout, temp_msg);

    sprintf(temp_msg, "  #: MSF       LSN     Type  Green?\n");
    report(stdout, temp_msg);
    if ( CDIO_DISC_MODE_CD_DA == discmode 
	 || CDIO_DISC_MODE_CD_MIXED == discmode )
      report(" Channels Premphasis?");
    report("\n");
  }

  start_track_lsn = cdio_get_track_lsn(p_cdio, i_first_track);

  /* Read and possibly print track information. */
  for (i = i_first_track; i <= CDIO_CDROM_LEADOUT_TRACK; i++) {
    msf_t msf;
    char *psz_msf;
    track_format_t track_format;

    if (!cdio_get_track_msf(p_cdio, i, &msf)) {
#ifdef _WIN32
	sprintf(temp_msg, "cdio_track_msf for track %i failed, I give up.\n", 
		i);
    report(stderr, temp_msg);
	myexit(p_cdio, EXIT_FAILURE);		     
#else
      err_exit("cdio_track_msf for track %i failed, I give up.\n", i);
#endif
    }

    track_format = cdio_get_track_format(p_cdio, i);
    psz_msf = cdio_msf_to_str(&msf);
    if (i == CDIO_CDROM_LEADOUT_TRACK) {
	  if (!opts.no_tracks) {
	lsn_t lsn= cdio_msf_to_lsn(&msf);
        long unsigned int i_bytes_raw = lsn * CDIO_CD_FRAMESIZE_RAW;
        long unsigned int i_bytes_formatted = lsn - start_track_lsn;

	printf( "%3d: %8s  %06lu leadout ", (int) i, psz_msf,
		(long unsigned int) lsn );

	switch (discmode) {
        case CDIO_DISC_MODE_DVD_ROM:
        case CDIO_DISC_MODE_DVD_RAM:
        case CDIO_DISC_MODE_DVD_R:
        case CDIO_DISC_MODE_DVD_RW:
        case CDIO_DISC_MODE_DVD_PR:
        case CDIO_DISC_MODE_DVD_PRW:
        case CDIO_DISC_MODE_DVD_OTHER:
	case CDIO_DISC_MODE_CD_DATA:
	  i_bytes_formatted *= CDIO_CD_FRAMESIZE;
	  break;
	case CDIO_DISC_MODE_CD_DA:
	  i_bytes_formatted *= CDIO_CD_FRAMESIZE_RAW;
	  break;
	case CDIO_DISC_MODE_CD_XA:
	case CDIO_DISC_MODE_CD_MIXED:
	  i_bytes_formatted *= CDIO_CD_FRAMESIZE_RAW0;
	  break;
	default:
	  i_bytes_formatted *= CDIO_CD_FRAMESIZE_RAW;
	}
	
	if (i_bytes_raw < 1024) 
	  printf( "(%lu bytes", i_bytes_raw );
	if (i_bytes_raw < 1024 * 1024) 
	  printf( "(%lu KB", i_bytes_raw / 1024 );
	else 
	  printf( "(%lu MB", i_bytes_raw / (1024 * 1024) );

	printf(" raw, ");
	if (i_bytes_formatted < 1024) 
	  printf( "%lu bytes", i_bytes_formatted );
	if (i_bytes_formatted < 1024 * 1024) 
	  printf( "%lu KB", i_bytes_formatted / 1024 );
	else 
	  printf( "%lu MB", i_bytes_formatted / (1024 * 1024) );

	printf(" formatted)\n");

      }
      free(psz_msf);
      break;
    } else if (!opts.no_tracks) {
      sprintf(temp_msg, "%3d: %8s  %06lu  %-5s %s\n", (int) i, psz_msf,
	     (long unsigned int) cdio_msf_to_lsn(&msf),
	     track_format2str[cdio_get_track_format(p_cdio, i)],
	     cdio_get_track_green(p_cdio, i)? "true" : "false");
	  report(stdout, temp_msg);

      switch (cdio_get_track_copy_permit(p_cdio, i)) {
      case CDIO_TRACK_FLAG_FALSE:
	psz="no";
	break;
      case CDIO_TRACK_FLAG_TRUE:
	psz="yes";
	break;
      case CDIO_TRACK_FLAG_UNKNOWN:
	psz="?";
	break;
      case CDIO_TRACK_FLAG_ERROR:
      default:
	psz="error";
	break;
      }
      printf("%-5s", psz);
	
      if (TRACK_FORMAT_AUDIO == track_format) {
	const int i_channels = cdio_get_track_channels(p_cdio, i);
	switch (cdio_get_track_preemphasis(p_cdio, i)) {
	case CDIO_TRACK_FLAG_FALSE:
	  psz="no";
	  break;
	case CDIO_TRACK_FLAG_TRUE:
	  psz="yes";
	  break;
	case CDIO_TRACK_FLAG_UNKNOWN:
	  psz="?";
	  break;
	case CDIO_TRACK_FLAG_ERROR:
	default:
	  psz="error";
	  break;
	}
	if (i_channels == -2) 
	  printf(" %-8s", "unknown");
	else if (i_channels == -1) 
	  printf(" %-8s", "error");
	else 
	  printf(" %-8d", i_channels);
	printf( " %s", psz);
      }
      
      printf( "\n" );
      
    }
    free(psz_msf);
    
    if (TRACK_FORMAT_AUDIO == track_format) {
      num_audio++;
      if (-1 == first_audio) first_audio = i;
    } else {
      num_data++;
      if (-1 == first_data)  first_data = i;
    }
    /* skip to leadout? */
    if (i == i_tracks) i = CDIO_CDROM_LEADOUT_TRACK-1;
  }

  if (cdio_is_discmode_cdrom(discmode)) {
    /* get and print MCN */
    media_catalog_number = cdio_get_mcn(p_cdio);
  
    sprintf(temp_msg, "Media Catalog Number (MCN): "); 
	report(stdout, temp_msg);
	fflush(stdout);
	if (NULL == media_catalog_number) {
      sprintf(temp_msg, "not available\n");
	  report(stdout, temp_msg);
	}
    else {
      sprintf(temp_msg, "%s\n", media_catalog_number);
	  report(stdout, temp_msg);
      free(media_catalog_number);
    }
  }
  
  
    
#if CDIO_IOCTL_FINISHED
  if (!opts.no_ioctl) {
    report(stdout, "What ioctl's report...\n");
  
#ifdef CDROMMULTISESSION
    /* get multisession */
    sprintf(temp_msg, "multisession: "); 
	report(stdout, temp_msg);
	fflush(stdout);
    ms.addr_format = CDROM_LBA;
	if (ioctl(filehandle,CDROMMULTISESSION,&ms)) {
      sprintf(temp_msg, "FAILED\n");
	  report(stdout, temp_msg);
	}
	else {
      sprintf(temp_msg, "%d%s\n",ms.addr.lba,ms.xa_flag?" XA":"");
	  report(stdout, temp_msg);
	}
#endif 
    
#ifdef CDROMSUBCHNL
    /* get audio status from subchnl */
    sprintf(temp_msg, "audio status: "); 
	report(stdout, temp_msg);
	fflush(stdout);
    sub.cdsc_format = CDROM_MSF;
	if (ioctl(filehandle,CDROMSUBCHNL,&sub)) {
      sprintf(temp_msg, "FAILED\n");
	  report(stdout, temp_msg);
	}
    else {
      switch (sub.cdsc_audiostatus) {
      case CDROM_AUDIO_INVALID:    
		  sprintf(temp_msg, "invalid\n");   
		  report(stdout, temp_msg);
		  break;
      case CDROM_AUDIO_PLAY:       
		  sprintf(temp_msg, "playing");     
		  report(stdout, temp_msg);
		  break;
      case CDROM_AUDIO_PAUSED:     
		  sprintf(temp_msg, "paused");      
		  report(stdout, temp_msg);
		  break;
      case CDROM_AUDIO_COMPLETED:  
		  sprintf(temp_msg, "completed\n"); 
		  report(stdout, temp_msg);
		  break;
      case CDROM_AUDIO_ERROR:      
		  sprintf(temp_msg, "error\n");     
		  report(stdout, temp_msg);
		  break;
      case CDROM_AUDIO_NO_STATUS:  
		  sprintf(temp_msg, "no status\n"); 
		  report(stdout, temp_msg);
		  break;
      default:                     
		  sprintf(temp_msg, "Oops: unknown\n");
		  report(stdout, temp_msg);
      }
      if (sub.cdsc_audiostatus == CDROM_AUDIO_PLAY ||
	  sub.cdsc_audiostatus == CDROM_AUDIO_PAUSED) {
		sprintf(temp_msg, " at: %02d:%02d abs / %02d:%02d track %d\n",
	       sub.cdsc_absaddr.msf.minute,
	       sub.cdsc_absaddr.msf.second,
	       sub.cdsc_reladdr.msf.minute,
	       sub.cdsc_reladdr.msf.second,
	       sub.cdsc_trk);
	    report(stdout, temp_msg);
      }
    }
#endif  /* CDROMSUBCHNL */
  }
#endif /*CDIO_IOCTL_FINISHED*/
  
  if (!opts.no_analysis) {
    report(stdout, STRONG "CD Analysis Report\n" NORMAL);
    
    /* try to find out what sort of CD we have */
    if (0 == num_data) {
      /* no data track, may be a "real" audio CD or hidden track CD */
      
      msf_t msf;
      cdio_get_track_msf(p_cdio, i_first_track, &msf);
      
      /* CD-I/Ready says start_track_lsn <= 30*75 then CDDA */
      if (start_track_lsn > 100 /* 100 is just a guess */) {
	fs = cdio_guess_cd_type(p_cdio, 0, 1, &cdio_iso_analysis);
	if ((CDIO_FSTYPE(fs)) != CDIO_FS_UNKNOWN)
	  fs |= CDIO_FS_ANAL_HIDDEN_TRACK;
	else {
	  fs &= ~CDIO_FS_MASK; /* del filesystem info */
	  sprintf(temp_msg, "Oops: %lu unused sectors at start, "
		 "but hidden track check failed.\n",
		 (long unsigned int) start_track_lsn);
	  report(stdout, temp_msg);
	}
      }
      print_analysis(ms_offset, cdio_iso_analysis, fs, first_data, num_audio,
		     i_tracks, i_first_track, 
		     cdio_get_track_format(p_cdio, 1), p_cdio);
    } else {
      /* we have data track(s) */
      int j;

      for (j = 2, i = first_data; i <= i_tracks; i++) {
	msf_t msf;
	track_format_t track_format = cdio_get_track_format(p_cdio, i);
      
	cdio_get_track_msf(p_cdio, i, &msf);

	switch ( track_format ) {
	case TRACK_FORMAT_AUDIO:
	case TRACK_FORMAT_ERROR:
	  break;
	case TRACK_FORMAT_CDI:
	case TRACK_FORMAT_XA:
	case TRACK_FORMAT_DATA: 
	case TRACK_FORMAT_PSX: 
	  ;
	}
	
	start_track_lsn = (i == 1) ? 0 : cdio_msf_to_lsn(&msf);
	
	/* save the start of the data area */
	if (i == first_data) 
	  data_start = start_track_lsn;
	
	/* skip tracks which belong to the current walked session */
	if (start_track_lsn < data_start + cdio_iso_analysis.isofs_size)
	  continue;
	
	fs = cdio_guess_cd_type(p_cdio, start_track_lsn, i, 
				&cdio_iso_analysis);

	if (i > 1) {
	  /* track is beyond last session -> new session found */
	  ms_offset = start_track_lsn;
	  sprintf(temp_msg, "session #%d starts at track %2i, LSN: %lu,"
		 " ISO 9660 blocks: %6i\n",
		 j++, i, (unsigned long int) start_track_lsn, 
		 cdio_iso_analysis.isofs_size);
	  report(stdout, temp_msg);
	  sprintf(temp_msg, "ISO 9660: %i blocks, label `%.32s'\n",
		 cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);
	  report(stdout, temp_msg);
	  fs |= CDIO_FS_ANAL_MULTISESSION;
	} else {
	  print_analysis(ms_offset, cdio_iso_analysis, fs, first_data, 
			 num_audio, i_tracks, i_first_track, 
			 track_format, p_cdio);
	}

	if ( !(CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660 ||
	       CDIO_FSTYPE(fs) == CDIO_FS_ISO_HFS  ||
	       /* CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660_INTERACTIVE) 
		  && (fs & XA))) */
	       CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660_INTERACTIVE) )
	  /* no method for non-ISO9660 multisessions */
	  break;
      }
    }
  }

  myexit(p_cdio, EXIT_SUCCESS);
  /* Not reached:*/
  return(EXIT_SUCCESS);
}
