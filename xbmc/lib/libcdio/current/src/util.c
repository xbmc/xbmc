/*
  $Id: util.c,v 1.53 2008/03/06 01:16:49 rocky Exp $

  Copyright (C) 2003, 2004, 2005, 2007, 2008 Rocky Bernstein <rocky@gnu.org>
  
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
#include <cdio/mmc.h>
#include <cdio/bytesex.h>

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

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
	    "%s version %s\nCopyright (c) 2003, 2004, 2005, 2007, 2008 R. Bernstein\n",
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
    exit(EXIT_INFO);
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
    char *full_device_name = (char*) calloc(1, strlen(device_name)+prefix_len);
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
  uint8_t buf[500] = { 0, };     /* Place to hold returned data */
  mmc_cdb_t cdb = {{0, }};       /* Command Descriptor Block */
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_GET_CONFIGURATION);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));
  cdb.field[1] = CDIO_MMC_GET_CONF_ALL_FEATURES;
  cdb.field[3] = 0x0;
  
  i_status = mmc_run_cmd(p_cdio, 0, &cdb, SCSI_MMC_DATA_READ, sizeof(buf), 
			 &buf);
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
      {
	uint8_t *q;
	const char *feature_str = mmc_feature2str(i_feature);
	report( stdout, "%s Feature\n", feature_str);
	switch( i_feature )
	  {
	  case CDIO_MMC_FEATURE_PROFILE_LIST:
	    for ( q = p+4 ; q < p + i_feature_additional ; q += 4 ) {
	      int i_profile=CDIO_MMC_GET_LEN16(q);
	      const char *feature_profile_str = 
		mmc_feature_profile2str(i_profile);
	      report( stdout, "\t%s", feature_profile_str );
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
	      switch(i_interface_standard) {
	      case CDIO_MMC_FEATURE_INTERFACE_UNSPECIFIED: 
		report( stdout, "\tunspecified interface\n");
		break;
	      case CDIO_MMC_FEATURE_INTERFACE_SCSI:
		report( stdout, "\tSCSI interface\n");
		break;
	      case CDIO_MMC_FEATURE_INTERFACE_ATAPI: 
		report( stdout, "\tATAPI interface\n");
		break;
	      case CDIO_MMC_FEATURE_INTERFACE_IEEE_1394: 
		report( stdout, "\tIEEE 1394 interface\n");
		break;
	      case CDIO_MMC_FEATURE_INTERFACE_IEEE_1394A: 
		report( stdout, "\tIEEE 1394A interface\n");
		break;
	      case CDIO_MMC_FEATURE_INTERFACE_FIBRE_CH: 
		report( stdout, "\tFibre Channel interface\n");
	      }
	      report( stdout, "\n");
	      break;
	    }
	  case CDIO_MMC_FEATURE_MORPHING:
	    report( stdout, 
		    "\tOperational Change Request/Notification %ssupported\n",
		    (p[4] & 2) ? "": "not " );
	    report( stdout, "\t%synchronous GET EVENT/STATUS NOTIFICATION "
		    "supported\n", 
		    (p[4] & 1) ? "As": "S" );
	    report( stdout, "\n");
	    break;
	    ;
	    
	  case CDIO_MMC_FEATURE_REMOVABLE_MEDIUM:
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
		    (p[4] & 8) ? "": "not" );
	    report( stdout, "\tcan%s be locked into the Logical Unit\n", 
		    (p[4] & 1) ? "": "not" );
	    report( stdout, "\n" );
	    break;
	  case CDIO_MMC_FEATURE_CD_READ:
	    report( stdout, "\tC2 Error pointers are %ssupported\n", 
		    (p[4] & 2) ? "": "not " );
	    report( stdout, "\tCD-Text is %ssupported\n", 
		    (p[4] & 1) ? "": "not " );
	    report( stdout, "\n" );
	    break;
	  case CDIO_MMC_FEATURE_ENHANCED_DEFECT:
	    report( stdout, "\t%s-DRM mode is supported\n", 
		    (p[4] & 1) ? "DRT": "Persistent" );
	    report( stdout, "\n" );
	    break;
	  case CDIO_MMC_FEATURE_CDDA_EXT_PLAY:
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
	  case CDIO_MMC_FEATURE_DVD_CSS:
#if 0
	    report( stdout, "\tMedium does%s have Content Scrambling (CSS/CPPM)\n", 
		    (p[2] & 1) ? "": "not " );
#endif
	    report( stdout, "\tCSS version %d\n", p[7] );
	    report( stdout, "\t\n");
	    break;
	  case CDIO_MMC_FEATURE_LU_SN: {
	    uint8_t i_serial = *(p+3);
	    char serial[257] = { '\0', };
	    
	    memcpy(serial, p+4, i_serial );
	    report( stdout, "\t%s\n\n", serial );
	    
	    break;
	  }
	  default: 
	    report( stdout, "\n");
	    break;
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
    report( stdout, _("Hardware                                  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_FILE  
	   ? "Disk Image"  : "CD-ROM or DVD");
    report( stdout, _("Can eject                                 : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_EJECT        ? "Yes" : "No" );
    report( stdout, _("Can close tray                            : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_CLOSE_TRAY   ? "Yes" : "No" );
    report( stdout, _("Can disable manual eject                  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_LOCK         ? "Yes" : "No" );
    report( stdout, _("Can select juke-box disc                  : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_DISC  ? "Yes" : "No" );

    report( stdout, _("Can set drive speed                       : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_SPEED ? "Yes" : "No" );
#if FIXED
    report( stdout, _("Can detect if CD changed                  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED ? "Yes" : "No" );
#endif
    report( stdout, _("Can read multiple sessions (e.g. PhotoCD) : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MULTI_SESSION ? "Yes" : "No" );
    report( stdout, _("Can hard reset device                     : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_RESET        ? "Yes" : "No" );
  }
  
    
  if (CDIO_DRIVE_CAP_ERROR == i_read_cap) {
      report( stdout, "Error in getting drive reading properties\n" );
  }  else if (CDIO_DRIVE_CAP_UNKNOWN == i_misc_cap) {
    report( stdout, "Uknown drive reading properties\n" );
  } else {
    report( stdout, "Reading....\n");
    report( stdout, _("  Can read Mode 2 Form 1                  : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_MODE2_FORM1  ? "Yes" : "No" );
    report( stdout, _("  Can read Mode 2 Form 2                  : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_MODE2_FORM2  ? "Yes" : "No" );
    report( stdout, _("  Can read (S)VCD (i.e. Mode 2 Form 1/2)  : %s\n"), 
	   i_read_cap & 
	    (CDIO_DRIVE_CAP_READ_MODE2_FORM1|CDIO_DRIVE_CAP_READ_MODE2_FORM2)
	    ? "Yes" : "No" );
    report( stdout, _("  Can read C2 Errors                      : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_C2_ERRS      ? "Yes" : "No" );
    report( stdout, _("  Can read IRSC                           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_ISRC         ? "Yes" : "No" );
    report( stdout, _("  Can read Media Channel Number (or UPC)  : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_MCN          ? "Yes" : "No" );
    report( stdout, _("  Can play audio                          : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_AUDIO        ? "Yes" : "No" );
    report( stdout, _("  Can read CD-DA                          : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_DA        ? "Yes" : "No" );
    report( stdout, _("  Can read CD-R                           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_R         ? "Yes" : "No" );
    report( stdout, _("  Can read CD-RW                          : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW        ? "Yes" : "No" );
    report( stdout, _("  Can read DVD-ROM                        : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM      ? "Yes" : "No" );
  }
  

  if (CDIO_DRIVE_CAP_ERROR == i_write_cap) {
      report( stdout, "Error in getting drive writing properties\n" );
  }  else if (CDIO_DRIVE_CAP_UNKNOWN == i_misc_cap) {
    report( stdout, "Uknown drive writing properties\n" );
  } else {
    report( stdout, "\nWriting....\n");
#if FIXED
    report( stdout, _("  Can write using Burn Proof              : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_BURN_PROOF ? "Yes" : "No" );
#endif
    report( stdout, _("  Can write CD-RW                         : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_CD_RW      ? "Yes" : "No" );
    report( stdout, _("  Can write DVD-R                         : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_R      ? "Yes" : "No" );
    report( stdout, _("  Can write DVD-RAM                       : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RAM    ? "Yes" : "No" );
    report( stdout, _("  Can write DVD-RW                        : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RW     ? "Yes" : "No" );
    report( stdout, _("  Can write DVD+RW                        : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RPW    ? "Yes" : "No" );
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

/* Prints "ls"-like file attributes */
void 
print_fs_attrs(iso9660_stat_t *p_statbuf, bool b_rock, bool b_xa, 
	       const char *psz_name_untranslated, 
	       const char *psz_name_translated)
{
  char date_str[30];

#ifdef HAVE_ROCK
  if (yep == p_statbuf->rr.b3_rock && b_rock) {
    report ( stdout, "  %s %3d %d %d [LSN %6lu] %9u",
	     iso9660_get_rock_attr_str (p_statbuf->rr.st_mode),
	     p_statbuf->rr.st_nlinks,
	     p_statbuf->rr.st_uid,
	     p_statbuf->rr.st_gid,
	     (long unsigned int) p_statbuf->lsn,
	     S_ISLNK(p_statbuf->rr.st_mode) 
	     ? strlen(p_statbuf->rr.psz_symlink)
	     : (unsigned int) p_statbuf->size );

  } else 
#endif
  if (b_xa) {
    report ( stdout, "  %s %d %d [fn %.2d] [LSN %6lu] ",
	     iso9660_get_xa_attr_str (p_statbuf->xa.attributes),
	     uint16_from_be (p_statbuf->xa.user_id),
	     uint16_from_be (p_statbuf->xa.group_id),
	     p_statbuf->xa.filenum,
	     (long unsigned int) p_statbuf->lsn );
    
    if (uint16_from_be(p_statbuf->xa.attributes) & XA_ATTR_MODE2FORM2) {
      report ( stdout, "%9u (%9u)",
	       (unsigned int) p_statbuf->secsize * M2F2_SECTOR_SIZE,
	       (unsigned int) p_statbuf->size );
    } else 
      report (stdout, "%9u", (unsigned int) p_statbuf->size);
  } else {
    report ( stdout,"  %c [LSN %6lu] %9u", 
	     (p_statbuf->type == _STAT_DIR) ? 'd' : '-',
	     (long unsigned int) p_statbuf->lsn,
	     (unsigned int) p_statbuf->size );
  }

  if (yep == p_statbuf->rr.b3_rock && b_rock) {
    struct tm tm;

    strftime(date_str, sizeof(date_str), "%b %d %Y %H:%M:%S ", &p_statbuf->tm);

    /* Now try the proper field for mtime: attributes  */
    if (p_statbuf->rr.modify.b_used) {
      if (p_statbuf->rr.modify.b_longdate) {
	iso9660_get_ltime(&p_statbuf->rr.modify.t.ltime, &tm);
      } else {
	iso9660_get_dtime(&p_statbuf->rr.modify.t.dtime, true, &tm);
      }
      strftime(date_str, sizeof(date_str), "%b %d %Y %H:%M:%S ", &tm);
    }
    
    report (stdout," %s %s", date_str, psz_name_untranslated );

    if (S_ISLNK(p_statbuf->rr.st_mode)) {
      report(stdout, " -> %s", p_statbuf->rr.psz_symlink);
    }

  } else {
    strftime(date_str, sizeof(date_str), "%b %d %Y %H:%M:%S ", &p_statbuf->tm);
    report (stdout," %s %s", date_str, psz_name_translated);
  }

  report(stdout, "\n");
}
