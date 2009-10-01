/*
    $Id: cd-info.c,v 1.152 2008/01/09 04:26:24 rocky Exp $

    Copyright (C) 2003, 2004, 2005, 2007, 2008 Rocky Bernstein <rocky@gnu.org>
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
#include "cddb.h"
#include "getopt.h"
#include <stdarg.h>

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
#include <cdio/mmc.h>
#include <cdio/audio.h>

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
struct opts_s
{
  int            no_tracks;
  int            no_ioctl;
  int            no_analysis;
  char          *access_mode; /* Access method driver should use for control */
  int            no_cddb;     /* If set the below are meaningless. */
  int            no_vcd;
  int            show_dvd;
  int            no_device;
  int            no_disc_mode;
  uint32_t       debug_level;
  int            version_only;
  int            silent;
  int            no_header;
  int            no_joliet;
  int            no_xa;
  int            no_rock_ridge;
  int            print_iso9660;
  int            list_drives;
  source_image_t source_image;
} opts;
     
/* Configuration option codes */
enum {
  OP_HANDLED = 0,
  
  OP_SOURCE_UNDEF,
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_CDRDAO,
  OP_SOURCE_NRG ,
  OP_SOURCE_DEVICE,

  OP_CDDB_SERVER,
  OP_CDDB_CACHE,
  OP_CDDB_EMAIL,
  OP_CDDB_NOCACHE,
  OP_CDDB_TIMEOUT,

  OP_USAGE,

  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

char *temp_str;

/* Parse source options. */
static void
parse_source(int opt)
{
  /* NOTE: The libpopt version made use of an extra temporary
     variable (psz_my_source) for all sources _except_ devices.
     This distinction seemed to serve no purpose.
  */

  if (opts.source_image != INPUT_UNKNOWN) {
    report(stderr, "%s: another source type option given before.\n", 
	   program_name);
    report(stderr, "%s: give only one source type option.\n", 
	   program_name);
    return;
  } 

  /* For all input sources which are not a DEVICE, we need to make
     a copy of the string; for a DEVICE the fill-out routine makes
     the copy.
  */
  if (OP_SOURCE_DEVICE != opt) 
    if (optarg != NULL) source_name = strdup(optarg);
  
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
    if (optarg != NULL) source_name = fillout_device_name(optarg);
    break;
  }
}


/* Parse all options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt; /* used for argument parsing */

  const char* helpText =
    "Usage: %s [OPTION...]\n"
    "  -a, --access-mode=STRING        Set CD access method\n"
    "  -d, --debug=INT                 Set debugging to LEVEL\n"
    "  -T, --no-tracks                 Don't show track information\n"
    "  -A, --no-analyze                Don't filesystem analysis\n"
#ifdef HAVE_CDDB
    "  --no-cddb                       Don't look up audio CDDB information\n"
    "                                  or print it\n"
    "  -P, --cddb-port=INT             CDDB port number to use (default 8880)\n"
    "  -H, --cddb-http                 Lookup CDDB via HTTP proxy (default no\n"
    "                                  proxy)\n"
    "  --cddb-server=STRING            CDDB server to contact for information\n"
    "                                  (default: freedb.freedb.org)\n"
    "  --cddb-cache=STRING             Location of CDDB cache directory\n"
    "                                  (default ~/.cddbclient)\n"
    "  --cddb-email=STRING             Email address to give CDDB server\n"
    "                                  (default me@home)\n"
    "  --no-cddb-cache                 Disable caching of CDDB entries\n"
    "                                  locally (default caches)\n"
    "  --cddb-timeout=INT              CDDB timeout value in seconds\n"
    "                                  (default 10 seconds)\n"
#else
    "  --no-cddb                       Does nothing since this program is not\n"
    "  -P, --cddb-port=INT             CDDB-enabled\n"
    "  -H, --cddb-http\n"
    "  --cddb-server=STRING\n"
    "  --cddb-cache=STRING\n"
    "  --cddb-email=STRING\n"
    "  --no-cddb-cache\n"
    "  --cddb-timeout=INT\n"
#endif
    "  --no-device-info                Don't show device info, just CD info\n"
    "  --no-disc-mode                  Don't show disc-mode info\n"
    "  --dvd                           Attempt to give DVD information if a DVD is\n"
    "                                  found.\n"
#ifdef HAVE_VCDINFO
    "  -v, --no-vcd                    Don't look up Video CD information\n"
#else
    "  -v, --no-vcd                    Don't look up Video CD information - for\n"
    "                                  this build, this is always set\n"
#endif
    "  -I, --no-ioctl                  Don't show ioctl() information\n"
    "  -b, --bin-file[=FILE]           set \"bin\" CD-ROM disk image file as source\n"
    "  -c, --cue-file[=FILE]           set \"cue\" CD-ROM disk image file as source\n"
    "  -N, --nrg-file[=FILE]           set Nero CD-ROM disk image file as source\n"
    "  -t, --toc-file[=FILE]           set cdrdao CD-ROM disk image file as source\n"
    "  -i, --input[=FILE]              set source and determine if \"bin\" image or\n"
    "                                  device\n"
    "  --iso9660                       print directory contents of any ISO-9660\n"
    "                                  filesystems\n"
    "  -C, --cdrom-device[=DEVICE]     set CD-ROM device as source\n"
    "  -l, --list-drives               Give a list of CD-drives\n"
    "  --no-header                     Don't display header and copyright (for\n"
    "                                  regression testing)\n"
#ifdef HAVE_JOLIET
    "  --no-joliet                     Don't use Joliet extensions\n"
#endif
    "  --no-rock-ridge                 Don't use Rock-Ridge-extension information\n"
    "  --no-xa                         Don't use XA-extension information\n"
    "  -q, --quiet                     Don't produce warning output\n"
    "  -V, --version                   display version and copyright information\n"
    "                                  and exit\n"
    "\n"
    "Help options:\n"
    "  -?, --help                      Show this help message\n"
    "  --usage                         Display brief usage message\n";
  
  const char* usageText = 
    "Usage: %s [-a|--access-mode STRING] [-d|--debug INT] [-T|--no-tracks]\n"
    "        [-A|--no-analyze] [--no-cddb] [-P|--cddb-port INT] [-H|--cddb-http]\n"
    "        [--cddb-server=STRING] [--cddb-cache=STRING] [--cddb-email=STRING]\n"
    "        [--no-cddb-cache] [--cddb-timeout=INT] [--no-device-info]\n"
    "        [--no-disc-mode] [--dvd] [-v|--no-vcd] [-I|--no-ioctl]\n"
    "        [-b|--bin-file FILE] [-c|--cue-file FILE] [-N|--nrg-file FILE]\n"
    "        [-t|--toc-file FILE] [-i|--input FILE] [--iso9660]\n"
    "        [-C|--cdrom-device DEVICE] [-l|--list-drives] [--no-header]\n"
    "        [--no-joliet] [--no-rock-ridge] [--no-xa] [-q|--quiet] [-V|--version]\n"
    "        [-?|--help] [--usage]\n";

  const char* optionsString = "a:d:TAP:HvIb::c::N::t::i::C::lqV?";
  struct option optionsTable[] = {
    {"access-mode", required_argument, NULL, 'a'},
    {"debug", required_argument, NULL, 'd' },
    {"no-tracks", no_argument, NULL, 'T' },
    {"no-analyze", no_argument, NULL, 'A' },
    {"no-cddb", no_argument, &opts.no_cddb, 1 },
    {"cddb-port", required_argument, NULL, 'P' },
    {"cddb-http", no_argument, NULL, 'H' },
    {"cddb-server", required_argument, NULL, OP_CDDB_SERVER },
    {"cddb-cache", required_argument, NULL, OP_CDDB_CACHE },
    {"cddb-email", required_argument, NULL, OP_CDDB_EMAIL },
    {"no-cddb-cache", no_argument, NULL, OP_CDDB_NOCACHE },
    {"cddb-timeout", required_argument, NULL, OP_CDDB_TIMEOUT },
    {"no-device-info", no_argument, &opts.no_device, 1 },
    {"no-disc-mode", no_argument, &opts.no_disc_mode, 1 },
    {"dvd", no_argument, &opts.show_dvd, 1 },
    {"no-vcd", no_argument, NULL, 'v' },
    {"no-ioctl", no_argument, NULL, 'I' },
    {"bin-file", optional_argument, NULL, 'b' },
    {"cue-file", optional_argument, NULL, 'c' },
    {"nrg-file", optional_argument, NULL, 'N' },
    {"toc-file", optional_argument, NULL, 't' },
    {"input", optional_argument, NULL, 'i' },
    {"iso9660", no_argument, &opts.print_iso9660, 1 },
    {"cdrom-device", optional_argument, NULL, 'C' },
    {"list-drives", no_argument, NULL, 'l' },
    {"no-header", no_argument, &opts.no_header, 1 }, 
#ifdef HAVE_JOLIET    
    {"no-joliet", no_argument, &opts.no_joliet, 1 },
#endif /*HAVE_JOLIET*/
    {"no-rock-ridge", no_argument, &opts.no_rock_ridge, 1 },
    {"no-xa", no_argument, &opts.no_xa, 1 },
    {"quiet", no_argument, NULL, 'q' },
    {"version", no_argument, NULL, 'V' },

    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OP_USAGE },
    { NULL, 0, NULL, 0 }
  };

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0) {
    switch (opt) {
    case 'a': opts.access_mode = strdup(optarg); break;
    case 'd': opts.debug_level = atoi(optarg); break;
    case 'T': opts.no_tracks = 1; break;
    case 'A': opts.no_analysis = 1; break;
    case 'P': cddb_opts.port = atoi(optarg); break;
    case 'H': cddb_opts.http = 1; break;
    case OP_CDDB_SERVER: cddb_opts.server = strdup(optarg); break;
    case OP_CDDB_CACHE: cddb_opts.cachedir = strdup(optarg); break;
    case OP_CDDB_EMAIL: cddb_opts.email = strdup(optarg); break;
    case OP_CDDB_NOCACHE: cddb_opts.disable_cache = 1; break;
    case OP_CDDB_TIMEOUT: cddb_opts.timeout = atoi(optarg); break;
    case 'v': opts.no_vcd = 1; break;
    case 'I': opts.no_ioctl = 1; break;
    case 'b': parse_source(OP_SOURCE_BIN); break;
    case 'c': parse_source(OP_SOURCE_CUE); break;
    case 'N': parse_source(OP_SOURCE_NRG); break;
    case 't': parse_source(OP_SOURCE_CDRDAO); break;
    case 'i': parse_source(OP_SOURCE_AUTO); break;
    case 'C': parse_source(OP_SOURCE_DEVICE); break;
    case 'l': opts.list_drives = 1; break;
    case 'q': opts.silent = 1; break;
    case 'V': opts.version_only = 1; break;

    case '?':
      fprintf(stdout, helpText, program_name);
      free(program_name);
      exit(EXIT_INFO);
      break;

    case OP_USAGE:
      fprintf(stderr, usageText, program_name);
      free(program_name);
      exit(EXIT_FAILURE);
      break;

    case OP_HANDLED:
      break;
    }
  }

  if (optind < argc) {
    const char *remaining_arg = argv[optind++];

    if (source_name != NULL) {
      report(stderr, "%s: Source '%s' given as an argument of an option and as "
	     "unnamed option '%s'\n", 
	     program_name, source_name, remaining_arg);
      free(program_name);
      exit (EXIT_FAILURE);
    }

    if (opts.source_image == INPUT_DEVICE)
      source_name = fillout_device_name(remaining_arg);
    else 
      source_name = strdup(remaining_arg);

    if (optind < argc) {
      report(stderr, "%s: Source specified in previously %s and %s\n", 
	     program_name, source_name, remaining_arg);
      free(program_name);
      exit (EXIT_FAILURE);
    }
  }

  return true;
}


/* CDIO logging routines */

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

#ifdef HAVE_CDDB
static void 
_cddb_log_handler (cddb_log_level_t level, const char message[])
{
  /* CDDB errors should not be considered fatal. */
  if (level == CDDB_LOG_ERROR)
    level = CDIO_LOG_WARN;

  /* Might consider doing some sort of CDDB to cdio to log level conversion,
     but right now it's a no op. */
  
  _log_handler(level, message);
}
#endif

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
cddb_errmsg(const char *msg)
{
  report(stderr, "%s: %s\n", program_name, msg);
}


static void 
print_cddb_info(CdIo_t *p_cdio, track_t i_tracks, track_t i_first_track) 
{
  int i, i_cddb_matches = 0;
  
  cddb_conn_t *p_conn = NULL;
  cddb_disc_t *p_cddb_disc = NULL;

  if ( init_cddb(p_cdio, &p_conn, &p_cddb_disc, cddb_errmsg, i_first_track, 
		 i_tracks, &i_cddb_matches) ) {

    if (-1 == i_cddb_matches) 
      printf("%s: %s\n", program_name, cddb_error_str(cddb_errno(p_conn)));
    else {
      printf("%s: Found %d matches in CDDB\n", program_name, i_cddb_matches);
      for (i=1; i<=i_cddb_matches; i++) {
	cddb_disc_print(p_cddb_disc);
	cddb_query_next(p_conn, p_cddb_disc);
	if (i != i_cddb_matches) cddb_read(p_conn, p_cddb_disc);
      }
    }
  
    cddb_disc_destroy(p_cddb_disc);
    cddb_destroy(p_conn);
    libcddb_shutdown();
  }
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
    report (stdout, "Format       : %s\n", 
	    vcdinfo_get_format_version_str(p_vcd));
    report (stdout, "Album        : `%.16s'\n",  vcdinfo_get_album_id(p_vcd));
    report (stdout, "Volume count : %d\n",   vcdinfo_get_volume_count(p_vcd));
    report (stdout, "volume number: %d\n",   vcdinfo_get_volume_num(p_vcd));

    break;
  case VCDINFO_OPEN_ERROR:
    report( stderr, "Error in Video CD opening of %s\n", source_name );
    break;
  case VCDINFO_OPEN_OTHER:
    report( stderr, "Even though we thought this was a Video CD, "
	    " further inspection says it is not.\n" );
    break;
  }
  if (p_vcd) vcdinfo_close(p_vcd);
    
}
#endif 

static void
print_iso9660_recurse (CdIo_t *p_cdio, const char pathname[], 
		       cdio_fs_anal_t fs)
{
  CdioList_t *p_entlist;
  CdioList_t *p_dirlist =  _cdio_list_new ();
  CdioListNode_t *entnode;
  uint8_t i_joliet_level;
  char *translated_name = (char *) malloc(4096);
  size_t translated_name_size = 4096;

  i_joliet_level = (opts.no_joliet) 
    ? 0
    : cdio_get_joliet_level(p_cdio);

  p_entlist = iso9660_fs_readdir (p_cdio, pathname, false);
    
  printf ("%s:\n", pathname);

  if (NULL == p_entlist) {
    report( stderr, "Error getting above directory information\n" );
    return;
  }

  /* Iterate over files in this directory */
  
  _CDIO_LIST_FOREACH (entnode, p_entlist)
    {
      iso9660_stat_t *p_statbuf = _cdio_list_node_data (entnode);
      char *psz_iso_name = p_statbuf->filename;
      char _fullname[4096] = { 0, };
       if (strlen(psz_iso_name) >= translated_name_size) {
         translated_name_size = strlen(psz_iso_name)+1;
         free(translated_name);
         translated_name = (char *) malloc(translated_name_size);
         if (!translated_name) {
           report( stderr, "Error allocating memory\n" );
           return;
         }
       }

      if (yep != p_statbuf->rr.b3_rock || 1 == opts.no_rock_ridge) {
	iso9660_name_translate_ext(psz_iso_name, translated_name, 
				   i_joliet_level);
      }
  
      
      snprintf (_fullname, sizeof (_fullname), "%s%s", pathname, 
		psz_iso_name);
  
      strncat (_fullname, "/", sizeof (_fullname));

      if (p_statbuf->type == _STAT_DIR
          && strcmp (psz_iso_name, ".") 
          && strcmp (psz_iso_name, ".."))
        _cdio_list_append (p_dirlist, strdup (_fullname));

      print_fs_attrs(p_statbuf, 0 == opts.no_rock_ridge, fs & CDIO_FS_ANAL_XA, 
		     psz_iso_name, translated_name);
      if (p_statbuf->rr.i_symlink) {
	free(p_statbuf->rr.psz_symlink);
	p_statbuf->rr.i_symlink = 0;
      }
    }
    free (translated_name);

  _cdio_list_free (p_entlist, true);

  printf ("\n");

  /* Now recurse over the directories. */

  _CDIO_LIST_FOREACH (entnode, p_dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);

      print_iso9660_recurse (p_cdio, _fullname, fs);
    }

  _cdio_list_free (p_dirlist, true);
}

static void
print_iso9660_fs (CdIo_t *p_cdio, cdio_fs_anal_t fs, 
		  track_format_t track_format)
{
  iso_extension_mask_t iso_extension_mask = ISO_EXTENSION_ALL;

  if (fs & CDIO_FS_ANAL_XA) track_format = TRACK_FORMAT_XA;

  if (opts.no_joliet) {
    iso_extension_mask &= ~ISO_EXTENSION_JOLIET;
  }

  if ( !iso9660_fs_read_superblock(p_cdio, iso_extension_mask) ) 
    return;
  
  printf ("ISO9660 filesystem\n");
  print_iso9660_recurse (p_cdio, "/", fs);
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
      printf("Audio CD, CDDB disc ID is %08x\n", 
	     cddb_discid(p_cdio, i_tracks));
#ifdef HAVE_CDDB
      if (!opts.no_cddb) print_cddb_info(p_cdio, i_tracks, i_first_track);
#endif      
      print_cdtext_info(p_cdio, i_tracks, i_first_track);
    }
    break;
  case CDIO_FS_ISO_9660:
    printf("CD-ROM with ISO 9660 filesystem");
    if (fs & CDIO_FS_ANAL_JOLIET) {
      printf(" and joliet extension level %d", cdio_iso_analysis.joliet_level);
    }
    if (fs & CDIO_FS_ANAL_ROCKRIDGE)
      printf(" and rockridge extensions");
    printf("\n");
    break;
  case CDIO_FS_ISO_9660_INTERACTIVE:
    printf("CD-ROM with CD-RTOS and ISO 9660 filesystem\n");
    break;
  case CDIO_FS_HIGH_SIERRA:
    printf("CD-ROM with High Sierra filesystem\n");
    break;
  case CDIO_FS_INTERACTIVE:
    printf("CD-Interactive%s\n", num_audio > 0 ? "/Ready" : "");
    break;
  case CDIO_FS_HFS:
    printf("CD-ROM with Macintosh HFS\n");
    break;
  case CDIO_FS_ISO_HFS:
    printf("CD-ROM with both Macintosh HFS and ISO 9660 filesystem\n");
    break;
  case CDIO_FS_UFS:
    printf("CD-ROM with Unix UFS\n");
    break;
  case CDIO_FS_EXT2:
    printf("CD-ROM with GNU/Linux EXT2 (native) filesystem\n");
	  break;
  case CDIO_FS_3DO:
    printf("CD-ROM with Panasonic 3DO filesystem\n");
    break;
  case CDIO_FS_UDFX:
    printf("CD-ROM with UDFX filesystem\n");
    break;
  case CDIO_FS_UNKNOWN:
    printf("CD-ROM with unknown filesystem\n");
    break;
  case CDIO_FS_XISO:
    printf("CD-ROM with Microsoft X-BOX XISO filesystem\n");
    break;
  }
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_ISO_9660:
  case CDIO_FS_ISO_9660_INTERACTIVE:
  case CDIO_FS_ISO_HFS:
  case CDIO_FS_ISO_UDF:
    printf("ISO 9660: %i blocks, label `%.32s'\n",
	   cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);

    {
      iso9660_pvd_t pvd;

      if ( iso9660_fs_read_pvd(p_cdio, &pvd) ) {
	char *psz_str;
	print_vd_info("Application", iso9660_get_application_id);
	print_vd_info("Preparer   ", iso9660_get_preparer_id);
	print_vd_info("Publisher  ", iso9660_get_publisher_id);
	print_vd_info("System     ", iso9660_get_system_id);
	print_vd_info("Volume     ", iso9660_get_volume_id);
	print_vd_info("Volume Set ", iso9660_get_volumeset_id);
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
    break;
  default: ;
  }

  need_lf = 0;
  if (first_data == 1 && num_audio > 0)
    need_lf += printf("mixed mode CD   ");
  if (fs & CDIO_FS_ANAL_XA)
    need_lf += printf("XA sectors   ");
  if (fs & CDIO_FS_ANAL_MULTISESSION)
    need_lf += printf("Multisession, offset = %i   ", ms_offset);
  if (fs & CDIO_FS_ANAL_HIDDEN_TRACK)
    need_lf += printf("Hidden Track   ");
  if (fs & CDIO_FS_ANAL_PHOTO_CD)
    need_lf += printf("%sPhoto CD   ", 
		      num_audio > 0 ? " Portfolio " : "");
  if (fs & CDIO_FS_ANAL_CDTV)
    need_lf += printf("Commodore CDTV   ");
  if (first_data > 1)
    need_lf += printf("CD-Plus/Extra   ");
  if (fs & CDIO_FS_ANAL_BOOTABLE)
    need_lf += printf("bootable CD   ");
  if (fs & CDIO_FS_ANAL_VIDEOCD && num_audio == 0) {
    need_lf += printf("Video CD   ");
  }
  if (fs & CDIO_FS_ANAL_SVCD)
    need_lf += printf("Super Video CD (SVCD) or Chaoji Video CD (CVD)");
  if (fs & CDIO_FS_ANAL_CVD)
    need_lf += printf("Chaoji Video CD (CVD)");
  if (need_lf) printf("\n");
#ifdef HAVE_VCDINFO
  if (fs & (CDIO_FS_ANAL_VIDEOCD|CDIO_FS_ANAL_CVD|CDIO_FS_ANAL_SVCD)) 
    if (!opts.no_vcd) {
      printf("\n");
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
    cddb_log_set_handler ((cddb_log_handler_t) _cddb_log_handler);
#endif

#ifdef HAVE_VCDINFO
  gl_default_vcd_log_handler =
    vcd_log_set_handler ((vcd_log_handler_t) _log_handler);
#endif

  /* Default option values. */
  opts.silent        = false;
  opts.list_drives   = false;
  opts.no_header     = false;
  opts.no_joliet     = 0;
  opts.no_rock_ridge = 0;
  opts.no_xa         = 0;
  opts.no_device     = 0;
  opts.no_disc_mode  = 0;
  opts.debug_level   = 0;
  opts.no_tracks     = 0;
  opts.print_iso9660 = 0;
#ifdef HAVE_CDDB
  opts.no_cddb       = 0;
  cddb_opts.port     = 8880;
  cddb_opts.http     = 0;
  cddb_opts.cachedir = NULL;
  cddb_opts.server   = NULL;
  cddb_opts.timeout  = -1;
  cddb_opts.disable_cache = false;
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
main(int argc, char *argv[])
{

  CdIo_t                *p_cdio=NULL;
  cdio_fs_anal_t         fs = CDIO_FS_AUDIO;
  int i;
  lsn_t                  start_track_lsn;      /* lsn of first track */
  lsn_t                  data_start     =  0;  /* start of data area */
  int                    ms_offset      =  0;
  track_t                i_tracks       =  0;
  track_t                i_first_track  =  0;
  unsigned int           num_audio      =  0;  /* # of audio tracks */
  unsigned int           num_data       =  0;  /* # of data tracks */
  int                    first_data     = -1;  /* # of first data track */
  int                    first_audio    = -1;  /* # of first audio track */
  bool                   b_playing_audio = false; /* currently playing a 
						     CD-DA */
  cdio_iso_analysis_t    cdio_iso_analysis; 
  char                  *media_catalog_number;  
  discmode_t             discmode = CDIO_DISC_MODE_NO_INFO;
  cdio_drive_read_cap_t  i_read_cap = 0;
  cdio_drive_write_cap_t i_write_cap;
  cdio_drive_misc_cap_t  i_misc_cap;
      
  memset(&cdio_iso_analysis, 0, sizeof(cdio_iso_analysis));
  init();

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
     
  print_version(program_name, CDIO_VERSION, opts.no_header, opts.version_only);

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
      err_exit("%s: Error in opening device driver for input %s.\n", 
	       program_name, source_name);
    } else {
      err_exit("%s: Error in opening device driver for unspecified input.\n", 
	       program_name);
    }
    
  } 

  if (source_name==NULL) {
    source_name=strdup(cdio_get_arg(p_cdio, "source"));
    if (NULL == source_name) {
      err_exit("%s: No input device given/found\n", program_name);
    }
  } 

  if (0 == opts.silent) {
    printf("CD location   : %s\n",   source_name);
    printf("CD driver name: %s\n",   cdio_get_driver_name(p_cdio));
    printf("   access mode: %s\n\n", cdio_get_arg(p_cdio, "access-mode"));
  } 

  cdio_get_drive_cap(p_cdio, &i_read_cap, &i_write_cap, &i_misc_cap);
  if (0 == opts.no_device) {
    cdio_hwinfo_t          hwinfo;
    if (cdio_get_hwinfo(p_cdio, &hwinfo)) {
      printf("%-28s: %s\n%-28s: %s\n%-28s: %s\n",
	     "Vendor"  , hwinfo.psz_vendor, 
	     "Model"   , hwinfo.psz_model, 
	     "Revision", hwinfo.psz_revision);
    }
    print_drive_capabilities(i_read_cap, i_write_cap, i_misc_cap);
  }

  if (opts.list_drives) {
    driver_id_t driver_id = DRIVER_DEVICE;
    char ** device_list = cdio_get_devices_ret(&driver_id);
    char ** d = device_list;

    printf("list of devices found:\n");
    if (NULL != d) {
      for ( ; *d != NULL ; d++ ) {
	CdIo_t *p_cdio = cdio_open(source_name, driver_id); 
	cdio_hwinfo_t hwinfo;
	printf("Drive %s\n", *d);
	if (mmc_get_hwinfo(p_cdio, &hwinfo)) {
	  printf("%-8s: %s\n%-8s: %s\n%-8s: %s\n",
		 "Vendor"  , hwinfo.psz_vendor, 
		 "Model"   , hwinfo.psz_model, 
		 "Revision", hwinfo.psz_revision);
	}
	if (p_cdio) cdio_destroy(p_cdio);
      }
    }
    cdio_free_device_list(device_list);
  }

  report(stdout, STRONG "\n");
  

  discmode = cdio_get_discmode(p_cdio);
  if ( 0 == opts.no_disc_mode ) {
    printf("Disc mode is listed as: %s\n", 
	   discmode2str[discmode]);
  }

  if (cdio_is_discmode_dvd(discmode) && !opts.show_dvd) {
    printf("No further information currently given for DVDs.\n");
    printf("Use --dvd to override.\n");
    myexit(p_cdio, EXIT_SUCCESS);
  }
  
  i_first_track = cdio_get_first_track_num(p_cdio);

  if (CDIO_INVALID_TRACK == i_first_track) {
    err_exit("Can't get first track number. I give up%s.\n", "");
  }

  i_tracks = cdio_get_num_tracks(p_cdio);

  if (CDIO_INVALID_TRACK == i_tracks) {
    err_exit("Can't get number of tracks. I give up.%s\n", "");
  }

  if (!opts.no_tracks) {
    printf("CD-ROM Track List (%i - %i)\n" NORMAL, 
	   i_first_track, i_tracks);

    printf("  #: MSF       LSN    Type   Green? Copy?");
    if ( CDIO_DISC_MODE_CD_DA == discmode 
	 || CDIO_DISC_MODE_CD_MIXED == discmode )
      printf(" Channels Premphasis?");
    printf("\n");
  }

  start_track_lsn = cdio_get_track_lsn(p_cdio, i_first_track);

  /* Read and possibly print track information. */
  for (i = i_first_track; i <= CDIO_CDROM_LEADOUT_TRACK; i++) {
    msf_t msf;
    char *psz_msf;
    track_format_t track_format;

    if (!cdio_get_track_msf(p_cdio, i, &msf)) {
      err_exit("cdio_track_msf for track %i failed, I give up.\n", i);
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
      const char *psz;
      printf( "%3d: %8s  %06lu %-6s %-5s  ", (int) i, psz_msf,
	      (long unsigned int) cdio_msf_to_lsn(&msf),
	      track_format2str[track_format],
	      cdio_get_track_green(p_cdio, i)? "true " : "false");

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

    report(stdout, "Media Catalog Number (MCN): "); fflush(stdout);

    media_catalog_number = cdio_get_mcn(p_cdio);
  
    if (NULL == media_catalog_number) {
      if (i_read_cap & CDIO_DRIVE_CAP_READ_MCN)
	report(stdout, "not available\n");
      else
	report(stdout, "not supported by drive/driver\n");
    } else {
      report(stdout, "%s\n", media_catalog_number);
      free(media_catalog_number);
    }

    /* List number of sessions */
    {
      lsn_t i_last_session;
      report(stdout, "Last CD Session LSN: "); fflush(stdout);
      if (DRIVER_OP_SUCCESS == cdio_get_last_session(p_cdio, &i_last_session)) 
	{
	  report(stdout, "%d\n", i_last_session);
	} else {
	  if (i_misc_cap & CDIO_DRIVE_CAP_MISC_MULTI_SESSION)
	    report(stdout, "failed\n");
	  else
	    report(stdout, "not supported by drive/driver\n");
	}
    }
      
    /* get audio status from subchannel */
    if  ( CDIO_DISC_MODE_CD_DA == discmode ||
	  CDIO_DISC_MODE_CD_MIXED == discmode ) {
      cdio_subchannel_t subchannel;
      driver_return_code_t rc;
      
      memset(&subchannel, 0, sizeof(subchannel));
      report( stdout, "audio status: "); fflush(stdout);
	
      rc = cdio_audio_read_subchannel(p_cdio, &subchannel);
      
      if (DRIVER_OP_SUCCESS == rc) {
	bool b_volume   = false;
	bool b_position = false;

	report ( stdout, "%s\n", 
		 mmc_audio_state2str(subchannel.audio_status) );

	switch (subchannel.audio_status) {
	case CDIO_MMC_READ_SUB_ST_PLAY:
	  b_playing_audio = true;
	  /* Fall through to next case. */
	case CDIO_MMC_READ_SUB_ST_PAUSED:
	  b_position      = true;
	  /* Fall through to next case. */
	case CDIO_MMC_READ_SUB_ST_NO_STATUS:
	  b_volume        = true;
	  break;
	default:                     
	  ;
	}

	if (b_position)
	  report( stdout, " at: %02d:%02d abs / %02d:%02d track %d\n",
		  subchannel.abs_addr.m,
		  subchannel.abs_addr.s,
		  subchannel.rel_addr.m,
		  subchannel.rel_addr.s,
		  subchannel.track );
	
	if (b_volume) {
	  cdio_audio_volume_t volume;

	  if (DRIVER_OP_SUCCESS == cdio_audio_get_volume (p_cdio, &volume)) {
	    uint8_t i=0;
	    for (i=0; i<4; i++) {
	      uint8_t i_level     = volume.level[i];
	      report( stdout, 
		      "volume level port %d: %3d (0..255) %3d (0..100)\n",
		      i, i_level, (i_level*100+128) / 256 );
	    }
	    
	  } else 
	    report( stdout, " can't get volume levels\n" );
	}
      } else if (DRIVER_OP_UNSUPPORTED == rc) {
	report( stdout, "not implemented\n" );
      } else {
	report( stdout, "FAILED\n" );
      }
    }
  }

  if (!opts.no_analysis) {
    if (b_playing_audio) {
      /* Running a CD Analysis would mess up audio playback.*/
      report(stdout, 
	     "CD Analysis Report omitted because audio is currently "
	     "playing.\n");
      myexit(p_cdio, EXIT_SUCCESS);
    }

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
	  report( stdout, "Oops: %lu unused sectors at start, "
		 "but hidden track check failed.\n",
		 (long unsigned int) start_track_lsn);
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
	  report( stdout, "session #%d starts at track %2i, LSN: %lu,"
		  " ISO 9660 blocks: %6i\n",
		  j++, i, (unsigned long int) start_track_lsn, 
		  cdio_iso_analysis.isofs_size);
	  report( stdout, "ISO 9660: %i blocks, label `%.32s'\n",
		  cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);
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
