/*
  $Id: util.c,v 1.31 2005/01/09 16:33:18 rocky Exp $

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

/* Miscellaneous things common to standalone programs. */

#include "util.h"
#include <cdio/scsi_mmc.h>

cdio_log_handler_t gl_default_cdio_log_handler = NULL;
char *source_name = NULL;
char *program_name;

void 
myexit(CdIo_t *cdio, int rc) 
{
  if (NULL != cdio) cdio_destroy(cdio);
  if (NULL != program_name) free(program_name);
  if (NULL != source_name)  free(source_name);
  exit(rc);
}

void
print_version (char *program_name, const char *version, 
	       int no_header, bool version_only)
{
  
  driver_id_t driver_id;

  if (no_header == 0)
    report( stdout,  
	    "%s version %s\nCopyright (c) 2003, 2004, 2005 R. Bernstein\n",
	    program_name, version);
  report( stdout,  
	  _("This is free software; see the source for copying conditions.\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
"));

  if (version_only) {
    char *default_device;
    for (driver_id=DRIVER_UNKNOWN+1; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
      if (cdio_have_driver(driver_id)) {
	report( stdout, "Have driver: %s\n", cdio_driver_describe(driver_id));
      }
    }
    default_device=cdio_get_default_device(NULL);
    if (default_device)
      report( stdout, "Default CD-ROM device: %s\n", default_device);
    else
      report( stdout, "No CD-ROM device found.\n");
    free(program_name);
    exit(100);
  }
  
}

/*! Device input routine. If successful we return an open CdIo_t
    pointer. On error the program exits.
 */
CdIo_t *
open_input(const char *psz_source, source_image_t source_image,
	   const char *psz_access_mode)
{
  CdIo_t *p_cdio = NULL;
  switch (source_image) {
  case INPUT_UNKNOWN:
  case INPUT_AUTO:
    p_cdio = cdio_open_am (psz_source, DRIVER_UNKNOWN, psz_access_mode);
    if (!p_cdio) {
      if (psz_source) {
	err_exit("Error in automatically selecting driver for input %s.\n", 
		 psz_source);
      } else {
	err_exit("%s", "Error in automatically selecting driver.\n");
      }
    } 
    break;
  case INPUT_DEVICE:
    p_cdio = cdio_open_am (psz_source, DRIVER_DEVICE, psz_access_mode);
    if (!p_cdio) {
      if (psz_source) {
	err_exit("Cannot use CD-ROM device %s. Is a CD loaded?\n",
		 psz_source);
      } else {
	err_exit("%s", "Cannot find a CD-ROM with a CD loaded.\n");
      }
    } 
    break;
  case INPUT_BIN:
    p_cdio = cdio_open_am (psz_source, DRIVER_BINCUE, psz_access_mode);
    if (!p_cdio) {
      if (psz_source) {
	err_exit("%s: Error in opening CDRWin BIN/CUE image for BIN"
		 " input %s\n", psz_source);
      } else {
	err_exit("%s", "Cannot find CDRWin BIN/CUE image.\n");
      }
    } 
    break;
  case INPUT_CUE:
    p_cdio = cdio_open_cue(psz_source);
    if (p_cdio==NULL) {
      if (psz_source) {
	err_exit("%s: Error in opening CDRWin BIN/CUE image for CUE"
		 " input %s\n", psz_source);
      } else {
	err_exit("%s", "Cannot find CDRWin BIN/CUE image.\n");
      }
    } 
    break;
  case INPUT_NRG:
    p_cdio = cdio_open_am (psz_source, DRIVER_NRG, psz_access_mode);
    if (p_cdio==NULL) {
      if (psz_source) {
	err_exit("Error in opening Nero NRG image for input %s\n", 
		 psz_source);
      } else {
	err_exit("%s", "Cannot find Nero NRG image.\n");
      }
    } 
    break;

  case INPUT_CDRDAO:
    p_cdio = cdio_open_am (psz_source, DRIVER_CDRDAO, psz_access_mode);
    if (p_cdio==NULL) {
      if (psz_source) {
	err_exit("Error in opening cdrdao TOC with input %s.\n", psz_source);
      } else {
	err_exit("%s", "Cannot find cdrdao TOC image.\n");
      }
    }
    break;
  }
  return p_cdio;
}


#define DEV_PREFIX "/dev/"
char *
fillout_device_name(const char *device_name) 
{
#if defined(HAVE_WIN32_CDROM)
  return strdup(device_name);
#else
  unsigned int prefix_len = strlen(DEV_PREFIX);
  if (!device_name) return NULL;
  if (0 == strncmp(device_name, DEV_PREFIX, prefix_len))
    return strdup(device_name);
  else {
    char *full_device_name = (char*) malloc(strlen(device_name)+prefix_len);
    report( stdout, full_device_name, DEV_PREFIX "%s", device_name);
    return full_device_name;
  }
#endif
}

/*! Prints out SCSI-MMC drive features  */
void 
print_mmc_drive_features(CdIo_t *p_cdio)
{
  
  int i_status;                  /* Result of SCSI MMC command */
  uint8_t buf[500] = { 0, };         /* Place to hold returned data */
  scsi_mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Block */
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_GET_CONFIGURATION);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));
  cdb.field[1] = CDIO_MMC_GET_CONF_ALL_FEATURES;
  cdb.field[3] = 0x0;
  
  i_status = scsi_mmc_run_cmd(p_cdio, 0, 
			      &cdb, SCSI_MMC_DATA_READ, 
			      sizeof(buf), &buf);
  if (i_status == 0) {
    uint8_t *p;
    uint32_t i_data;
    uint8_t *p_max = buf + 65530;
    
    i_data  = (unsigned int) CDIO_MMC_GET_LEN32(buf);

    /* set to first sense feature code, and then walk through the masks */
    p = buf + 8;
    while( (p < &(buf[i_data])) && (p < p_max) ) {
      uint16_t i_feature;
      uint8_t i_feature_additional = p[3];
      
      i_feature = CDIO_MMC_GET_LEN16(p);
      switch( i_feature )
	{
	  uint8_t *q;
	case CDIO_MMC_FEATURE_PROFILE_LIST:
	  report( stdout, "Profile List Feature\n");
	  for ( q = p+4 ; q < p + i_feature_additional ; q += 4 ) {
	    int i_profile=CDIO_MMC_GET_LEN16(q);
	    switch (i_profile) {
	    case CDIO_MMC_FEATURE_PROF_NON_REMOVABLE:
	      report( stdout, 
		      "\tRe-writable disk, capable of changing behavior");
	      break;
	    case CDIO_MMC_FEATURE_PROF_REMOVABLE:
	      report( stdout, 
		      "\tdisk Re-writable; with removable media");
	      break;
	    case CDIO_MMC_FEATURE_PROF_MO_ERASABLE:
	      report( stdout, 
		      "\tErasable Magneto-Optical disk with sector erase capability");
	      break;
	    case CDIO_MMC_FEATURE_PROF_MO_WRITE_ONCE:
	      report( stdout, 
		      "\tWrite Once Magneto-Optical write once");
	      break;
	    case CDIO_MMC_FEATURE_PROF_AS_MO:
	      report( stdout, 
		      "\tAdvance Storage Magneto-Optical");
	      break;
	    case CDIO_MMC_FEATURE_PROF_CD_ROM:
	      report( stdout, 
		      "\tRead only Compact Disc capable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_CD_R:
	      report( stdout, 
		      "\tWrite once Compact Disc capable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_CD_RW:
	      report( stdout, 
		      "\tCD-RW Re-writable Compact Disc capable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_ROM:
	      report( stdout, 
		      "\tRead only DVD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_R_SEQ:
	      report( stdout, 
		      "\tRe-recordable DVD using Sequential recording");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_RAM:
	      report( stdout, 
		      "\tRe-writable DVD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_RW_RO:
	      report( stdout, 
		      "\tRe-recordable DVD using Restricted Overwrite");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_RW_SEQ:
	      report( stdout, 
		      "\tRe-recordable DVD using Sequential recording");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_PRW:
	      report( stdout, 
		      "\tDVD+RW - DVD ReWritable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_PR:
	      report( stdout, 
		      "\tDVD+R - DVD Recordable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DDCD_ROM:
	      report( stdout, 
		      "\tRead only DDCD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DDCD_R:
	      report( stdout, 
		      "\tDDCD-R Write only DDCD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DDCD_RW:
	      report( stdout, 
		      "\tRe-Write only DDCD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_NON_CONFORM:
	      report( stdout, 
		      "\tThe Logical Unit does not conform to any Profile.");
	      break;
	    default: 
	      report( stdout, 
		      "\tUnknown Profile %x", i_profile);
	      break;
	    }
	    if (q[2] & 1) {
	      report( stdout, " - on");
	    }
	    report( stdout, "\n");
	  }
	  report( stdout, "\n");
	  
	  break;
	case CDIO_MMC_FEATURE_CORE: 
	  {
	    uint8_t *q = p+4;
	    uint32_t 	i_interface_standard = CDIO_MMC_GET_LEN32(q);
	    report( stdout, "Core Feature\n");
	    switch(i_interface_standard) {
	    case 0: 
	      report( stdout, "\tunspecified interface\n");
	      break;
	    case 1: 
	      report( stdout, "\tSCSI interface\n");
	      break;
	    case 2: 
	      report( stdout, "\tATAPI interface\n");
	      break;
	    case 3: 
	      report( stdout, "\tIEEE 1394 interface\n");
	      break;
	    case 4:
	      report( stdout, "\tIEEE 1394A interface\n");
	      break;
	    case 5:
	      report( stdout, "\tFibre Channel interface\n");
	    }
	    report( stdout, "\n");
	    break;
	  }
	case CDIO_MMC_FEATURE_REMOVABLE_MEDIUM:
	  report( stdout, "Removable Medium Feature\n" );
	  switch(p[4] >> 5) {
	  case 0:
	    report( stdout, 
		    "\tCaddy/Slot type loading mechanism\n" );
	    break;
	  case 1:
	    report( stdout, 
		    "\tTray type loading mechanism\n" );
	    break;
	  case 2:
	    report( stdout, "\tPop-up type loading mechanism\n");
	    break;
	  case 4:
	    report( stdout, 
		    "\tEmbedded changer with individually changeable discs\n");
	    break;
	  case 5:
	    report( stdout, 
		    "\tEmbedded changer using a magazine mechanism\n" );
	    break;
	  default:
	    report( stdout, 
		    "\tUnknown changer mechanism\n" );
	  }
	  
	  report( stdout, 
		  "\tcan%s eject the medium or magazine via the normal "
		 "START/STOP command\n", 
		 (p[4] & 8) ? "": "not");
	  report( stdout, "\tcan%s be locked into the Logical Unit\n", 
		 (p[4] & 1) ? "": "not");
	  report( stdout, "\n" );
	  break;
	case CDIO_MMC_FEATURE_WRITE_PROTECT:
	  report( stdout, "Write Protect Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_RANDOM_READABLE:
	  report( stdout, "Random Readable Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_MULTI_READ:
	  report( stdout, "Multi-Read Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_CD_READ:
	  report( stdout, "CD Read Feature\n" );
	  report( stdout, "\tC2 Error pointers are %ssupported\n", 
		 (p[4] & 2) ? "": "not ");
	  report( stdout, "\tCD-Text is %ssupported\n", 
		 (p[4] & 1) ? "": "not ");
	  report( stdout, "\n" );
	  break;
	case CDIO_MMC_FEATURE_DVD_READ:
	  report( stdout, "DVD Read Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_RANDOM_WRITABLE:
	  report( stdout, "Random Writable Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_INCR_WRITE:
	  report( stdout, "Incremental Streaming Writable Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_SECTOR_ERASE:
	  report( stdout, "Sector Erasable Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_FORMATABLE:
	  report( stdout, "Formattable Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_DEFECT_MGMT:
	  report( stdout, 
		  "Management Ability of the Logical Unit/media system "
		 "to provide an apparently defect-free space.\n");
	  break;
	case CDIO_MMC_FEATURE_WRITE_ONCE:
	  report( stdout, "Write Once Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_RESTRICT_OVERW:
	  report( stdout, "Restricted Overwrite Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_CD_RW_CAV:
	  report( stdout, "CD-RW CAV Write Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_MRW:
	  report( stdout, "MRW Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_DVD_PRW:
	  report( stdout, "DVD+RW Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_DVD_PR:
	  report( stdout, "DVD+R Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_CD_TAO:
	  report( stdout, "CD Track at Once Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_CD_SAO:
	  report( stdout, "CD Mastering (Session at Once) Feature\n" );
	  break;
	case CDIO_MMC_FEATURE_POWER_MGMT:
	  report( stdout, 
		  "Initiator and device directed power management\n" );
	  break;
	case CDIO_MMC_FEATURE_CDDA_EXT_PLAY:
	  report( stdout, "CD Audio External Play Feature\n" );
	  report( stdout, "\tSCAN command is %ssupported\n", 
		 (p[4] & 4) ? "": "not ");
	  report( stdout, 
		  "\taudio channels can %sbe muted separately\n", 
		 (p[4] & 2) ? "": "not ");
	  report( stdout, 
		  "\taudio channels can %shave separate volume levels\n", 
		 (p[4] & 1) ? "": "not ");
	  {
	    uint8_t *q = p+6;
	    uint16_t i_vol_levels = CDIO_MMC_GET_LEN16(q);
	    report( stdout, "\t%d volume levels can be set\n", i_vol_levels );
	  }
	  report( stdout, "\n");
	  break;
	case CDIO_MMC_FEATURE_MCODE_UPGRADE:
	  report( stdout, "Ability for the device to accept new microcode via "
		 "the interface\n");
	  break;
	case CDIO_MMC_FEATURE_TIME_OUT:
	  report( stdout, "Ability to respond to all commands within a "
		 "specific time\n");
	  break;
	case CDIO_MMC_FEATURE_DVD_CSS:
	  report( stdout, "Ability to perform DVD CSS/CPPM authentication and"
		 " RPC\n");
#if 0
	  report( stdout, "\tMedium does%s have Content Scrambling (CSS/CPPM)\n", 
		 (p[2] & 1) ? "": "not ");
#endif
	  report( stdout, "\tCSS version %d\n", p[7] );
	  report( stdout, "\t\n");
	  break;
	case CDIO_MMC_FEATURE_RT_STREAMING:
	  report( stdout, 
		  "Ability to read and write using Initiator requested performance parameters\n");
	  break;
	case CDIO_MMC_FEATURE_LU_SN: {
	  uint8_t i_serial = *(p+3);
	  char serial[257] = { '\0', };
	      
	  report( stdout, "The Logical Unit has a unique identifier:\n" );
	  memcpy(serial, p+4, i_serial);
	  report( stdout, "\t%s\n\n", serial );

	  break;
	}
	default: 
	  if ( 0 != (i_feature & 0xFF00) ) {
	    report( stdout, "Vendor-specific feature code %x\n", i_feature );
	  } else {
	    report( stdout, "Unknown feature code %x\n", i_feature );
	  }
	  
	}
      p += i_feature_additional + 4;
    }
  } else {
    report( stdout, "Didn't get all feature codes\n");
  }
}


/* Prints out drive capabilities */
void 
print_drive_capabilities(cdio_drive_read_cap_t  i_read_cap,
			 cdio_drive_write_cap_t i_write_cap,
			 cdio_drive_misc_cap_t  i_misc_cap)
{
  if (CDIO_DRIVE_CAP_ERROR == i_misc_cap) {
    report( stdout, "Error in getting drive hardware properties\n");
  }  else if (CDIO_DRIVE_CAP_UNKNOWN == i_misc_cap) {
    report( stdout, "Uknown drive hardware properties\n");
  } else {
    report( stdout, _("Hardware                    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_FILE  
	   ? "Disk Image"  : "CD-ROM or DVD");
    report( stdout, _("Can eject                   : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_EJECT         ? "Yes" : "No");
    report( stdout, _("Can close tray              : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_CLOSE_TRAY    ? "Yes" : "No");
    report( stdout, _("Can disable manual eject    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_LOCK          ? "Yes" : "No");
    report( stdout, _("Can select juke-box disc    : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_DISC   ? "Yes" : "No");

    report( stdout, _("Can set drive speed         : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_SPEED  ? "Yes" : "No");
#if FIXED
    report( stdout, _("Can detect if CD changed    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED ? "Yes" : "No");
#endif
    report( stdout, _("Can read multiple sessions  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MULTI_SESSION ? "Yes" : "No");
    report( stdout, _("Can hard reset device       : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_RESET         ? "Yes" : "No");
  }
  
    
  if (CDIO_DRIVE_CAP_ERROR == i_read_cap) {
      report( stdout, "Error in getting drive reading properties\n" );
  }  else if (CDIO_DRIVE_CAP_UNKNOWN == i_misc_cap) {
    report( stdout, "Uknown drive reading properties\n" );
  } else {
    report( stdout, "Reading....\n");
    report( stdout, _("  Can read Mode 2 Form 1    : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_MODE2_FORM1 ? "Yes" : "No" );
    report( stdout, _("  Can read Mode 2 Form 2    : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_MODE2_FORM2 ? "Yes" : "No" );
    report( stdout, _("  Can read C2 Errors        : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_C2_ERRS     ? "Yes" : "No" );
    report( stdout, _("  Can read IRSC             : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_ISRC        ? "Yes" : "No" );
    report( stdout, _("  Can play audio            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_AUDIO      ? "Yes" : "No");
    report( stdout, _("  Can read CD-DA            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_DA       ? "Yes" : "No" );
    report( stdout, _("  Can read  CD-R            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_R       ? "Yes" : "No");
    report( stdout, _("  Can read  CD-RW           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW      ? "Yes" : "No");
    report( stdout, _("  Can read  DVD-ROM         : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM    ? "Yes" : "No");
  }
  

  if (CDIO_DRIVE_CAP_ERROR == i_write_cap) {
      report( stdout, "Error in getting drive writing properties\n" );
  }  else if (CDIO_DRIVE_CAP_UNKNOWN == i_misc_cap) {
    report( stdout, "Uknown drive writing properties\n" );
  } else {
    report( stdout, "\nWriting....\n");
    report( stdout, _("  Can write using Burn Proof: %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_BURN_PROOF ? "Yes" : "No" );
    report( stdout, _("  Can write CD-RW           : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_CD_RW     ? "Yes" : "No");
    report( stdout, _("  Can write DVD-R           : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_R    ? "Yes" : "No");
    report( stdout, _("  Can write DVD-RAM         : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RAM  ? "Yes" : "No");
    report( stdout, _("  Can write DVD-RW          : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RW   ? "Yes" : "No");
    report( stdout, _("  Can write DVD+RW          : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RPW  ? "Yes" : "No");
  }
}

/*! Common place for output routine. In some environments, like XBOX,
  it may not be desireable to send output to stdout and stderr. */
void 
report (FILE *stream, const char *psz_format,  ...)
{
  va_list args;
  va_start (args, psz_format);
#ifdef _XBOX
  OutputDebugString(psz_format, args);
#else
  vfprintf (stream, psz_format, args);
#endif
  va_end(args);
}

