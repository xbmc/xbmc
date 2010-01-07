/*
  $Id: scsi-mmc2.c,v 1.3 2005/01/29 20:54:20 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  
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
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/scsi_mmc.h>
#include <string.h>

/* Set how long do wto wait for SCSI-MMC commands to complete */
#define DEFAULT_TIMEOUT_MS 10000

int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio;

  p_cdio = cdio_open (NULL, DRIVER_UNKNOWN);

  if (NULL == p_cdio) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    int i_status;                  /* Result of MMC command */
    uint8_t buf[500] = { 0, };         /* Place to hold returned data */
    scsi_mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Buffer */

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

      i_data = (unsigned int) CDIO_MMC_GET_LEN32(buf);
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
	    printf("Profile List Feature\n");
	    for ( q = p+4 ; q < p + i_feature_additional ; q += 4 ) {
	      int i_profile=CDIO_MMC_GET_LEN16(q);
	      switch (i_profile) {
	      case CDIO_MMC_FEATURE_PROF_NON_REMOVABLE:
		printf("\tRe-writable disk, capable of changing behavior");
		break;
	      case CDIO_MMC_FEATURE_PROF_REMOVABLE:
		printf("\tdisk Re-writable; with removable media");
		break;
	      case CDIO_MMC_FEATURE_PROF_MO_ERASABLE:
		printf("\tErasable Magneto-Optical disk with sector erase capability");
		break;
	      case CDIO_MMC_FEATURE_PROF_MO_WRITE_ONCE:
		printf("\tWrite Once Magneto-Optical write once");
		break;
	      case CDIO_MMC_FEATURE_PROF_AS_MO:
		printf("\tAdvance Storage Magneto-Optical");
		break;
	      case CDIO_MMC_FEATURE_PROF_CD_ROM:
		printf("\tRead only Compact Disc capable");
		break;
	      case CDIO_MMC_FEATURE_PROF_CD_R:
		printf("\tWrite once Compact Disc capable");
		break;
	      case CDIO_MMC_FEATURE_PROF_CD_RW:
		printf("\tCD-RW Re-writable Compact Disc capable");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_ROM:
		printf("\tRead only DVD");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_R_SEQ:
		printf("\tRe-recordable DVD using Sequential recording");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_RAM:
		printf("\tRe-writable DVD");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_RW_RO:
		printf("\tRe-recordable DVD using Restricted Overwrite");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_RW_SEQ:
		printf("\tRe-recordable DVD using Sequential recording");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_PRW:
		printf("\tDVD+RW - DVD ReWritable");
		break;
	      case CDIO_MMC_FEATURE_PROF_DVD_PR:
		printf("\tDVD+R - DVD Recordable");
		break;
	      case CDIO_MMC_FEATURE_PROF_DDCD_ROM:
		printf("\tRead only DDCD");
		break;
	      case CDIO_MMC_FEATURE_PROF_DDCD_R:
		printf("\tDDCD-R Write only DDCD");
		break;
	      case CDIO_MMC_FEATURE_PROF_DDCD_RW:
		printf("\tRe-Write only DDCD");
		break;
	      case CDIO_MMC_FEATURE_PROF_NON_CONFORM:
		printf("\tThe Logical Unit does not conform to any Profile.");
		break;
	      default: 
		printf("\tUnknown Profile %x", i_profile);
		break;
	      }
	      if (q[2] & 1) {
		printf(" - on");
	      }
	      printf("\n");
	    }
	    printf("\n");
	    
	    break;
	  case CDIO_MMC_FEATURE_CORE: 
	    {
	      uint8_t *q = p+4;
	      uint32_t 	i_interface_standard = CDIO_MMC_GET_LEN32(q);
	      printf("Core Feature\n");
	      switch(i_interface_standard) {
	      case 0: 
		printf("\tunspecified interface\n");
		break;
	      case 1: 
		printf("\tSCSI interface\n");
		break;
	      case 2: 
		printf("\tATAPI interface\n");
		break;
	      case 3: 
		printf("\tIEEE 1394 interface\n");
		break;
	      case 4:
		printf("\tIEEE 1394A interface\n");
		break;
	      case 5:
		printf("\tFibre Channel interface\n");
	      }
	      printf("\n");
	      break;
	    }
	  case CDIO_MMC_FEATURE_REMOVABLE_MEDIUM:
	    printf("Removable Medium Feature\n");
	    switch(p[4] >> 5) {
	    case 0:
	      printf("\tCaddy/Slot type loading mechanism\n");
	      break;
	    case 1:
	      printf("\tTray type loading mechanism\n");
	      break;
	    case 2:
	      printf("\tPop-up type loading mechanism\n");
	      break;
	    case 4:
	      printf("\tEmbedded changer with individually changeable discs\n");
	      break;
	    case 5:
	      printf("\tEmbedded changer using a magazine mechanism\n");
	      break;
	    default:
	      printf("\tUnknown changer mechanism\n");
	    }
	    
	    printf("\tcan%s eject the medium or magazine via the normal "
		   "START/STOP command\n", 
		   (p[4] & 8) ? "": "not");
	    printf("\tcan%s be locked into the Logical Unit\n", 
		   (p[4] & 1) ? "": "not");
	    printf("\n");
	    break;
	  case CDIO_MMC_FEATURE_WRITE_PROTECT:
	    printf("Write Protect Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_RANDOM_READABLE:
	    printf("Random Readable Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_MULTI_READ:
	    printf("Multi-Read Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_CD_READ:
	    printf("CD Read Feature\n");
	    printf("\tC2 Error pointers are %ssupported\n", 
		   (p[4] & 2) ? "": "not ");
	    printf("\tCD-Text is %ssupported\n", 
		   (p[4] & 1) ? "": "not ");
	    printf("\n");
	    break;
	  case CDIO_MMC_FEATURE_DVD_READ:
	    printf("DVD Read Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_RANDOM_WRITABLE:
	    printf("Random Writable Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_INCR_WRITE:
	    printf("Incremental Streaming Writable Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_SECTOR_ERASE:
	    printf("Sector Erasable Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_FORMATABLE:
	    printf("Formattable Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_DEFECT_MGMT:
	    printf("Management Ability of the Logical Unit/media system "
		 "to provide an apparently defect-free space.\n");
	    break;
	  case CDIO_MMC_FEATURE_WRITE_ONCE:
	    printf("Write Once Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_RESTRICT_OVERW:
	    printf("Restricted Overwrite Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_CD_RW_CAV:
	    printf("CD-RW CAV Write Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_MRW:
	    printf("MRW Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_DVD_PRW:
	    printf("DVD+RW Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_DVD_PR:
	    printf("DVD+R Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_CD_TAO:
	    printf("CD Track at Once Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_CD_SAO:
	    printf("CD Mastering (Session at Once) Feature\n");
	    break;
	  case CDIO_MMC_FEATURE_POWER_MGMT:
	    printf("Initiator and device directed power management\n");
	    break;
	  case CDIO_MMC_FEATURE_CDDA_EXT_PLAY:
	    printf("CD Audio External Play Feature\n");
	    printf("\tSCAN command is %ssupported\n", 
		   (p[4] & 4) ? "": "not ");
	    printf("\taudio channels can %sbe muted separately\n", 
		   (p[4] & 2) ? "": "not ");
	    printf("\taudio channels can %shave separate volume levels\n", 
		   (p[4] & 1) ? "": "not ");
	    {
	      uint8_t *q = p+6;
	      uint16_t i_vol_levels = CDIO_MMC_GET_LEN16(q);
	      printf("\t%d volume levels can be set\n", i_vol_levels);
	    }
	    printf("\n");
	    break;
	  case CDIO_MMC_FEATURE_MCODE_UPGRADE:
	    printf("Ability for the device to accept new microcode via "
		   "the interface\n");
	    break;
	  case CDIO_MMC_FEATURE_TIME_OUT:
	    printf("Ability to respond to all commands within a "
		   "specific time\n");
	    break;
          case CDIO_MMC_FEATURE_DVD_CSS:
	    printf("Ability to perform DVD CSS/CPPM authentication and"
		   " RPC\n");
	    break;
	  case CDIO_MMC_FEATURE_RT_STREAMING:
	    printf("Ability to read and write using Initiator requested performance parameters\n");
	    break;
	  case CDIO_MMC_FEATURE_LU_SN: {
	    uint8_t i_serial = *(p+3);
	    char serial[257] = { '\0', };
	    
	    printf("The Logical Unit has a unique identifier:\n");
	    memcpy(serial, p+4, i_serial);
	    printf("\t%s\n\n", serial);
	    
	    break;
	  }
	  default: 
	    if ( 0 != (i_feature & 0xFF00) ) {
	      printf("Vendor-specific feature code %x\n", i_feature);
	    } else {
	      printf("Unknown feature code %x\n", i_feature);
	    }
	  }
	p += i_feature_additional + 4;
      }
    } else {
      printf("Didn't get all feature codes\n");
    }
  }
  
  cdio_destroy(p_cdio);
  
  return 0;
}
