/*
    $Id: iso-info.c,v 1.20 2005/01/22 22:21:36 rocky Exp $

    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>

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
  ISO Info - prints various information about a ISO 9660 image.
*/
#include "util.h"
#undef err_exit

#define err_exit(fmt, args...) \
  report (stderr, "%s: "fmt, program_name, ##args); \
  iso9660_close(p_iso);				    \
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
  int            print_iso9660;
  int            print_iso9660_short;
} opts;
     
/* Configuration option codes */
enum {
  
  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

char *temp_str;


/* Parse a all options. */
static bool
parse_options (int argc, const char *argv[])
{
  int opt;

  struct poptOption optionsTable[] = {
    {"debug",       'd', POPT_ARG_INT, &opts.debug_level, 0,
     "Set debugging to LEVEL"},
    
    {"input", 'i', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 0,
     "Filename to read ISO-9960 image from", "FILE"},
    
    {'\0',    'f', POPT_ARG_NONE, &opts.print_iso9660_short, 0,
     "Generate output similar to 'find . -print'"},

    {"iso9660",  'l', POPT_ARG_NONE, &opts.print_iso9660, 0,
     "Generate output similar to 'ls -lR'"},
    
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
    default:
      poptFreeContext(optCon);
      return false;
    }
  }
  {
    const char *remaining_arg = poptGetArg(optCon);
    if ( remaining_arg != NULL) {
      if ( (poptGetArgs(optCon)) != NULL) {
	report( stderr, "%s: Source specified in previously %s and %s\n", 
		 program_name, source_name, remaining_arg);
	poptFreeContext(optCon);
	exit (EXIT_FAILURE);
      }
      source_name = strdup(remaining_arg);
    }
  }
  
  poptFreeContext(optCon);
  return true;
}

/* CDIO logging routines */

static cdio_log_handler_t gl_default_cdio_log_handler = NULL;

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
print_iso9660_recurse (iso9660_t *p_iso, const char pathname[])
{
  CdioList_t *entlist;
  CdioList_t *dirlist =  _cdio_list_new ();
  CdioListNode_t *entnode;
  uint8_t i_joliet_level = iso9660_ifs_get_joliet_level(p_iso);

  entlist = iso9660_ifs_readdir (p_iso, pathname);
    
  if (opts.print_iso9660) {
  printf ("%s:\n", pathname);
  }

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

      if (opts.print_iso9660) {
      if (iso9660_ifs_is_xa(p_iso)) {
	printf ( "  %c %s %d %d [fn %.2d] [LSN %6lu] ",
		 (statbuf->type == _STAT_DIR) ? 'd' : '-',
		 iso9660_get_xa_attr_str (statbuf->xa.attributes),
		 uint16_from_be (statbuf->xa.user_id),
		 uint16_from_be (statbuf->xa.group_id),
		 statbuf->xa.filenum,
		 (long unsigned int) statbuf->lsn);
	
	if (uint16_from_be(statbuf->xa.attributes) & XA_ATTR_MODE2FORM2) {
	  printf ("%9u (%9u)",
		  (unsigned int) statbuf->secsize * M2F2_SECTOR_SIZE,
		  (unsigned int) statbuf->size);
	} else {
	  printf ("%9u", (unsigned int) statbuf->size);
	}
      }
      strftime(date_str, DATESTR_SIZE, "%b %d %Y %H:%M ", &statbuf->tm);
      printf (" %s %s\n", date_str, translated_name);
      } else 
	if ( strcmp (iso_name, ".") && strcmp (iso_name, ".."))
	  printf("%s%s\n", pathname, translated_name);
    }

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
  opts.silent        = false;
  opts.no_header     = false;
  opts.no_joliet     = false;
  opts.debug_level   = 0;
  opts.print_iso9660 = 0;
  opts.print_iso9660_short = 0;
}

#define print_vd_info(title, fn)	  \
  if (fn(p_iso, &psz_str)) {		  \
    printf(title ": %s\n", psz_str);	  \
    free(psz_str);			  \
    psz_str = NULL;			  \
  }

/* ------------------------------------------------------------------------ */

int
main(int argc, const char *argv[])
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
