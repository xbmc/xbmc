/*
    $Id: iso-info.c,v 1.38 2008/01/09 04:26:24 rocky Exp $

    Copyright (C) 2004, 2005, 2006, 2008 Rocky Bernstein <rocky@gnu.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301 USA.
*/
/*
  ISO Info - prints various information about a ISO 9660 image.
*/
#include "getopt.h"
#include "util.h"
#undef err_exit

#define err_exit(fmt, args...)			    \
  report (stderr, "%s: "fmt, program_name, ##args); \
  iso9660_close(p_iso);				    \
  free(program_name);				    \
  return(EXIT_FAILURE);

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio/bytesex.h>
#include <cdio/cdio.h>
#include <cdio/ds.h>
#include <cdio/iso9660.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <errno.h>

#if 0
#define STRONG "\033[1m"
#define NORMAL "\033[0m"
#else
#define STRONG "__________________________________\n"
#define NORMAL ""
#endif

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  uint32_t       debug_level;
  int            version_only;
  int            silent;
  int            no_header;
  int            no_joliet;
  int            no_xa;
  int            no_rock_ridge;
  int            print_iso9660;
  int            print_iso9660_short;
} opts;
     
/* Configuration option codes */
enum {
  OP_HANDLED = 0,

  OP_USAGE,

  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

char *temp_str;


/* Parse a all options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt;

  const char* helpText =
    "Usage: %s [OPTION...]\n"
    "  -d, --debug=INT        Set debugging to LEVEL\n"
    "  -i, --input[=FILE]     Filename to read ISO-9960 image from\n"
    "  -f                     Generate output similar to 'find . -print'\n"
    "  -l, --iso9660          Generate output similar to 'ls -lR'\n"
    "  --no-header            Don't display header and copyright (for regression\n"
    "                         testing)\n"
#ifdef HAVE_JOLIET    
    "  --no-joliet            Don't use Joliet-extension information\n"
#endif /*HAVE_JOLIET*/
    "  --no-rock-ridge        Don't use Rock-Ridge-extension information\n"
    "  --no-xa                Don't use XA-extension information\n"
    "  -q, --quiet            Don't produce warning output\n"
    "  -V, --version          display version and copyright information and exit\n"
    "\n"
    "Help options:\n"
    "  -?, --help             Show this help message\n"
    "  --usage                Display brief usage message\n";

  const char* usageText =
    "Usage: %s [-d|--debug INT] [-i|--input FILE] [-f] [-l|--iso9660]\n"
    "        [--no-header] [--no-joliet] [--no-rock-ridge] [--no-xa] [-q|--quiet]\n"
    "        [-V|--version] [-?|--help] [--usage]\n";

  const char* optionsString = "d:i::flqV?";
  struct option optionsTable[] = {
    {"debug", required_argument, NULL, 'd'},
    {"input", optional_argument, NULL, 'i'},
    {"iso9660", no_argument, NULL, 'l'},
    {"no-header", no_argument, &opts.no_header, 1 },
#ifdef HAVE_JOLIET    
    {"no-joliet", no_argument, &opts.no_joliet, 1 },
#endif /*HAVE_JOLIET*/
    {"no-rock-ridge", no_argument, &opts.no_rock_ridge, 1 },
    {"no-xa", no_argument, &opts.no_xa, 1 },
    {"quiet", no_argument, NULL, 'q'},
    {"version", no_argument, NULL, 'V'},

    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OP_USAGE },
    { NULL, 0, NULL, 0 }
  };

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0) {
    switch (opt)
      {
      case 'd': opts.debug_level = atoi(optarg); break;
      case 'i': if (optarg != NULL) source_name = strdup(optarg); break;
      case 'f': opts.print_iso9660_short = 1; break;
      case 'l': opts.print_iso9660 = 1; break;
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
    if ( optind < argc ) {
      report( stderr, "%s: Source specified in previously %s and %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }
    source_name = strdup(remaining_arg);
  }
  
  return true;
}

/* CDIO logging routines */

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
print_iso9660_recurse (iso9660_t *p_iso, const char psz_path[])
{
  CdioList_t *entlist;
  CdioList_t *dirlist =  _cdio_list_new ();
  CdioListNode_t *entnode;
  uint8_t i_joliet_level = iso9660_ifs_get_joliet_level(p_iso);
  char *translated_name = (char *) malloc(4096);
  size_t translated_name_size = 4096;
  entlist = iso9660_ifs_readdir (p_iso, psz_path);
    
  if (opts.print_iso9660) {
    printf ("%s:\n", psz_path);
  }

  if (NULL == entlist) {
    report( stderr, "Error getting above directory information\n" );
    return;
  }

  /* Iterate over files in this directory */
  
  _CDIO_LIST_FOREACH (entnode, entlist)
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
	snprintf (_fullname, sizeof (_fullname), "%s%s", psz_path, 
		  translated_name);
      } else {
	snprintf (_fullname, sizeof (_fullname), "%s%s", psz_path, 
		  psz_iso_name);
      }
      
      strncat (_fullname, "/", sizeof (_fullname));

      if (p_statbuf->type == _STAT_DIR
          && strcmp (psz_iso_name, ".") 
          && strcmp (psz_iso_name, ".."))
        _cdio_list_append (dirlist, strdup (_fullname));

      if (opts.print_iso9660) {
	print_fs_attrs(p_statbuf, 
		       0 == opts.no_rock_ridge,
		       iso9660_ifs_is_xa(p_iso) && 0 == opts.no_xa,
		       psz_iso_name, translated_name);
      } else 
	if ( strcmp (psz_iso_name, ".") && strcmp (psz_iso_name, ".."))
	  printf("%9u %s%s\n", (unsigned int) p_statbuf->size, psz_path, 
		 yep == p_statbuf->rr.b3_rock 
		 ? psz_iso_name : translated_name);
      if (p_statbuf->rr.i_symlink) {
	free(p_statbuf->rr.psz_symlink);
	p_statbuf->rr.i_symlink = 0;
      }
    }
    free (translated_name);

  _cdio_list_free (entlist, true);

  if (opts.print_iso9660) {
    printf ("\n");
  }

  /* Now recurse over the directories. */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);

      print_iso9660_recurse (p_iso, _fullname);
    }

  _cdio_list_free (dirlist, true);
}

static void
print_iso9660_fs (iso9660_t *iso)
{
  print_iso9660_recurse (iso, "/");
}


/* Initialize global variables. */
static void 
init(void) 
{
  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);

  /* Default option values. */
  opts.silent              = false;
  opts.no_header           = false;
  opts.no_joliet           = 0;
  opts.no_rock_ridge       = 0;
  opts.no_xa               = 0;
  opts.debug_level         = 0;
  opts.print_iso9660       = 0;
  opts.print_iso9660_short = 0;
}

#define print_vd_info(title, fn)	  \
  if (fn(p_iso, &psz_str)) {		  \
    printf(title ": %s\n", psz_str);	  \
  }					  \
  free(psz_str);			  \
  psz_str = NULL;			  

/* ------------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{

  iso9660_t           *p_iso=NULL;
  iso_extension_mask_t iso_extension_mask = ISO_EXTENSION_ALL;
      
  init();

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
  
  print_version(program_name, CDIO_VERSION, opts.no_header, opts.version_only);

  if (opts.debug_level == 3) {
    cdio_loglevel_default = CDIO_LOG_INFO;
  } else if (opts.debug_level >= 4) {
    cdio_loglevel_default = CDIO_LOG_DEBUG;
  }

  if (source_name==NULL) {
    err_exit("No input device given/found%s\n", "");
  } 

  if (opts.no_joliet) {
    iso_extension_mask &= ~ISO_EXTENSION_JOLIET;
  }
  
  p_iso = iso9660_open_ext (source_name, iso_extension_mask);

  if (p_iso==NULL) {
    free(source_name);
    err_exit("Error in opening ISO-9660 image%s\n", "");
  } 

  if (opts.silent == 0) {
    char *psz_str = NULL;
    
    printf(STRONG "ISO 9660 image: %s\n", source_name);
    print_vd_info("Application", iso9660_ifs_get_application_id);
    print_vd_info("Preparer   ", iso9660_ifs_get_preparer_id);
    print_vd_info("Publisher  ", iso9660_ifs_get_publisher_id);
    print_vd_info("System     ", iso9660_ifs_get_system_id);
    print_vd_info("Volume     ", iso9660_ifs_get_volume_id);
    print_vd_info("Volume Set ", iso9660_ifs_get_volumeset_id);
  }
  
  if (opts.print_iso9660 || opts.print_iso9660_short) {
    printf(STRONG "ISO-9660 Information\n" NORMAL);
    if (opts.print_iso9660 && opts.print_iso9660_short) {
      printf("Note: both -f and -l options given -- "
	     "-l (long listing) takes precidence\n");
    }
    print_iso9660_fs(p_iso);
  }

  free(source_name);
  iso9660_close(p_iso);
  /* Not reached:*/
  free(program_name);
  return(EXIT_SUCCESS);
}
