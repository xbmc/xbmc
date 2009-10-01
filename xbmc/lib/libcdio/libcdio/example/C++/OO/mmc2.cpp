/*
  $Id: mmc2.cpp,v 1.1 2005/11/14 01:16:25 rocky Exp $

  Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
  
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
#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <cdio++/cdio.hpp>

/* Set how long do wto wait for SCSI-MMC commands to complete */
#define DEFAULT_TIMEOUT_MS 10000

int
main(int argc, const char *argv[])
{
  CdioDevice device;
  const char *psz_drive = NULL;

  if (argc > 1) psz_drive = argv[1];

  if (!device.open(psz_drive)) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    int i_status;              /* Result of MMC command */
    uint8_t buf[500] = { 0, }; /* Place to hold returned data */
    mmc_cdb_t cdb = {{0, }};   /* Command Descriptor Buffer */

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_GET_CONFIGURATION);
    CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));
    cdb.field[1] = CDIO_MMC_GET_CONF_ALL_FEATURES;
    cdb.field[3] = 0x0;

    i_status = device.mmcRunCmd(0, &cdb, SCSI_MMC_DATA_READ, sizeof(buf), 
				&buf);
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
	{
	  uint8_t *q;
	  const char *feature_str = mmc_feature2str(i_feature);
	  printf("%s Feature\n", feature_str);
	  switch( i_feature )
	    {
	    case CDIO_MMC_FEATURE_PROFILE_LIST:
	      for ( q = p+4 ; q < p + i_feature_additional ; q += 4 ) {
		int i_profile=CDIO_MMC_GET_LEN16(q);
		const char *feature_profile_str = 
		  mmc_feature_profile2str(i_profile);
		printf( "\t%s", feature_profile_str );
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
	    case CDIO_MMC_FEATURE_CD_READ:
	      printf("CD Read Feature\n");
	      printf("\tC2 Error pointers are %ssupported\n", 
		     (p[4] & 2) ? "": "not ");
	      printf("\tCD-Text is %ssupported\n", 
		     (p[4] & 1) ? "": "not ");
	      printf("\n");
	      break;
	    case CDIO_MMC_FEATURE_CDDA_EXT_PLAY:
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
	    case CDIO_MMC_FEATURE_LU_SN: {
	      uint8_t i_serial = *(p+3);
	      char serial[257] = { '\0', };
	      memcpy(serial, p+4, i_serial);
	      printf("\t%s\n\n", serial);
	      break;
	    }
	    default: 
	      printf("\n");
	      break;
	    }
	  p += i_feature_additional + 4;
	}
      }
    } else {
      printf("Didn't get all feature codes\n");
    }
  }
  
  return 0;
}
