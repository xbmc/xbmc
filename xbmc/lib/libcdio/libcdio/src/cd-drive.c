/*
  $Id: cd-drive.c,v 1.27 2007/03/09 01:33:42 rocky Exp $

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

/* Program to show drivers installed and capibilites of CD drives. */

#include "util.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include "getopt.h"
#include <cdio/cdio.h>
#include <cdio/mmc.h>

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  uint32_t       debug_level;
  int            version_only;
  int            silent;
  source_image_t source_image;
} opts;
     
/* Configuration option codes */
enum {
  OP_HANDLED,

  OP_SOURCE_DEVICE,

  OP_USAGE,

  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

/* Parse all options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt;

  const char* helpText =
    "Usage: %s [OPTION...]\n" 
    "  -d, --debug=INT                 Set debugging to LEVEL\n"
    "  -i, --cdrom-device[=DEVICE]     show only info about CD-ROM device\n"
    "  -q, --quiet                     Don't produce warning output\n"
    "  -V, --version                   display version and copyright information\n"
    "                                  and exit\n"
    "\n"
    "Help options:\n"
    "  -?, --help                      Show this help message\n"
    "  --usage                         Display brief usage message\n";
  
  const char* usageText =
    "Usage: %s [-d|--debug INT] [-i|--cdrom-device DEVICE] [-q|--quiet]\n"
    "        [-V|--version] [-?|--help] [--usage]\n";

  const char* optionsString = "d:i::qV?";
  struct option optionsTable[] = {
    {"debug", required_argument, NULL, 'd' },
    {"cdrom-device", optional_argument, NULL, 'i' },
    {"quiet", no_argument, NULL, 'q' },
    {"version", no_argument, NULL, 'V' },
    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OP_USAGE },
    {NULL, 0, NULL, 0 }
  };

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) != -1) {
    switch (opt) {
    case 'd':
      opts.debug_level = atoi(optarg);
      break;

    case 'i': 
      if (opts.source_image != DRIVER_UNKNOWN) {
	/* NOTE: The libpopt version already set source_name by this time.
	   To restore this behavior, fall through to the else{} block.
	*/
	report( stderr, "%s: another source type option given before.\n", 
		program_name );
	report( stderr, "%s: give only one source type option.\n", 
		program_name );
	break;
      } else {
	opts.source_image  = DRIVER_DEVICE;
	if (optarg != NULL) {
	  source_name = fillout_device_name(optarg);
	}
	break;
      }
      break;

    case 'q':
      opts.silent = 1;
      break;

    case 'V':
      opts.version_only = 1;
      break;

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

    default:
      return false;
    }
  }
  if (optind < argc) {
    const char *remaining_arg = argv[optind++];

    /* NOTE: A bug in the libpopt version checked source_image, which
       rendered the subsequent source_image test useless.
    */
    if (source_name != NULL) {
      report( stderr, "%s: Source specified in option %s and as %s\n", 
	      program_name, source_name, remaining_arg);
      free(program_name);
      exit (EXIT_FAILURE);
    }
      
    if (opts.source_image == DRIVER_DEVICE)
      source_name = fillout_device_name(remaining_arg);
    else 
      source_name = strdup(remaining_arg);

    if (optind < argc) {
      report( stderr, "%s: Source specified in previously %s and %s\n", 
	      program_name, source_name, remaining_arg);
      free(program_name);
      exit (EXIT_FAILURE);
    }
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

/*! Prints out SCSI-MMC drive features  */
static void 
print_mmc_drive_level(CdIo_t *p_cdio)
{
  cdio_mmc_level_t mmc_level = mmc_get_drive_mmc_cap(p_cdio);
    
  printf( "CD-ROM drive supports " );

  switch(mmc_level) {
  case CDIO_MMC_LEVEL_WEIRD:
    printf("some nonstandard or degenerate set of MMC\n");
    break;
  case CDIO_MMC_LEVEL_1:
    printf("MMC 1\n");
    break;
  case CDIO_MMC_LEVEL_2:
    printf("MMC 2\n");
    break;
  case CDIO_MMC_LEVEL_3:
    printf("MMC 3\n");
    break;
  case CDIO_MMC_LEVEL_NONE:
    printf("no MMC\n");
    break;
  }
  printf("\n");
}

/* Initialize global variables. */
static void 
init(void) 
{
  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);

  /* Default option values. */
  opts.silent        = false;
  opts.debug_level   = 0;
  opts.source_image  = DRIVER_UNKNOWN;
}

int
main(int argc, char *argv[])
{
  CdIo_t *p_cdio=NULL;
  
  init();

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
     
  print_version(program_name, CDIO_VERSION, false, opts.version_only);

  if (opts.debug_level == 3) {
    cdio_loglevel_default = CDIO_LOG_INFO;
  } else if (opts.debug_level >= 4) {
    cdio_loglevel_default = CDIO_LOG_DEBUG;
  }

  if (NULL == source_name) {
    char *default_device;

    p_cdio = cdio_open (NULL, DRIVER_DEVICE);

    if (NULL == p_cdio) {
      printf("No loaded CD-ROM device accessible.\n");
    }  else {
      default_device = cdio_get_default_device(p_cdio);
      
      printf("The driver selected is %s\n", cdio_get_driver_name(p_cdio));

      if (default_device) {
	printf("The default device for this driver is %s\n", default_device);
      }
    
      free(default_device);
      cdio_destroy(p_cdio);
      p_cdio=NULL;
      printf("\n");
    }
  }
  
  /* Print out a drivers available */
  {
    driver_id_t driver_id;

    printf("Drivers available...\n");
    for (driver_id=CDIO_MIN_DRIVER; driver_id<=CDIO_MAX_DRIVER; driver_id++)
      if (cdio_have_driver(driver_id)) {
	printf("  %-35s\n", cdio_driver_describe(driver_id));
      }
    printf("\n");
  }
  
    
  if (NULL == source_name) {
    /* Print out a list of CD-drives */

    char **ppsz_cdrives=NULL, **ppsz_cd;
    driver_id_t driver_id = DRIVER_DEVICE;
    
    ppsz_cdrives = cdio_get_devices_ret(&driver_id);
    if (NULL != ppsz_cdrives) 
      for( ppsz_cd = ppsz_cdrives; *ppsz_cd != NULL; ppsz_cd++ ) {
	cdio_drive_read_cap_t  i_read_cap;
	cdio_drive_write_cap_t i_write_cap;
	cdio_drive_misc_cap_t  i_misc_cap;
	cdio_hwinfo_t          hwinfo;
	CdIo_t *p_cdio = cdio_open(*ppsz_cd, driver_id); 

	print_mmc_drive_level(p_cdio);

	printf("%28s: %s\n", "Drive", *ppsz_cd);

	if (p_cdio) {
	  if (cdio_get_hwinfo(p_cdio, &hwinfo)) {
	    printf("%-28s: %s\n%-28s: %s\n%-28s: %s\n",
		   "Vendor"  , hwinfo.psz_vendor, 
		   "Model"   , hwinfo.psz_model, 
		   "Revision", hwinfo.psz_revision);
	  }
	  print_mmc_drive_features(p_cdio);
	  cdio_get_drive_cap(p_cdio, &i_read_cap, &i_write_cap, 
			     &i_misc_cap);
	  print_drive_capabilities(i_read_cap, i_write_cap, i_misc_cap);
	}
	printf("\n");
	if (p_cdio) cdio_destroy(p_cdio);
      }
    
    cdio_free_device_list(ppsz_cdrives);
    ppsz_cdrives = NULL;
  } else {
    /* Print CD-drive info for given source */
    cdio_drive_read_cap_t  i_read_cap;
    cdio_drive_write_cap_t i_write_cap;
    cdio_drive_misc_cap_t  i_misc_cap;
    cdio_hwinfo_t          hwinfo;
    
    printf("Drive %s\n", source_name);
    p_cdio = cdio_open (source_name, DRIVER_UNKNOWN);

    if (p_cdio) {

      print_mmc_drive_level(p_cdio);

      if (cdio_get_hwinfo(p_cdio, &hwinfo)) {
	printf("%-28s: %s\n%-28s: %s\n%-28s: %s\n",
	       "Vendor"  , hwinfo.psz_vendor, 
	       "Model"   , hwinfo.psz_model, 
	       "Revision", hwinfo.psz_revision);
      }
      print_mmc_drive_features(p_cdio);
    }
    cdio_get_drive_cap_dev(source_name, &i_read_cap, &i_write_cap, 
			   &i_misc_cap);
    print_drive_capabilities(i_read_cap, i_write_cap, i_misc_cap);
    printf("\n");
  }

  myexit(p_cdio, EXIT_SUCCESS);
  /* Not reached:*/
  return(EXIT_SUCCESS);
}
