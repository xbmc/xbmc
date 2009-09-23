/*
  $Id: cd-read.c,v 1.31 2008/03/06 01:16:49 rocky Exp $

  Copyright (C) 2003, 2004, 2005, 2006, 2008 Rocky Bernstein <rocky@gnu.org>
  
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

/* Program to debug read routines audio, auto, mode1, mode2 forms 1 & 2. */

#include "util.h"
#include <cdio/mmc.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "getopt.h"

/* Configuration option codes */
enum {
  OP_HANDLED = 0,

  /* NOTE: libpopt version associated these with drivers.  That
     appeared to be an unused historical artifact.
  */
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_NRG,
  OP_SOURCE_CDRDAO,
  OP_SOURCE_DEVICE,

  OP_USAGE,

  /* These are the remaining configuration options */
  OP_READ_MODE,
  OP_VERSION,

};

typedef enum
{
  READ_AUDIO = CDIO_READ_MODE_AUDIO,
  READ_M1F1  = CDIO_READ_MODE_M1F1,
  READ_M1F2  = CDIO_READ_MODE_M1F2,
  READ_M2F1  = CDIO_READ_MODE_M2F1,
  READ_M2F2  = CDIO_READ_MODE_M2F2,
  READ_MODE_UNINIT,
  READ_ANY
} read_mode_t;

/* Structure used so we can binary sort and set the --mode switch. */
typedef struct
{
  char name[30];
  read_mode_t read_mode;
} subopt_entry_t;


/* Sub-options for --mode.  Note: entries must be sorted! */
subopt_entry_t modes_sublist[] = {
  {"any",        READ_ANY},
  {"audio",      READ_AUDIO},
  {"m1f1",       READ_M1F1},
  {"m1f2",       READ_M1F2},
  {"m2f1",       READ_M2F1},
  {"m2f2",       READ_M2F2},
  {"mode1form1", READ_M1F1},
  {"mode1form2", READ_M1F2},
  {"mode2form1", READ_M2F1},
  {"mode2form2", READ_M2F2},
  {"red",        READ_AUDIO},
};

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  char          *access_mode; /* Access method driver should use for control */
  char          *output_file; /* file to output blocks if not NULL. */
  int            debug_level;
  int            hexdump;     /* Show output as a hexdump */
  int            nohexdump;   /* Don't output as a hexdump. I don't know
				 how to get popt to combine these as 
				 one variable.
			       */
  int            just_hex;    /* Don't try to print "printable" characters
				 in hex dump.    */
  read_mode_t    read_mode;
  int            version_only;
  int            no_header;
  int            print_iso9660;
  source_image_t source_image;
  lsn_t          start_lsn;
  lsn_t          end_lsn;
  int            num_sectors;
} opts;
     
static void
hexdump (FILE *stream,  uint8_t * buffer, unsigned int len,
	 int just_hex)
{
  unsigned int i;
  for (i = 0; i < len; i++, buffer++)
    {
      if (i % 16 == 0)
	fprintf (stream, "0x%04x: ", i);
      fprintf (stream, "%02x", *buffer);
      if (i % 2 == 1)
	fprintf (stream, " ");
      if (i % 16 == 15) {
	if (!just_hex) {
	  uint8_t *p; 
	  fprintf (stream, "  ");
	  for (p=buffer-15; p <= buffer; p++) {
	    fprintf(stream, "%c", isprint(*p) ?  *p : '.');
	  }
	}
	fprintf (stream, "\n");
      }
    }
  fprintf (stream, "\n");
  fflush (stream);
}

/* Comparison function called by bearch() to find sub-option record. */
static int
compare_subopts(const void *key1, const void *key2) 
{
  subopt_entry_t *a = (subopt_entry_t *) key1;
  subopt_entry_t *b = (subopt_entry_t *) key2;
  return (strncmp(a->name, b->name, 30));
}

/* Do processing of a --mode sub option. 
   Basically we find the option in the array, set it's corresponding
   flag variable to true as well as the "show.all" false. 
*/
static void
process_suboption(const char *subopt, subopt_entry_t *sublist, const int num,
                  const char *subopt_name) 
{
  subopt_entry_t *subopt_rec = 
    bsearch(subopt, sublist, num, sizeof(subopt_entry_t), 
            &compare_subopts);
  if (subopt_rec != NULL) {
    opts.read_mode = subopt_rec->read_mode;
    return;
  } else {
    unsigned int i;
    bool is_help=strcmp(subopt, "help")==0;
    if (is_help) {
      report( stderr, "The list of sub options for \"%s\" are:\n", 
	      subopt_name );
    } else {
      report( stderr, "Invalid option following \"%s\": %s.\n", 
	      subopt_name, subopt );
      report( stderr, "Should be one of: " );
    }
    for (i=0; i<num-1; i++) {
      report( stderr, "%s, ", sublist[i].name );
    }
    report( stderr, "or %s.\n", sublist[num-1].name );
    exit (is_help ? EXIT_SUCCESS : EXIT_FAILURE);
  }
}


/* Parse source options. */
static void
parse_source(int opt)
{
  /* NOTE: The libpopt version made use of an extra temporary
     variable (psz_my_source) for all sources _except_ devices.
     This distinction seemed to serve no purpose.
  */
  /* NOTE: The libpopt version had a bug which kept it from
     processing toc-file inputs
  */

  if (opts.source_image != INPUT_UNKNOWN) {
    report( stderr, "%s: another source type option given before.\n", 
	    program_name );
    report( stderr, "%s: give only one source type option.\n", 
	    program_name );
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


/* Parse a options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt;

  const char* helpText =
    "Usage: %s [OPTION...]\n"
    "  -a, --access-mode=STRING        Set CD control access mode\n"
    "  -m, --mode=MODE-TYPE            set CD-ROM read mode (audio, auto, m1f1, m1f2,\n"
    "                                  m2mf1, m2f2)\n"
    "  -d, --debug=INT                 Set debugging to LEVEL\n"
    "  -x, --hexdump                   Show output as a hex dump. The default is a\n"
    "                                  hex dump when output goes to stdout and no\n"
    "                                  hex dump when output is to a file.\n"
    "  -j, --just-hex                  Don't display printable chars on hex\n"
    "                                  dump. The default is print chars too.\n"
    "  --no-header                     Don't display header and copyright (for\n"
    "                                  regression testing)\n"
    "  --no-hexdump                    Don't show output as a hex dump.\n"
    "  -s, --start=INT                 Set LBA to start reading from\n"
    "  -e, --end=INT                   Set LBA to end reading from\n"
    "  -n, --number=INT                Set number of sectors to read\n"
    "  -b, --bin-file[=FILE]           set \"bin\" CD-ROM disk image file as source\n"
    "  -c, --cue-file[=FILE]           set \"cue\" CD-ROM disk image file as source\n"
    "  -i, --input[=FILE]              set source and determine if \"bin\" image or\n"
    "                                  device\n"
    "  -C, --cdrom-device[=DEVICE]     set CD-ROM device as source\n"
    "  -N, --nrg-file[=FILE]           set Nero CD-ROM disk image file as source\n"
    "  -t, --toc-file[=FILE]           set \"TOC\" CD-ROM disk image file as source\n"
    "  -o, --output-file=FILE          Output blocks to file rather than give a\n"
    "                                  hexdump.\n"
    "  -V, --version                   display version and copyright information\n"
    "                                  and exit\n"
    "\n"
    "Help options:\n"
    "  -?, --help                      Show this help message\n"
    "  --usage                         Display brief usage message\n";
  
  const char* usageText =
    "Usage: %s [-a|--access-mode STRING] [-m|--mode MODE-TYPE]\n"
    "        [-d|--debug INT] [-x|--hexdump] [--no-header] [--no-hexdump]\n"
    "        [-s|--start INT] [-e|--end INT] [-n|--number INT] [-b|--bin-file FILE]\n"
    "        [-c|--cue-file FILE] [-i|--input FILE] [-C|--cdrom-device DEVICE]\n"
    "        [-N|--nrg-file FILE] [-t|--toc-file FILE] [-o|--output-file FILE]\n"
    "        [-V|--version] [-?|--help] [--usage]\n";
  
  /* Command-line options */
  const char* optionsString = "a:m:d:xjs:e:n:b::c::i::C::N::t::o:V?";
  struct option optionsTable[] = {
  
    {"access-mode", required_argument, NULL, 'a'},
    {"mode", required_argument, NULL, 'm'},
    {"debug", required_argument, NULL, 'd'},
    {"hexdump", no_argument, NULL, 'x'},
    {"no-header", no_argument, &opts.no_header, 1},
    {"no-hexdump", no_argument, &opts.nohexdump, 1},
    {"just-hex", no_argument, &opts.just_hex, 'j'},
    {"start", required_argument, NULL, 's'},
    {"end", required_argument, NULL, 'e'},
    {"number", required_argument, NULL, 'n'},
    {"bin-file", optional_argument, NULL, 'b'},
    {"cue-file", optional_argument, NULL, 'c'},
    {"input", optional_argument, NULL, 'i'},
    {"cdrom-device", optional_argument, NULL, 'C'},
    {"nrg-file", optional_argument, NULL, 'N'},
    {"toc-file", optional_argument, NULL, 't'},
    {"output-file", required_argument, NULL, 'o'},
    {"version", no_argument, NULL, 'V'},
   
    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OP_USAGE },
    { NULL, 0, NULL, 0 }
  };
  
  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0)
    switch (opt)
      {
      case 'a': opts.access_mode = strdup(optarg); break;
      case 'd': opts.debug_level = atoi(optarg); break;
      case 'x': opts.hexdump = 1; break;
      case 's': opts.start_lsn = atoi(optarg); break;
      case 'e': opts.end_lsn = atoi(optarg); break;
      case 'n': opts.num_sectors = atoi(optarg); break;
      case 'b': parse_source(OP_SOURCE_BIN); break;
      case 'c': parse_source(OP_SOURCE_CUE); break;
      case 'i': parse_source(OP_SOURCE_AUTO); break;
      case 'C': parse_source(OP_SOURCE_DEVICE); break;
      case 'N': parse_source(OP_SOURCE_NRG); break;
      case 't': parse_source(OP_SOURCE_CDRDAO); break;
      case 'o': opts.output_file = strdup(optarg); break;
     
      case 'm':
	process_suboption(optarg, modes_sublist,
			  sizeof(modes_sublist) / sizeof(subopt_entry_t),
                            "--mode");
        break;

      case 'V':
        print_version(program_name, VERSION, 0, true);
	free(program_name);
        exit (EXIT_SUCCESS);
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
      }

  if (optind < argc) {
    const char *remaining_arg = argv[optind++];

    /* NOTE: A bug in the libpopt version checked source_image, which
       rendered the subsequent source_image test useless.
    */
    if (source_name != NULL) {
      report( stderr, "%s: Source specified in option %s and as %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }

    if (opts.source_image == INPUT_DEVICE)
      source_name = fillout_device_name(remaining_arg);
    else 
      source_name = strdup(remaining_arg);
      
    if (optind < argc) {
      report( stderr, "%s: Source specified in previously %s and %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }
  }
  
  if (opts.debug_level == 3) {
    cdio_loglevel_default = CDIO_LOG_INFO;
  } else if (opts.debug_level >= 4) {
    cdio_loglevel_default = CDIO_LOG_DEBUG;
  }

  if (opts.read_mode == READ_MODE_UNINIT) {
    report( stderr, 
	    "%s: Need to give a read mode "
	    "(audio, m1f1, m1f2, m2f1, m2f2, or auto)\n",
	    program_name );
    free(program_name);
    exit(10);
  }

  /* Check consistency between start_lsn, end_lsn and num_sectors. */

  if (opts.nohexdump && opts.hexdump != 2) {
    report( stderr, 
	    "%s: don't give both --hexdump and --no-hexdump together\n",
	    program_name );
    exit(13);
  }

  if (opts.nohexdump) opts.hexdump = 0;
  
  if (opts.start_lsn == CDIO_INVALID_LSN) {
    /* Maybe we derive the start from the end and num sectors. */
    if (opts.end_lsn == CDIO_INVALID_LSN) {
      /* No start or end LSN, so use 0 for the start */
      opts.start_lsn = 0;
      if (opts.num_sectors == 0) opts.num_sectors = 1;
    } else if (opts.num_sectors != 0) {
      if (opts.end_lsn <= opts.num_sectors) {
	report( stderr, "%s: end LSN (%lu) needs to be greater than "
		" the sector to read (%lu)\n",
		program_name, (unsigned long) opts.end_lsn, 
		(unsigned long) opts.num_sectors );
	exit(12);
      }
      opts.start_lsn = opts.end_lsn - opts.num_sectors + 1;
    }
  }

  /* opts.start_lsn has been set somehow or we've aborted. */

  if (opts.end_lsn == CDIO_INVALID_LSN) {
    if (0 == opts.num_sectors) opts.num_sectors = 1;
    opts.end_lsn = opts.start_lsn + opts.num_sectors - 1;
  } else {
    /* We were given an end lsn. */
    if (opts.end_lsn < opts.start_lsn) {
      report( stderr, 
	      "%s: end LSN (%lu) needs to be less than start LSN (%lu)\n",
	      program_name, (unsigned long) opts.start_lsn, 
	      (unsigned long) opts.end_lsn );
      free(program_name);
      exit(13);
    }
    if (opts.num_sectors != opts.end_lsn - opts.start_lsn + 1)
      if (opts.num_sectors != 0) {
	 report( stderr, 
		 "%s: inconsistency between start LSN (%lu), end (%lu), "
		 "and count (%d)\n",
		 program_name, (unsigned long) opts.start_lsn, 
		 (unsigned long) opts.end_lsn, opts.num_sectors );
	 free(program_name);
	 exit(14);
	}
    opts.num_sectors = opts.end_lsn - opts.start_lsn + 1;
  }
  
  return true;
}

static void 
log_handler (cdio_log_level_t level, const char message[])
{
  if (level == CDIO_LOG_DEBUG && opts.debug_level < 2)
    return;

  if (level == CDIO_LOG_INFO  && opts.debug_level < 1)
    return;
  
  if (level == CDIO_LOG_WARN  && opts.debug_level < 0)
    return;
  
  gl_default_cdio_log_handler (level, message);
}


static void 
init(void) 
{
  opts.debug_level   = 0;
  opts.start_lsn     = CDIO_INVALID_LSN;
  opts.end_lsn       = CDIO_INVALID_LSN;
  opts.num_sectors   = 0;
  opts.read_mode     = READ_MODE_UNINIT;
  opts.source_image  = INPUT_UNKNOWN;
  opts.hexdump       = 2;         /* Not set. */

  gl_default_cdio_log_handler = cdio_log_set_handler (log_handler);
}

int
main(int argc, char *argv[])
{
  uint8_t buffer[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  unsigned int blocklen=CDIO_CD_FRAMESIZE_RAW;
  CdIo *p_cdio=NULL;
  int output_fd=-1;
  FILE *output_stream;
  
  init();

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
     
  print_version(program_name, VERSION, opts.no_header, opts.version_only);

  p_cdio = open_input(source_name, opts.source_image, opts.access_mode);

  if (opts.output_file!=NULL) {
    
    /* If hexdump not explicitly set, then don't produce hexdump 
       when writing to a file.
     */
    if (opts.hexdump == 2) opts.hexdump = 0;

    output_fd = open(opts.output_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (-1 == output_fd) {
      err_exit("Error opening output file %s: %s\n",
	       opts.output_file, strerror(errno));

    }
  } else 
    /* If we are writing to stdout, then the default is to produce 
       a hexdump.
     */
    if (opts.hexdump == 2) opts.hexdump = 1;


  for ( ; opts.start_lsn <= opts.end_lsn; opts.start_lsn++ ) {
    switch (opts.read_mode) {
    case READ_AUDIO:
    case READ_M1F1:
    case READ_M1F2:
    case READ_M2F1:
    case READ_M2F2:
      if (DRIVER_OP_SUCCESS != 
	  cdio_read_sector(p_cdio, &buffer, 
			   opts.start_lsn, 
			   (cdio_read_mode_t) opts.read_mode)) {
	report( stderr, "error reading block %u\n", 
		(unsigned int) opts.start_lsn );
	blocklen = 0;
      } else {
	switch (opts.read_mode) {
	case READ_M1F1:
	  blocklen=CDIO_CD_FRAMESIZE;
	  break;
	case READ_M1F2:
	  blocklen=M2RAW_SECTOR_SIZE;
	case READ_M2F1:
	  blocklen=CDIO_CD_FRAMESIZE;
	case READ_M2F2:
	  blocklen=M2F2_SECTOR_SIZE;
	default: ;
	}
      }
      break;

    case READ_ANY:
      {
	driver_id_t driver_id = cdio_get_driver_id(p_cdio);
	if (cdio_is_device(source_name, driver_id)) {
	  if (DRIVER_OP_SUCCESS != 
	      mmc_read_sectors(p_cdio, &buffer, 
			       opts.start_lsn, CDIO_MMC_READ_TYPE_ANY, 1)) {
	    report( stderr, "error reading block %u\n", 
		    (unsigned int) opts.start_lsn );
	    blocklen = 0;
	  }
	} else {
	  err_exit(
		   "%s: mode 'any' must be used with a real CD-ROM, not an image file.\n", program_name);
	}
      }
      break;

      case READ_MODE_UNINIT: 
      err_exit("%s: Reading mode not set\n", program_name);
      break;
    }

    if (!opts.output_file) {
      output_stream = stdout;
    } else {
      output_stream = fdopen(output_fd, "w");
    }
    
    if (opts.hexdump)
      hexdump(output_stream, buffer, blocklen, opts.just_hex);
    else if (opts.output_file)
      write(output_fd, buffer, blocklen);
    else {
      unsigned int i;
      for (i=0; i<blocklen; i++) printf("%c", buffer[i]);
    }
    
  }

  if (opts.output_file) close(output_fd);

  myexit(p_cdio, EXIT_SUCCESS);
  /* Not reached:*/
  return(EXIT_SUCCESS);

}
