/*
  $Id: mmc-tool.c,v 1.10 2008/01/09 04:26:24 rocky Exp $

  Copyright (C) 2006 Rocky Bernstein <rocky@cpan.org>
  
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

/* A program to using the MMC interface to list CD and drive features
   from the MMC GET_CONFIGURATION command . */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include "util.h"
#include "getopt.h"

static void
init(const char *argv0) 
{
  program_name = strrchr(argv0,'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv0);
}

/* Configuration option codes */
typedef enum {
  OPT_HANDLED = 0,
  OPT_USAGE,
  OPT_DRIVE_CAP,
  OPT_VERSION,
} option_t;

typedef enum {
  /* These are the remaining configuration options */
  OP_FINISHED = 0,
  OP_BLOCKSIZE,
  OP_CLOSETRAY,
  OP_EJECT,
  OP_IDLE,
  OP_INQUIRY,
  OP_MODE_SENSE_2A,
  OP_MCN,
  OP_SPEED,
} operation_enum_t;

typedef struct 
{
  operation_enum_t op;
  union 
  {
    long int i_num;
    char * psz;
  } arg;
} operation_t;
  

enum { MAX_OPS = 10 };

unsigned int last_op = 0;
operation_t operation[MAX_OPS] = { {OP_FINISHED, {0}} };

static void 
push_op(operation_t *p_op) 
{
  if (last_op < MAX_OPS) {
    memcpy(&operation[last_op], p_op, sizeof(operation_t));
    last_op++;
  }
}

/* Parse a options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt;
  operation_t op;
  int i_blocksize = 0;

  const char* helpText =
    "Usage: %s [OPTION...]\n"
    " Issues libcdio Multimedia commands. Operations occur in the order\n"
    " in which the options are given and a given operation may appear\n"
    " more than once to have it run more than once.\n"
    "options: \n"
    "  -b, --blocksize[=INT]           set blocksize. If no block size or a \n"
    "                                  zero blocksize is given we return the\n"
    "                                  current setting.\n"
    "  -C, --drive-cap [6|10]          print mode sense 2a data\n"
    "                                  using 6-byte or 10-byte form\n"
    "  -c, --close drive               close drive via ALLOW_MEDIUM_REMOVAL\n"
    "  -e, --eject [drive]             eject drive via ALLOW_MEDIUM_REMOVAL\n"
    "                                  and a MMC START/STOP command\n"
    "  -I, --idle                      set CD-ROM to idle or power down\n"
    "                                  via MMC START/STOP command\n"
    "  -i, --inquiry                   print HW info via INQUIRY\n"
    "  -m, --mcn                       get media catalog number (AKA UPC)\n"
    "  -s, --speed-KB=INT              Set drive speed to SPEED K bytes/sec\n"
    "                                  Note: 1x = 176 KB/s          \n"
    "  -S, --speed-X=INT               Set drive speed to INT X\n"
    "                                  Note: 1x = 176 KB/s          \n"
    "  -V, --version                   display version and copyright information\n"
    "                                  and exit\n"
    "\n"
    "Help options:\n"
    "  -?, --help                      Show this help message\n"
    "  --usage                         Display brief usage message\n";
  
  const char* usageText =
    "Usage: %s [-b|--blocksize[=INT]] [-m|--mcn]\n"
    "        [-I|--idle] [-I|inquiry] [-m[-s|--speed-KB INT]\n"
    "        [-V|--version] [-?|--help] [--usage]\n";
  
  /* Command-line options */
  const char* optionsString = "b::c:C::e::Iis:V?";
  struct option optionsTable[] = {
  
    {"blocksize", optional_argument, &i_blocksize, 'b' },
    {"close", required_argument, NULL, 'c'},
    {"drive-cap", optional_argument, NULL, 'C'},
    {"eject", optional_argument, NULL, 'e'},
    {"idle", no_argument, NULL, 'I'},
    {"inquiry", no_argument, NULL, 'i'},
    {"mcn",   no_argument, NULL, 'm'},
    {"speed-KB", required_argument, NULL, 's'},
    {"speed-X",  required_argument, NULL, 'S'},

    {"version", no_argument, NULL, 'V'},
    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OPT_USAGE },
    { NULL, 0, NULL, 0 }
  };
  
  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0)
    switch (opt)
      {
      case 'b': 
	op.op = OP_BLOCKSIZE;
	op.arg.i_num = i_blocksize;
	push_op(&op);
	break;
      case 'C': 
	op.arg.i_num = optarg ? atoi(optarg) : 10;
	switch (op.arg.i_num) {
	case 10:
	    op.op = OP_MODE_SENSE_2A;
	  op.arg.i_num = 10;
	  push_op(&op);
	  break;
	case 6:
	  op.op = OP_MODE_SENSE_2A;
	  op.arg.i_num = 6;
	  push_op(&op);
	  break;
	default:
	  report( stderr, "%s: Expecting 6 or 10 or nothing\n", program_name );
	}
	break;
      case 'c': 
	op.op = OP_CLOSETRAY;
	op.arg.psz = strdup(optarg);
	push_op(&op);
	break;
      case 'e': 
	op.op = OP_EJECT;
	op.arg.psz=NULL;
	if (optarg) op.arg.psz = strdup(optarg);
	push_op(&op);
	break;
      case 'i': 
	op.op = OP_INQUIRY;
	op.arg.psz=NULL;
	push_op(&op);
	break;
      case 'I': 
	op.op = OP_IDLE;
	op.arg.psz=NULL;
	push_op(&op);
	break;
      case 'm': 
	op.op = OP_MCN;
	op.arg.psz=NULL;
	push_op(&op);
	break;
      case 's': 
	op.op = OP_SPEED;
	op.arg.i_num=atoi(optarg);
	push_op(&op);
	break;
      case 'S': 
	op.op = OP_SPEED;
	op.arg.i_num=176 * atoi(optarg);
	push_op(&op);
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
	
      case OPT_USAGE:
	fprintf(stderr, usageText, program_name);
	free(program_name);
	exit(EXIT_FAILURE);
	break;

      case OPT_HANDLED:
	break;
      i_blocksize = 0;
      }

  if (optind < argc) {
    const char *remaining_arg = argv[optind++];

    if (source_name != NULL) {
      report( stderr, "%s: Source specified in option %s and as %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }

    source_name = strdup(remaining_arg);
      
    if (optind < argc) {
      report( stderr, "%s: Source specified in previously %s and %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }
  }
  
  return true;
}

static void 
print_mode_sense (unsigned int i_mmc_size, const uint8_t buf[30])
{
  printf("Mode sense %d information\n", i_mmc_size);
  if (buf[2] & 0x01) {
    printf("\tReads CD-R media.\n");
  }
  if (buf[2] & 0x02) {
    printf("\tReads CD-RW media.\n");
  }
  if (buf[2] & 0x04) {
    printf("\tReads fixed-packet tracks when Addressing type is method 2.\n");
  }
  if (buf[2] & 0x08) {
    printf("\tReads DVD ROM media.\n");
  }
  if (buf[2] & 0x10) {
    printf("\tReads DVD-R media.\n");
  }
  if (buf[2] & 0x20) {
    printf("\tReads DVD-RAM media.\n");
  }
  if (buf[2] & 0x40) {
    printf("\tReads DVD-RAM media.\n");
  }
  if (buf[3] & 0x01) {
    printf("\tWrites CD-R media.\n");
  }
  if (buf[3] & 0x02) {
    printf("\tWrites CD-RW media.\n");
  }
  if (buf[3] & 0x04) {
    printf("\tSupports emulation write.\n");
  }
  if (buf[3] & 0x10) {
    printf("\tWrites DVD-R media.\n");
  }
  if (buf[3] & 0x20) {
    printf("\tWrites DVD-RAM media.\n");
  }
  if (buf[4] & 0x01) {
    printf("\tCan play audio.\n");
  }
  if (buf[4] & 0x02) {
    printf("\tDelivers composition A/V stream.\n");
  }
  if (buf[4] & 0x04) {
    printf("\tSupports digital output on port 2.\n");
  }
  if (buf[4] & 0x08) {
    printf("\tSupports digital output on port 1.\n");
  }
  if (buf[4] & 0x10) {
    printf("\tReads Mode-2 form 1 (e.g. XA) media.\n");
  }
  if (buf[4] & 0x20) {
    printf("\tReads Mode-2 form 2 media.\n");
  }
  if (buf[4] & 0x40) {
    printf("\tReads multi-session CD media.\n");
  }
  if (buf[4] & 0x80) {
    printf("\tSupports Buffer under-run free recording on CD-R/RW media.\n");
  }
  if (buf[4] & 0x01) {
    printf("\tCan read audio data with READ CD.\n");
  }
  if (buf[4] & 0x02) {
    printf("\tREAD CD data stream is accurate.\n");
  }
  if (buf[5] & 0x04) {
    printf("\tReads R-W subchannel information.\n");
  }
  if (buf[5] & 0x08) {
    printf("\tReads de-interleaved R-W subchannel.\n");
  }
  if (buf[5] & 0x10) {
    printf("\tSupports C2 error pointers.\n");
  }
  if (buf[5] & 0x20) {
    printf("\tReads ISRC information.\n");
  }
  if (buf[5] & 0x40) {
    printf("\tReads ISRC informaton.\n");
  }
  if (buf[5] & 0x40) {
    printf("\tReads media catalog number (MCN also known as UPC).\n");
  }
  if (buf[5] & 0x80) {
    printf("\tReads bar codes.\n");
  }
  if (buf[6] & 0x01) {
    printf("\tPREVENT/ALLOW may lock media.\n");
  }
  printf("\tLock state is %slocked.\n", (buf[6] & 0x02) ? "" : "un");
  printf("\tPREVENT/ALLOW jumper is %spresent.\n", (buf[6] & 0x04) ? "": "not ");
  if (buf[6] & 0x08) {
    printf("\tEjects media with START STOP UNIT.\n");
  }
  {
    const unsigned int i_load_type = (buf[6]>>5 & 0x07);
    printf("\tLoading mechanism type  is %d: ", i_load_type);
    switch (buf[6]>>5 & 0x07) {
    case 0: 
      printf("caddy type loading mechanism.\n"); 
      break;
    case 1: 
      printf("tray type loading mechanism.\n"); 
      break;
    case 2: 
      printf("popup type loading mechanism.\n");
      break;
    case 3: 
      printf("reserved\n");
      break;
    case 4: 
      printf("changer with individually changeable discs.\n");
      break;
    case 5: 
      printf("changer using Magazine mechanism.\n");
      break;
    case 6: 
      printf("changer using Magazine mechanism.\n");
      break;
    default:
      printf("Invalid.\n");
      break;
    }
  }
  
  if (buf[7] & 0x01) {
    printf("\tVolume controls each channel separately.\n");
  }
  if (buf[7] & 0x02) {
    printf("\tHas a changer that supports disc present reporting.\n");
  }
  if (buf[7] & 0x04) {
    printf("\tCan load empty slot in changer.\n");
  }
  if (buf[7] & 0x08) {
    printf("\tSide change capable.\n");
  }
  if (buf[7] & 0x10) {
    printf("\tReads raw R-W subchannel information from lead in.\n");
  }
  {
    const unsigned int i_speed_Kbs = CDIO_MMC_GETPOS_LEN16(buf,  8);
    printf("\tMaximum read speed is %d K bytes/sec (about %dX)\n", 
	   i_speed_Kbs, i_speed_Kbs / 176) ;
  }
  printf("\tNumber of Volume levels is %d\n",  CDIO_MMC_GETPOS_LEN16(buf, 10));
  printf("\tBuffers size for data is %d KB\n", CDIO_MMC_GETPOS_LEN16(buf, 12));
  printf("\tCurrent read speed is %d KB\n",    CDIO_MMC_GETPOS_LEN16(buf, 14));
  printf("\tMaximum write speed is %d KB\n",   CDIO_MMC_GETPOS_LEN16(buf, 18));
  printf("\tCurrent write speed is %d KB\n",   CDIO_MMC_GETPOS_LEN16(buf, 28));
}

int
main(int argc, char *argv[])
{
  CdIo_t *p_cdio;

  driver_return_code_t rc = DRIVER_OP_SUCCESS;
  unsigned int i;

  init(argv[0]);
  
  parse_options(argc, argv);
  p_cdio = cdio_open (source_name, DRIVER_DEVICE);

  if (NULL == p_cdio) {
    printf("Couldn't find CD\n");
    return 1;
  } 

  for (i=0; i < last_op; i++) {
    const operation_t *p_op = &operation[i];
    switch (p_op->op) {
    case OP_SPEED:
      rc = mmc_set_speed(p_cdio, p_op->arg.i_num);
      report(stdout, "%s (mmc_set_speed): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      break;
    case OP_BLOCKSIZE:
      if (p_op->arg.i_num) {
	driver_return_code_t rc = mmc_set_blocksize(p_cdio, p_op->arg.i_num);
	report(stdout, "%s (mmc_set_blocksize): %s\n", program_name, 
	       cdio_driver_errmsg(rc));
      } else {
	int i_blocksize = mmc_get_blocksize(p_cdio);
	if (i_blocksize > 0) {
	  report(stdout, "%s (mmc_get_blocksize): %d\n", program_name, 
		 i_blocksize);
	} else {
	  report(stdout, "%s (mmc_get_blocksize): can't retrieve.\n", 
		 program_name);
	}
      }
      break;
    case OP_MODE_SENSE_2A: 
      {
	uint8_t buf[30] = { 0, };    /* Place to hold returned data */
	if (p_op->arg.i_num == 10) {
	  rc = mmc_mode_sense_10(p_cdio, buf, sizeof(buf),
				 CDIO_MMC_CAPABILITIES_PAGE);
	} else {
	  rc = mmc_mode_sense_6(p_cdio, buf, sizeof(buf),
				 CDIO_MMC_CAPABILITIES_PAGE);
	}
	if (DRIVER_OP_SUCCESS == rc) {
	  print_mode_sense(p_op->arg.i_num, buf);
	} else {
	  report(stdout, "%s (mmc_mode_sense 2a - drive_cap %d): %s\n", 
		 program_name, p_op->arg.i_num, cdio_driver_errmsg(rc));
	}
      }
      break;
    case OP_CLOSETRAY:
      rc = mmc_close_tray(p_cdio);
      report(stdout, "%s (mmc_close_tray): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      free(p_op->arg.psz);
      break;
    case OP_EJECT:
      rc = mmc_eject_media(p_cdio);
      report(stdout, "%s (mmc_eject_media): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      if (p_op->arg.psz) free(p_op->arg.psz);
      break;
    case OP_IDLE:
      rc = mmc_start_stop_media(p_cdio, false, false, true);
      report(stdout, "%s (mmc_start_stop_media - powerdown): %s\n", 
	     program_name, cdio_driver_errmsg(rc));
      break;
    case OP_INQUIRY: 
      {
	cdio_hwinfo_t hw_info = { "", "", ""}; 
	if (mmc_get_hwinfo(p_cdio, &hw_info)) {
	  printf("%-8s: %s\n%-8s: %s\n%-8s: %s\n",
		 "Vendor"  , hw_info.psz_vendor, 
		 "Model"   , hw_info.psz_model, 
		 "Revision", hw_info.psz_revision);
	} else {
	  report(stdout, "%s (mmc_gpcmd_inquiry error)\n", program_name);
	}
      }
      break;
    case OP_MCN: 
      {
	char *psz_mcn = mmc_get_mcn(p_cdio);
	if (psz_mcn) {
	  report(stdout, "%s (mmc_get_mcn): %s\n", program_name, psz_mcn);
	  free(psz_mcn);
	} else
	  report(stdout, "%s (mmc_get_mcn): can't retrieve\n", program_name);
      }
      break;
    default:
      ;
    }
  }
  

  free(source_name);
  cdio_destroy(p_cdio);
  
  return rc;
}
