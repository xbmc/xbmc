/*
  $Id: cd-read.c,v 1.23 2005/01/09 00:10:49 rocky Exp $

  Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
  
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

/* Program to debug read routines audio, mode1, mode2 forms 1 & 2. */

#include "util.h"

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/* Configuration option codes */
enum {

  /* These correspond to driver_id_t in cdio.h and have to MATCH! */
  OP_SOURCE_UNDEF       = DRIVER_UNKNOWN,
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_NRG         = DRIVER_NRG,
  OP_SOURCE_CDRDAO         = DRIVER_CDRDAO,
  OP_SOURCE_DEVICE      = DRIVER_DEVICE,

  /* These are the remaining configuration options */
  OP_READ_MODE,
  OP_VERSION,

};

typedef enum
{
  READ_MODE_UNINIT,
  READ_AUDIO,
  READ_M1F1,
  READ_M1F2,
  READ_M2F1,
  READ_M2F2
#if AUTO_FINISHED
  READ_AUTO
#endif
} read_mode_t;

/* Structure used so we can binary sort and set the --mode switch. */
typedef struct
{
  char name[30];
  read_mode_t read_mode;
} subopt_entry_t;


/* Sub-options for --mode.  Note: entries must be sorted! */
subopt_entry_t modes_sublist[] = {
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
hexdump (FILE *stream,  uint8_t * buffer, unsigned int len)
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
	uint8_t *p; 
	fprintf (stream, "  ");
	for (p=buffer-15; p <= buffer; p++) {
	  fprintf(stream, "%c", isprint(*p) ?  *p : '.');
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
               subopt_name);
    } else {
      report( stderr, "Invalid option following \"%s\": %s.\n", 
               subopt_name, subopt);
      report( stderr, "Should be one of: " );
    }
    for (i=0; i<num-1; i++) {
      report( stderr, "%s, ", sublist[i].name );
    }
    report( stderr, "or %s.\n", sublist[num-1].name );
    exit (is_help ? EXIT_SUCCESS : EXIT_FAILURE);
  }
}

/* Parse a options. */
static bool
parse_options (int argc, const char *argv[])
{

  int opt;
  char *psz_my_source;
  char *opt_arg;

  /* Command-line options */
  struct poptOption optionsTable[] = {
  
    {"access-mode",       'a', POPT_ARG_STRING, &opts.access_mode, 0,
     "Set CD control access mode"},
    
    {"mode", 'm', 
     POPT_ARG_STRING, &opt_arg, 
     OP_READ_MODE,
     "set CD-ROM read mode (audio, m1f1, m1f2, m2mf1, m2f2)", 
     "MODE-TYPE"},
    
    {"debug",       'd', 
     POPT_ARG_INT, &opts.debug_level, 0,
     "Set debugging to LEVEL"},
    
    {"hexdump",  'x', POPT_ARG_NONE, &opts.hexdump, 0,
     "Show output as a hex dump. The default is a hex dump when "
     "output goes to stdout and no hex dump when output is to a file."},

    {"no-header", '\0', POPT_ARG_NONE, &opts.no_header, 
     0, "Don't display header and copyright (for regression testing)"},

    {"no-hexdump",  '\0', POPT_ARG_NONE, &opts.nohexdump, 0,
     "Don't show output as a hex dump."},

    {"start",       's', 
     POPT_ARG_INT, &opts.start_lsn, 0,
     "Set LBA to start reading from"},
    
    {"end",       'e', 
     POPT_ARG_INT, &opts.end_lsn, 0,
     "Set LBA to end reading from"},
    
    {"number",    'n', 
     POPT_ARG_INT, &opts.num_sectors, 0,
     "Set number of sectors to read"},
    
    {"bin-file", 'b', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_BIN, "set \"bin\" CD-ROM disk image file as source", "FILE"},
    
    {"cue-file", 'c', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_CUE, "set \"cue\" CD-ROM disk image file as source", "FILE"},
    
    {"input", 'i', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_AUTO,
     "set source and determine if \"bin\" image or device", "FILE"},
    
    {"cdrom-device", 'C', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name,
     OP_SOURCE_DEVICE,
     "set CD-ROM device as source", "DEVICE"},
    
    {"nrg-file", 'N', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_NRG, "set Nero CD-ROM disk image file as source", "FILE"},
    
    {"toc-file", 't', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_CDRDAO, "set \"TOC\" CD-ROM disk image file as source", "FILE"},
    
    {"output-file",     'o', POPT_ARG_STRING, &opts.output_file, 0,
     "Output blocks to file rather than give a hexdump.", "FILE"},
    
    {"version", 'V', POPT_ARG_NONE, NULL, OP_VERSION,
     "display version and copyright information and exit"},
    POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
  };
  poptContext optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
  
  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = poptGetNextOpt (optCon)) != -1)
    switch (opt)
      {
      case OP_SOURCE_AUTO:
      case OP_SOURCE_BIN: 
      case OP_SOURCE_CUE: 
      case OP_SOURCE_NRG: 
      case OP_SOURCE_DEVICE: 
	if (opts.source_image != INPUT_UNKNOWN) {
	  report( stderr, "%s: another source type option given before.\n", 
		  program_name);
	  report( stderr, "%s: give only one source type option.\n", 
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
      
      case OP_READ_MODE:
	process_suboption(opt_arg, modes_sublist,
			  sizeof(modes_sublist) / sizeof(subopt_entry_t),
                            "--mode");
        break;
      case OP_VERSION:
        print_version(program_name, VERSION, 0, true);
	poptFreeContext(optCon);
	free(program_name);
        exit (EXIT_SUCCESS);
        break;

      default:
        report( stderr, "%s: %s\n", 
                 poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                 poptStrerror(opt));
        report( stderr, "error while parsing command line - try --help\n" );
	poptFreeContext(optCon);
	free(program_name);
        exit (EXIT_FAILURE);
      }

  {
    const char *remaining_arg = poptGetArg(optCon);
    if ( remaining_arg != NULL) {
      if (opts.source_image != INPUT_UNKNOWN) {
	report( stderr, "%s: Source specified in option %s and as %s\n", 
		 program_name, source_name, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
      }
      
      if (opts.source_image == OP_SOURCE_DEVICE)
	source_name = fillout_device_name(remaining_arg);
      else 
	source_name = strdup(remaining_arg);
      
      if ( (poptGetArgs(optCon)) != NULL) {
	report( stderr, "%s: Source specified in previously %s and %s\n", 
		 program_name, source_name, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
	
      }
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
	    "(audio, m1f1, m1f2, m2f1 or m2f2)\n",
	    program_name);
    poptFreeContext(optCon);
    free(program_name);
    exit(10);
  }

  /* Check consistency between start_lsn, end_lsn and num_sectors. */

  if (opts.nohexdump && opts.hexdump != 2) {
    report( stderr, 
	    "%s: don't give both --hexdump and --no-hexdump together\n",
	    program_name);
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
		(unsigned long) opts.num_sectors);
	poptFreeContext(optCon);
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
	      (unsigned long) opts.end_lsn);
      poptFreeContext(optCon);
      free(program_name);
      exit(13);
    }
    if (opts.num_sectors != opts.end_lsn - opts.start_lsn + 1)
      if (opts.num_sectors != 0) {
	 report( stderr, 
		 "%s: inconsistency between start LSN (%lu), end (%lu), "
		 "and count (%d)\n",
		 program_name, (unsigned long) opts.start_lsn, 
		 (unsigned long) opts.end_lsn, opts.num_sectors);
	 poptFreeContext(optCon);
	 free(program_name);
	 exit(14);
	}
    opts.num_sectors = opts.end_lsn - opts.start_lsn + 1;
  }
  
  poptFreeContext(optCon);
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
main(int argc, const char *argv[])
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
      if (cdio_read_audio_sector(p_cdio, &buffer, opts.start_lsn)) {
	report( stderr, "error reading block %u\n", 
		 (unsigned int) opts.start_lsn);
	blocklen = 0;
      }
      break;
    case READ_M1F1:
      if (cdio_read_mode1_sector(p_cdio, &buffer, opts.start_lsn, false)) {
        report( stderr, "error reading block %u\n", 
		 (unsigned int) opts.start_lsn);
	blocklen = 0;
      } else
	blocklen=CDIO_CD_FRAMESIZE;
      break;
    case READ_M1F2:
      if (cdio_read_mode1_sector(p_cdio, &buffer, opts.start_lsn, true)) {
        report( stderr, "error reading block %u\n", 
		 (unsigned int) opts.start_lsn);
	blocklen = 0;
      } else 
	blocklen=M2RAW_SECTOR_SIZE;
      break;
    case READ_M2F1:
      if (cdio_read_mode2_sector(p_cdio, &buffer, opts.start_lsn, false)) {
        report( stderr, "error reading block %u\n", 
		 (unsigned int) opts.start_lsn);
	blocklen=0;
      } else
	blocklen=CDIO_CD_FRAMESIZE;
      break;
    case READ_M2F2:
      if (cdio_read_mode2_sector(p_cdio, &buffer, opts.start_lsn, true)) {
        report( stderr, "error reading block %u\n",  
		 (unsigned int) opts.start_lsn);
	blocklen=0;
      } else
	blocklen=M2F2_SECTOR_SIZE;
      break;
#if AUTO_FINISHED
    case READ_AUTO:
      /* Find what track lsn is in. Then 
         switch cdio_get_track_format(p_cdio, i)
         and also test using is_green

      */
      break;
#endif 
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
      hexdump(output_stream, buffer, blocklen);
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
