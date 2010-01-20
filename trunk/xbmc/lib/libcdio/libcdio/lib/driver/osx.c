/*
    $Id: osx.c,v 1.11 2007/08/11 12:28:25 flameeyes Exp $

    Copyright (C) 2003, 2004, 2005, 2006 Rocky Bernstein 
    <rockyb@users.sourceforge.net> 
    from vcdimager code: 
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    and VideoLAN code Copyright (C) 1998-2001 VideoLAN
      Authors: Johan Bilien <jobi@via.ecp.fr>
               Gildas Bazin <gbazin@netcourrier.com>
               Jon Lech Johansen <jon-vl@nanocrew.net>
               Derk-Jan Hartman <hartman at videolan.org>
               Justin F. Hallett <thesin@southofheaven.org>

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

/* This file contains OSX-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static const char _rcsid[] = "$Id: osx.c,v 1.11 2007/08/11 12:28:25 flameeyes Exp $";

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>

/* For SCSI TR_* enumerations */
#include <cdio/cdda.h>

#include "cdio_assert.h"
#include "cdio_private.h"

#include <string.h>

#ifdef HAVE_DARWIN_CDROM
#undef VERSION 

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>

#include <mach/mach.h>
#include <Carbon/Carbon.h>
#include <IOKit/scsi-commands/SCSITaskLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <mach/mach_error.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>


#include <paths.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/scsi-commands/IOSCSIMultimediaCommandsDevice.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IODVDTypes.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#include <IOKit/storage/IODVDMediaBSDClient.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>

#ifdef HAVE_DISKARBITRATION
#include <DiskArbitration/DiskArbitration.h>
#endif

/* FIXME */
#define MAX_BIG_BUFF_SIZE  65535

#define kIOCDBlockStorageDeviceClassString              "IOCDBlockStorageDevice"

/* Note leadout is normally defined 0xAA, But on OSX 0xA0 is "lead in" while
   0xA2 is "lead out". I don't understand the distinction, and therefore
   something could be wrong. */
#define OSX_CDROM_LEADOUT_TRACK 0xA2

#define TOTAL_TRACKS    (p_env->i_last_track - p_env->gen.i_first_track + 1)

#define CDROM_CDI_TRACK 0x1
#define CDROM_XA_TRACK  0x2

typedef enum {
  _AM_NONE,
  _AM_OSX,
} access_mode_t;

#define MAX_SERVICE_NAME 1000
typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  access_mode_t access_mode;

  /* Track information */
  CDTOC *pTOC;
  int i_descriptors;
  track_t i_last_track;      /* highest track number */
  track_t i_last_session;    /* highest session number */
  track_t i_first_session;   /* first session number */
  lsn_t   *pp_lba;
  io_service_t MediaClass_service;
  char    psz_MediaClass_service[MAX_SERVICE_NAME];
  SCSITaskDeviceInterface **pp_scsiTaskDeviceInterface;

  // io_service_t obj;
  // SCSITaskDeviceInterface **scsi;
  SCSITaskInterface **scsi_task;
  MMCDeviceInterface **mmc;
  IOCFPlugInInterface **plugin;
  
  SCSI_Sense_Data sense;
  SCSITaskStatus status;
  UInt64 realized_len;


} _img_private_t;

static bool read_toc_osx (void *p_user_data);
static track_format_t get_track_format_osx(void *p_user_data, 
                                           track_t i_track);

/****
 * GetRegistryEntryProperties - Gets the registry entry properties for
 *  an io_service_t.
 *****/

static CFMutableDictionaryRef
GetRegistryEntryProperties ( io_service_t service )
{
  IOReturn                      err     = kIOReturnSuccess;
  CFMutableDictionaryRef        dict    = 0;
  
  err = IORegistryEntryCreateCFProperties (service, &dict, 
					   kCFAllocatorDefault, 0); 
  if ( err != kIOReturnSuccess )
    cdio_warn( "IORegistryEntryCreateCFProperties: 0x%08x", err );

  return dict;
}

#ifdef GET_SCSI_FIXED
static bool
get_scsi(_img_private_t *p_env)
{
  SInt32 score;
  kern_return_t err;
  HRESULT herr;
  
  err = IOCreatePlugInInterfaceForService(p_env->MediaClass_service, 
                                          kIOMMCDeviceUserClientTypeID,
                                          kIOCFPlugInInterfaceID,
                                          &p_env->plugin, 
                                          &score);

  if (err != noErr) {
    fprintf(stderr, "Error %x accessing MMC plugin.\n", err);
    return false;
    }

  herr = (*p_env->plugin) ->
    QueryInterface(p_env->plugin, CFUUIDGetUUIDBytes(kIOMMCDeviceInterfaceID),
                   (void *)&p_env->mmc);

  if (herr != S_OK) {
    fprintf(stderr, "Error %x accessing MMC interface.\n", (int) herr);
    IODestroyPlugInInterface(p_env->plugin);
    return false;
  }
  
  p_env->pp_scsiTaskDeviceInterface = 
    (*p_env->mmc)->GetSCSITaskDeviceInterface(p_env->mmc);
  
  if (!p_env->pp_scsiTaskDeviceInterface) {
    fprintf(stderr, 
            "Could not get SCSITaskkDevice interface from MMC interface.\n");
    (*p_env->mmc)->Release(p_env->mmc);
    IODestroyPlugInInterface(p_env->plugin);
    return false;
  }
  
  err = (*p_env->pp_scsiTaskDeviceInterface)->
    ObtainExclusiveAccess(p_env->pp_scsiTaskDeviceInterface);
  if (err != kIOReturnSuccess) {
    fprintf(stderr, "Could not obtain exclusive access to the device (%x).\n",
            err);
    
    if (err == kIOReturnBusy)
      fprintf(stderr, "The volume is already mounted.\n");
    else if (err == kIOReturnExclusiveAccess)
      fprintf(stderr, "Another application already has exclusive access "
              "to this device.\n");
    else
      fprintf(stderr, "I don't know why.\n");
    
    (*p_env->pp_scsiTaskDeviceInterface)->
      Release(p_env->pp_scsiTaskDeviceInterface);
    (*p_env->mmc)->Release(p_env->mmc);
    IODestroyPlugInInterface(p_env->plugin);
    return false;
  }
  
  p_env->scsi_task = 
    (*p_env->pp_scsiTaskDeviceInterface) -> 
    CreateSCSITask(p_env->pp_scsiTaskDeviceInterface);
  
  if (!p_env->scsi_task) {
    fprintf(stderr, "Could not create a SCSITask interface.\n");
    (*p_env->pp_scsiTaskDeviceInterface)->
      ReleaseExclusiveAccess(p_env->pp_scsiTaskDeviceInterface);
    (*p_env->pp_scsiTaskDeviceInterface)->
      Release(p_env->pp_scsiTaskDeviceInterface);
    (*p_env->mmc)->Release(p_env->mmc);
    IODestroyPlugInInterface(p_env->plugin);
    return false;
  }
  
  return true;
}
#endif

static bool 
init_osx(_img_private_t *p_env) {
  char *psz_devname;
  kern_return_t ret;
  io_iterator_t iterator;

  // only open if not already opened. otherwise too many descriptors are holding the device busy.  
  if (-1 == p_env->gen.fd)
    p_env->gen.fd = open( p_env->gen.source_name, O_RDONLY | O_NONBLOCK );

  if (-1 == p_env->gen.fd) {
    cdio_warn("Failed to open %s: %s", p_env->gen.source_name,
               strerror(errno));
    return false;
  }

  /* get the device name */
  psz_devname = strrchr( p_env->gen.source_name, '/');
  if( NULL != psz_devname )
    ++psz_devname;
  else
    psz_devname = p_env->gen.source_name;
  
  /* unraw the device name */
  if( *psz_devname == 'r' )
    ++psz_devname;
  
  ret = IOServiceGetMatchingServices( kIOMasterPortDefault, 
                                      IOBSDNameMatching(kIOMasterPortDefault, 0, psz_devname),
                                      &iterator );
  
  /* get service iterator for the device */
  if( ret != KERN_SUCCESS )
    {
        cdio_warn( "IOServiceGetMatchingServices: 0x%08x", ret );
        return false;
    }
  
  /* first service */
  p_env->MediaClass_service = IOIteratorNext( iterator );
  IOObjectRelease( iterator );

  /* search for kIOCDMediaClass or kIOCDVDMediaClass */ 
  while( p_env->MediaClass_service && 
         (!IOObjectConformsTo(p_env->MediaClass_service, kIOCDMediaClass)) &&
         (!IOObjectConformsTo(p_env->MediaClass_service, kIODVDMediaClass)) )
    {

      ret = IORegistryEntryGetParentIterator( p_env->MediaClass_service, 
                                              kIOServicePlane, 
                                              &iterator );
      if( ret != KERN_SUCCESS )
        {
          cdio_warn( "IORegistryEntryGetParentIterator: 0x%08x", ret );
          IOObjectRelease( p_env->MediaClass_service );
          return false;
        }
      
      IOObjectRelease( p_env->MediaClass_service );
      p_env->MediaClass_service = IOIteratorNext( iterator );
      IOObjectRelease( iterator );
    }
  
  if ( 0 == p_env->MediaClass_service )     {
    cdio_warn( "search for kIOCDMediaClass/kIODVDMediaClass came up empty" );
    return false;
  }

  /* Save the name so we can compare against this in case we have to do
     another scan. FIXME: this is hoaky and there's got to be a better
     variable to test or way to do.
   */
  IORegistryEntryGetPath(p_env->MediaClass_service, kIOServicePlane, 
                         p_env->psz_MediaClass_service);
#ifdef GET_SCSI_FIXED
  return get_scsi(p_env);
#else 
  return true;
#endif
}

/*!
  Run a SCSI MMC command. 
 
  cdio          CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
                time-out value.
  p_buf         Buffer for data, both sending and receiving
  i_buf         Size of buffer
  e_direction   direction the transfer is to go.
  cdb           CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return true if command completed successfully and false if not.
 */
#if 1

/* process a complete scsi command. */
// handle_scsi_cmd(cdrom_drive *d,
static int 
run_mmc_cmd_osx( void *p_user_data, 
                 unsigned int i_timeout_ms,
                 unsigned int i_cdb, const mmc_cdb_t *p_cdb, 
                 cdio_mmc_direction_t e_direction, 
                 unsigned int i_buf, /*in/out*/ void *p_buf )
{
  _img_private_t *p_env = p_user_data;
  uint8_t cmdbuf[16];
  UInt8 dir;
  IOVirtualRange buf;
  IOReturn ret;

  if (!p_env->scsi_task) return DRIVER_OP_UNSUPPORTED;

  memcpy(cmdbuf, p_cdb, i_cdb);

  dir = ( SCSI_MMC_DATA_READ == e_direction)
    ? kSCSIDataTransfer_FromTargetToInitiator
    : kSCSIDataTransfer_FromInitiatorToTarget;
  
  if (!i_buf)
    dir = kSCSIDataTransfer_NoDataTransfer;

  if (i_buf > MAX_BIG_BUFF_SIZE) {
    fprintf(stderr, "Excessive request size: %d bytes\n", i_buf);
    return TR_ILLEGAL;
  }

  buf.address = (IOVirtualAddress)p_buf;
  buf.length = i_buf;

  ret = (*p_env->scsi_task)->SetCommandDescriptorBlock(p_env->scsi_task, 
                                                       cmdbuf, i_cdb);
  if (ret != kIOReturnSuccess) {
    fprintf(stderr, "SetCommandDescriptorBlock: %x\n", ret);
    return TR_UNKNOWN;
  }
  
  ret = (*p_env->scsi_task)->SetScatterGatherEntries(p_env->scsi_task, &buf, 1,
                                                     i_buf, dir);
  if (ret != kIOReturnSuccess) {
    fprintf(stderr, "SetScatterGatherEntries: %x\n", ret);
    return TR_UNKNOWN;
  }
  
  ret = (*p_env->scsi_task)->ExecuteTaskSync(p_env->scsi_task, &p_env->sense, 
                                             &p_env->status, 
                                             &p_env->realized_len);
  if (ret != kIOReturnSuccess) {
    fprintf(stderr, "ExecuteTaskSync: %x\n", ret);
    return TR_UNKNOWN;
  }
  
  if (p_env->status != kSCSITaskStatus_GOOD) {
    int i;
    
    fprintf(stderr, "SCSI status: %x\n", p_env->status);
    fprintf(stderr, "Sense: %x %x %x\n",
            p_env->sense.SENSE_KEY,
            p_env->sense.ADDITIONAL_SENSE_CODE,
            p_env->sense.ADDITIONAL_SENSE_CODE_QUALIFIER);
    
    for (i = 0; i < i_cdb; i++)
      fprintf(stderr, "%02x ", cmdbuf[i]);
    
    fprintf(stderr, "\n");
    return TR_UNKNOWN;
  }
  
  if (p_env->sense.VALID_RESPONSE_CODE) {
    char key = p_env->sense.SENSE_KEY & 0xf;
    char ASC = p_env->sense.ADDITIONAL_SENSE_CODE;
    char ASCQ = p_env->sense.ADDITIONAL_SENSE_CODE_QUALIFIER;
    
    switch (key) {
    case 0:
      if (errno == 0)
        errno = EIO;
      return (TR_UNKNOWN);
    case 1:
      break;
    case 2:
      if (errno == 0)
        errno = EBUSY;
      return (TR_BUSY);
    case 3:
      if (ASC == 0x0C && ASCQ == 0x09) {
        /* loss of streaming */
        if (errno == 0)
          errno = EIO;
        return (TR_STREAMING);
      } else {
        if (errno == 0)
          errno = EIO;
        return (TR_MEDIUM);
      }
    case 4:
      if (errno == 0)
        errno = EIO;
      return (TR_FAULT);
    case 5:
      if (errno == 0)
        errno = EINVAL;
      return (TR_ILLEGAL);
    default:
      if (errno == 0)
        errno = EIO;
      return (TR_UNKNOWN);
    }
  }

  errno = 0;
  return (0);
}
#endif

#if 0
/*!
  Run a SCSI MMC command. 
 
  cdio          CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
                time-out value.
  p_buf         Buffer for data, both sending and receiving
  i_buf         Size of buffer
  e_direction   direction the transfer is to go.
  cdb           CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return true if command completed successfully and false if not.
 */
static int
run_mmc_cmd_osx( const void *p_user_data, 
                 unsigned int i_timeout_ms,
                 unsigned int i_cdb, const mmc_cdb_t *p_cdb, 
                 cdio_mmc_direction_t e_direction, 
                 unsigned int i_buf, /*in/out*/ void *p_buf )
{

#ifndef SCSI_MMC_FIXED
  return DRIVER_OP_UNSUPPORTED;
#else 
  const _img_private_t *p_env = p_user_data;
  SCSITaskDeviceInterface **sc;
  SCSITaskInterface **cmd = NULL;
  IOVirtualRange iov;
  SCSI_Sense_Data senseData;
  SCSITaskStatus status;
  UInt64 bytesTransferred;
  IOReturn ioReturnValue;
  int ret = 0;

  if (NULL == p_user_data) return 2;

  /* Make sure pp_scsiTaskDeviceInterface is initialized. FIXME: The code
     should probably be reorganized better for this. */
  if (!p_env->gen.toc_init) read_toc_osx (p_user_data) ;

  sc = p_env->pp_scsiTaskDeviceInterface;

  if (NULL == sc) return 3;

  cmd = (*sc)->CreateSCSITask(sc);
  if (cmd == NULL) {
    cdio_warn("Failed to create SCSI task");
    return -1;
  }

  iov.address = (IOVirtualAddress) p_buf;
  iov.length = i_buf;

  ioReturnValue = (*cmd)->SetCommandDescriptorBlock(cmd, (UInt8 *) p_cdb, 
                                                    i_cdb);
  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("SetCommandDescriptorBlock failed with status %x", 
              ioReturnValue);
    return -1;
  }

  ioReturnValue = (*cmd)->SetScatterGatherEntries(cmd, &iov, 1, i_buf,
                                                  (SCSI_MMC_DATA_READ == e_direction ) ? 
                                                  kSCSIDataTransfer_FromTargetToInitiator :
                                                  kSCSIDataTransfer_FromInitiatorToTarget);
  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("SetScatterGatherEntries failed with status %x", ioReturnValue);
    return -1;
  }

  ioReturnValue = (*cmd)->SetTimeoutDuration(cmd, i_timeout_ms );
  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("SetTimeoutDuration failed with status %x", ioReturnValue);
    return -1;
  }

  memset(&senseData, 0, sizeof(senseData));

  ioReturnValue = (*cmd)->ExecuteTaskSync(cmd,&senseData, &status, &
                                          bytesTransferred);

  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("Command execution failed with status %x", ioReturnValue);
    return -1;
  }

  if (cmd != NULL) {
    (*cmd)->Release(cmd);
  }

  return (ret);
#endif
}
#endif /* 0*/

/***************************************************************************
 * GetDeviceIterator - Gets an io_iterator_t for our class type
 ***************************************************************************/

static io_iterator_t
GetDeviceIterator ( const char * deviceClass )
{
  
  IOReturn      err      = kIOReturnSuccess;
  io_iterator_t iterator = MACH_PORT_NULL;
  
  err = IOServiceGetMatchingServices ( kIOMasterPortDefault,
                                       IOServiceMatching ( deviceClass ),
                                       &iterator );
  check ( err == kIOReturnSuccess );
  
  return iterator;
  
}

/***************************************************************************
 * GetFeaturesFlagsForDrive -Gets the bitfield which represents the
 * features flags.
 ***************************************************************************/

static bool
GetFeaturesFlagsForDrive ( CFDictionaryRef dict,
                           uint32_t *i_cdFlags,
                           uint32_t *i_dvdFlags )
{
  CFDictionaryRef propertiesDict = 0;
  CFNumberRef     flagsNumberRef = 0;
  
  *i_cdFlags = 0;
  *i_dvdFlags= 0;
  
  propertiesDict = ( CFDictionaryRef ) 
    CFDictionaryGetValue ( dict, 
                           CFSTR ( kIOPropertyDeviceCharacteristicsKey ) );

  if ( propertiesDict == 0 ) return false;
  
  /* Get the CD features */
  flagsNumberRef = ( CFNumberRef ) 
    CFDictionaryGetValue ( propertiesDict, 
                           CFSTR ( kIOPropertySupportedCDFeatures ) );
  if ( flagsNumberRef != 0 ) {
    CFNumberGetValue ( flagsNumberRef, kCFNumberLongType, i_cdFlags );
  }
  
  /* Get the DVD features */
  flagsNumberRef = ( CFNumberRef ) 
    CFDictionaryGetValue ( propertiesDict, 
                           CFSTR ( kIOPropertySupportedDVDFeatures ) );
  if ( flagsNumberRef != 0 ) {
    CFNumberGetValue ( flagsNumberRef, kCFNumberLongType, i_dvdFlags );
  }

  return true;
}

/*! 
  Get disc type associated with the cd object.
*/
static discmode_t
get_discmode_osx (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;
  char str[10];
  int32_t i_discmode = CDIO_DISC_MODE_ERROR;
  CFDictionaryRef propertiesDict = 0;
  CFStringRef data;

  propertiesDict  = GetRegistryEntryProperties ( p_env->MediaClass_service );

  if ( propertiesDict == 0 ) return i_discmode;

  data = ( CFStringRef ) 
    CFDictionaryGetValue ( propertiesDict, CFSTR ( kIODVDMediaTypeKey ) );

  if( CFStringGetCString( data, str, sizeof(str),
                          kCFStringEncodingASCII ) ) {
    if (0 == strncmp(str, "DVD+R", strlen(str)) )
      i_discmode = CDIO_DISC_MODE_DVD_PR;
    else if (0 == strncmp(str, "DVD+RW", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_DVD_PRW;
    else if (0 == strncmp(str, "DVD-R", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_DVD_R;
    else if (0 == strncmp(str, "DVD-RW", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_DVD_RW;
    else if (0 == strncmp(str, "DVD-ROM", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_DVD_ROM;
    else if (0 == strncmp(str, "DVD-RAM", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_DVD_RAM;
    else if (0 == strncmp(str, "CD-ROM", strlen(str)) )
      i_discmode = CDIO_DISC_MODE_CD_DATA;
    else if (0 == strncmp(str, "CDR", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_CD_DATA;
    else if (0 == strncmp(str, "CDRW", strlen(str)) ) 
      i_discmode = CDIO_DISC_MODE_CD_DATA;
    //??  Handled by below? CFRelease( data );
  }
  CFRelease( propertiesDict );    
  if (CDIO_DISC_MODE_CD_DATA == i_discmode) {
    /* Need to do more classification */
    return get_discmode_cd_generic(p_user_data);
  }
  return i_discmode;

}

static io_service_t
get_drive_service_osx(const _img_private_t *p_env)
{
  io_service_t  service;
  io_iterator_t service_iterator;
  
  service_iterator = GetDeviceIterator ( kIOCDBlockStorageDeviceClassString );

  if( service_iterator == MACH_PORT_NULL ) return 0;
  
  service = IOIteratorNext( service_iterator );
  if( service == 0 ) return 0;

  do
    {
      char psz_service[MAX_SERVICE_NAME];
      IORegistryEntryGetPath(service, kIOServicePlane, psz_service);
      psz_service[MAX_SERVICE_NAME-1] = '\0';
      
      /* FIXME: This is all hoaky. Here we need info from a parent class,
         psz_service of what we opened above. We are relying on the
         fact that the name  will be a substring of the name we
         openned with.
      */
      if (0 == strncmp(psz_service, p_env->psz_MediaClass_service, 
                       strlen(psz_service))) {
        /* Found our device */
        IOObjectRelease( service_iterator );
        return service;
      }
      
      IOObjectRelease( service );
      
    } while( ( service = IOIteratorNext( service_iterator ) ) != 0 );

  IOObjectRelease( service_iterator );
  return service;
}

static void
get_drive_cap_osx(const void *p_user_data,
                  /*out*/ cdio_drive_read_cap_t  *p_read_cap,
                  /*out*/ cdio_drive_write_cap_t *p_write_cap,
                  /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  const _img_private_t *p_env = p_user_data;
  uint32_t i_cdFlags;
  uint32_t i_dvdFlags;

  io_service_t  service = get_drive_service_osx(p_env);
  
  if( service == 0 ) goto err_exit;

  /* Found our device */
  {
    CFDictionaryRef  properties = GetRegistryEntryProperties ( service );
    
    if (! GetFeaturesFlagsForDrive ( properties, &i_cdFlags, 
                                     &i_dvdFlags ) ) {
      IOObjectRelease( service );
      goto err_exit;
    }
    
    /* Reader */
    
    if ( 0 != (i_cdFlags & kCDFeaturesAnalogAudioMask) )
      *p_read_cap  |= CDIO_DRIVE_CAP_READ_AUDIO;      
    
    if ( 0 != (i_cdFlags & kCDFeaturesWriteOnceMask) ) 
      *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_R;
    
    if ( 0 != (i_cdFlags & kCDFeaturesCDDAStreamAccurateMask) )
      *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_DA;
    
    if ( 0 != (i_dvdFlags & kDVDFeaturesReadStructuresMask) )
      *p_read_cap  |= CDIO_DRIVE_CAP_READ_DVD_ROM;
    
    if ( 0 != (i_cdFlags & kCDFeaturesReWriteableMask) )
      *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_RW;
    
    if ( 0 != (i_dvdFlags & kDVDFeaturesWriteOnceMask) ) 
      *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_R;
    
    if ( 0 != (i_dvdFlags & kDVDFeaturesRandomWriteableMask) )
      *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_RAM;
    
    if ( 0 != (i_dvdFlags & kDVDFeaturesReWriteableMask) )
      *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_RW;
    
    /***
        if ( 0 != (i_dvdFlags & kDVDFeaturesPlusRMask) )
        *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_PR;
        
        if ( 0 != (i_dvdFlags & kDVDFeaturesPlusRWMask )
        *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_PRW;
        ***/

    /* FIXME: fill out. For now assume CD-ROM is relatively modern. */
      *p_misc_cap = (
                     CDIO_DRIVE_CAP_MISC_CLOSE_TRAY 
                     | CDIO_DRIVE_CAP_MISC_EJECT
                     | CDIO_DRIVE_CAP_MISC_LOCK
                     | CDIO_DRIVE_CAP_MISC_SELECT_SPEED
                     | CDIO_DRIVE_CAP_MISC_MULTI_SESSION
                     | CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED
                     | CDIO_DRIVE_CAP_MISC_RESET
                     | CDIO_DRIVE_CAP_READ_MCN
                     | CDIO_DRIVE_CAP_READ_ISRC
                     );

    IOObjectRelease( service );
  }
  
  return;

 err_exit:
  *p_misc_cap = *p_write_cap = *p_read_cap = CDIO_DRIVE_CAP_UNKNOWN;
  return;
}

#if 1
/****************************************************************************
 * GetDriveDescription - Gets drive description. 
 ****************************************************************************/

static bool
get_hwinfo_osx ( const CdIo_t *p_cdio, /*out*/ cdio_hwinfo_t *hw_info)
{
  _img_private_t *p_env = (_img_private_t *) p_cdio->env;
  io_service_t  service = get_drive_service_osx(p_env);

  if ( service == 0 ) return false;
  
  /* Found our device */
  {
    CFStringRef      vendor      = NULL;
    CFStringRef      product     = NULL;
    CFStringRef      revision    = NULL;
  
    CFDictionaryRef  properties  = GetRegistryEntryProperties ( service );
    CFDictionaryRef  deviceDict  = ( CFDictionaryRef ) 
      CFDictionaryGetValue ( properties, 
                             CFSTR ( kIOPropertyDeviceCharacteristicsKey ) );
    
    if ( deviceDict == 0 ) return false;
    
    vendor = ( CFStringRef ) 
      CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyVendorNameKey ) );
    
    if ( CFStringGetCString( vendor,
                             (char *) &(hw_info->psz_vendor),
                             sizeof(hw_info->psz_vendor),
                             kCFStringEncodingASCII ) )
      CFRelease( vendor );
    
    product = ( CFStringRef ) 
      CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyProductNameKey ) );
    
    if ( CFStringGetCString( product,
                             (char *) &(hw_info->psz_model),
                             sizeof(hw_info->psz_model),
                             kCFStringEncodingASCII ) )
      CFRelease( product );
    
    revision = ( CFStringRef ) 
      CFDictionaryGetValue ( deviceDict, 
                             CFSTR ( kIOPropertyProductRevisionLevelKey ) );
    
    if ( CFStringGetCString( revision,
                             (char *) &(hw_info->psz_revision),
                             sizeof(hw_info->psz_revision),
                             kCFStringEncodingASCII ) )
      CFRelease( revision );
  }
  return true;
  
}
#endif

/*
  Get cdtext information in p_user_data for track i_track. 
  For disc information i_track is 0.
  
  Return the CD-TEXT or NULL if obj is NULL,
  CD-TEXT information does not exist, or (as is the case here)
  we don't know how to get this implemented.
*/
static cdtext_t *
get_cdtext_osx (void *p_user_data, track_t i_track) 
{
  return NULL;
}

static void 
_free_osx (void *p_user_data) {
  _img_private_t *p_env = p_user_data;
  if (NULL == p_env) return;
  if (p_env->gen.fd != -1)
    close(p_env->gen.fd);
  if (p_env->MediaClass_service)
    IOObjectRelease( p_env->MediaClass_service );
  cdio_generic_free(p_env);
  if (NULL != p_env->pp_lba)  free((void *) p_env->pp_lba);
  if (NULL != p_env->pTOC)    free((void *) p_env->pTOC);

  if (p_env->scsi_task)
    (*p_env->scsi_task)->Release(p_env->scsi_task);

  if (p_env->pp_scsiTaskDeviceInterface) 
    (*p_env->pp_scsiTaskDeviceInterface) -> 
      ReleaseExclusiveAccess(p_env->pp_scsiTaskDeviceInterface);
  if (p_env->pp_scsiTaskDeviceInterface) 
    (*p_env->pp_scsiTaskDeviceInterface) ->
      Release ( p_env->pp_scsiTaskDeviceInterface );

  if (p_env->mmc) 
    (*p_env->mmc)->Release(p_env->mmc);

  if (p_env->plugin) 
    IODestroyPlugInInterface(p_env->plugin);

}

/*!
   Reads i_blocks of data sectors from cd device into p_data starting
   from i_lsn.
   Returns DRIVER_OP_SUCCESS if no error. 
 */
static driver_return_code_t
read_data_sectors_osx (void *p_user_data, void *p_data, lsn_t i_lsn, 
                       uint16_t i_blocksize, uint32_t i_blocks)
{
  _img_private_t *p_env = p_user_data;

  if (!p_user_data) return DRIVER_OP_UNINIT;

  {
    dk_cd_read_t cd_read;
    track_t i_track = cdio_get_track(p_env->gen.cdio, i_lsn);
    
    memset( &cd_read, 0, sizeof(cd_read) );
    
    cd_read.sectorArea  = kCDSectorAreaUser;
    cd_read.buffer      = p_data;

    /* FIXME: Do I have to put use get_track_green_osx? */
    switch(get_track_format_osx(p_user_data, i_track)) {
    case TRACK_FORMAT_CDI:
    case TRACK_FORMAT_DATA:
      cd_read.sectorType  = kCDSectorTypeMode1;
      cd_read.offset      = i_lsn * kCDSectorSizeMode1;
      break;
    case TRACK_FORMAT_XA:
      cd_read.sectorType  = kCDSectorTypeMode2;
      cd_read.offset      = i_lsn * kCDSectorSizeMode2;
      break;
    default:
      return DRIVER_OP_ERROR;
    }
    
    cd_read.bufferLength = i_blocksize * i_blocks;
    
    if( ioctl( p_env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
      {
        cdio_info( "could not read block %d, %s", i_lsn, strerror(errno) );
        return DRIVER_OP_ERROR;
      }
    return DRIVER_OP_SUCCESS;
  }
}

  
/*!
   Reads i_blocks of mode2 form2 sectors from cd device into data starting
   from i_lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
read_mode1_sectors_osx (void *p_user_data, void *p_data, lsn_t i_lsn, 
                        bool b_form2, uint32_t i_blocks)
{
  _img_private_t *p_env = p_user_data;
  dk_cd_read_t cd_read;
  
  memset( &cd_read, 0, sizeof(cd_read) );
  
  cd_read.sectorArea  = kCDSectorAreaUser;
  cd_read.buffer      = p_data;
  cd_read.sectorType  = kCDSectorTypeMode1;
  
  if (b_form2) {
    cd_read.offset       = i_lsn * kCDSectorSizeMode2;
    cd_read.bufferLength = kCDSectorSizeMode2 * i_blocks;
  } else {
    cd_read.offset       = i_lsn * kCDSectorSizeMode1;
    cd_read.bufferLength = kCDSectorSizeMode1 * i_blocks;
  }
  
   if( ioctl( p_env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_info( "could not read block %d, %s", i_lsn, strerror(errno) );
    return DRIVER_OP_ERROR;
  }
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads i_blocks of mode2 form2 sectors from cd device into data starting
   from lsn.
   Returns DRIVER_OP_SUCCESS if no error. 
 */
static driver_return_code_t
read_mode2_sectors_osx (void *p_user_data, void *p_data, lsn_t i_lsn,
                        bool b_form2, uint32_t i_blocks)
{
  _img_private_t *p_env = p_user_data;
  dk_cd_read_t cd_read;
  
  memset( &cd_read, 0, sizeof(cd_read) );
  
  cd_read.sectorArea = kCDSectorAreaUser;
  cd_read.buffer = p_data;
  
  if (b_form2) {
    cd_read.offset       = i_lsn * kCDSectorSizeMode2Form2;
    cd_read.sectorType   = kCDSectorTypeMode2Form2;
    cd_read.bufferLength = kCDSectorSizeMode2Form2 * i_blocks;
  } else {
    cd_read.offset       = i_lsn * kCDSectorSizeMode2Form1;
    cd_read.sectorType   = kCDSectorTypeMode2Form1;
    cd_read.bufferLength = kCDSectorSizeMode2Form1 * i_blocks;
  }
  
  if( ioctl( p_env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_info( "could not read block %d, %s", i_lsn, strerror(errno) );
    return DRIVER_OP_ERROR;
  }
  return DRIVER_OP_SUCCESS;
}

  
/*!
   Reads a single audio sector from CD device into p_data starting from lsn.
   Returns 0 if no error. 
 */
static int
read_audio_sectors_osx (void *user_data, void *p_data, lsn_t lsn, 
                             unsigned int i_blocks)
{
  _img_private_t *env = user_data;
  dk_cd_read_t cd_read;
  
  memset( &cd_read, 0, sizeof(cd_read) );
  
  cd_read.offset       = lsn * kCDSectorSizeCDDA;
  cd_read.sectorArea   = kCDSectorAreaUser;
  cd_read.sectorType   = kCDSectorTypeCDDA;
  
  cd_read.buffer       = p_data;
  cd_read.bufferLength = kCDSectorSizeCDDA * i_blocks;
  
  if( ioctl( env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_info( "could not read block %d\n%s", lsn,
               strerror(errno));
    return DRIVER_OP_ERROR;
  }
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads a single mode2 sector from cd device into p_data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
read_mode1_sector_osx (void *p_user_data, void *p_data, lsn_t i_lsn, 
                       bool b_form2)
{
  return read_mode1_sectors_osx(p_user_data, p_data, i_lsn, b_form2, 1);
}

/*!
   Reads a single mode2 sector from cd device into p_data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
read_mode2_sector_osx (void *p_user_data, void *p_data, lsn_t i_lsn,
                       bool b_form2)
{
  return read_mode2_sectors_osx(p_user_data, p_data, i_lsn, b_form2, 1);
}

/*!
  Set the key "arg" to "value" in source device.
*/
static driver_return_code_t
_set_arg_osx (void *p_user_data, const char key[], const char value[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source"))
    {
      if (!value) return DRIVER_OP_ERROR;
      free (p_env->gen.source_name);
      p_env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      if (!strcmp(value, "OSX"))
        p_env->access_mode = _AM_OSX;
      else
        cdio_warn ("unknown access type: %s. ignored.", value);
    }
  else return DRIVER_OP_ERROR;

  return DRIVER_OP_SUCCESS;
}

#if 0
static void 
TestDevice(_img_private_t *p_env, io_service_t service)
{
  SInt32                          score;
  HRESULT                         herr;
  kern_return_t                   err;
  IOCFPlugInInterface             **plugInInterface = NULL;
  MMCDeviceInterface              **mmcInterface = NULL;

  /* Create the IOCFPlugIn interface so we can query it. */

  err = IOCreatePlugInInterfaceForService ( service,
                                            kIOMMCDeviceUserClientTypeID,
                                            kIOCFPlugInInterfaceID,
                                            &plugInInterface,
                                            &score );
  if ( err != noErr ) {
    printf("IOCreatePlugInInterfaceForService returned %d\n", err);
    return;
  }
  
  /* Query the interface for the MMCDeviceInterface. */
  
  herr = ( *plugInInterface )->QueryInterface ( plugInInterface,
                                                CFUUIDGetUUIDBytes ( kIOMMCDeviceInterfaceID ),
                                                ( LPVOID ) &mmcInterface );
  
  if ( herr != S_OK )     {
    printf("QueryInterface returned %ld\n", herr);
    return;
  }
  
  p_env->pp_scsiTaskDeviceInterface = 
    ( *mmcInterface )->GetSCSITaskDeviceInterface ( mmcInterface );
  
  if ( NULL == p_env->pp_scsiTaskDeviceInterface )  {
    printf("GetSCSITaskDeviceInterface returned NULL\n");
    return;
  }
  
  ( *mmcInterface )->Release ( mmcInterface );
  IODestroyPlugInInterface ( plugInInterface );
}
#endif

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return false if successful or true if an error.
*/
static bool
read_toc_osx (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  CFDictionaryRef propertiesDict = 0;
  CFDataRef data;

  /* create a CF dictionary containing the TOC */
  propertiesDict = GetRegistryEntryProperties( p_env->MediaClass_service );

  if ( 0 == propertiesDict )     {
    return false;
  }

  /* get the TOC from the dictionary */
  data = (CFDataRef) CFDictionaryGetValue( propertiesDict,
                                           CFSTR(kIOCDMediaTOCKey) );
  if ( data  != NULL ) {
    CFRange range;
    CFIndex buf_len;
    
    buf_len = CFDataGetLength( data ) + 1;
    range = CFRangeMake( 0, buf_len );
    
    if( ( p_env->pTOC = (CDTOC *)malloc( buf_len ) ) != NULL ) {
      CFDataGetBytes( data, range, (u_char *) p_env->pTOC );
    } else {
      cdio_warn( "Trouble allocating CDROM TOC" );
      CFRelease( propertiesDict );    
      return false;
    }
  } else     {
    cdio_warn( "Trouble reading TOC" );
    CFRelease( propertiesDict );    
    return false;
  }

  /* TestDevice(p_env, service); */
  CFRelease( propertiesDict );    

  p_env->i_descriptors = CDTOCGetDescriptorCount ( p_env->pTOC );

  /* Read in starting sectors. There may be non-tracks mixed in with
     the real tracks.  So find the first and last track number by
     scanning. Also find the lead-out track position.
   */
  {
    int i, i_leadout = -1;
    
    CDTOCDescriptor *pTrackDescriptors;
    
    p_env->pp_lba = malloc( p_env->i_descriptors * sizeof(int) );
    if( p_env->pp_lba == NULL )
      {
        cdio_warn("Out of memory in allocating track starting LSNs" );
        free( p_env->pTOC );
        return false;
      }
    
    pTrackDescriptors = p_env->pTOC->descriptors;

    p_env->gen.i_first_track = CDIO_CD_MAX_TRACKS+1;
    p_env->i_last_track      = CDIO_CD_MIN_TRACK_NO;
    p_env->i_first_session   = CDIO_CD_MAX_TRACKS+1;
    p_env->i_last_session    = CDIO_CD_MIN_TRACK_NO;
    
    for( i = 0; i < p_env->i_descriptors; i++ )
      {
        track_t i_track     = pTrackDescriptors[i].point;
        session_t i_session = pTrackDescriptors[i].session;

        cdio_debug( "point: %d, tno: %d, session: %d, adr: %d, control:%d, "
                    "address: %d:%d:%d, p: %d:%d:%d", 
                    i_track,
                    pTrackDescriptors[i].tno, i_session,
                    pTrackDescriptors[i].adr, pTrackDescriptors[i].control,
                    pTrackDescriptors[i].address.minute,
                    pTrackDescriptors[i].address.second,
                    pTrackDescriptors[i].address.frame, 
                    pTrackDescriptors[i].p.minute,
                    pTrackDescriptors[i].p.second, 
                    pTrackDescriptors[i].p.frame );

        /* track information has adr = 1 */
        if ( 0x01 != pTrackDescriptors[i].adr ) 
          continue;

        if( i_track == OSX_CDROM_LEADOUT_TRACK )
          i_leadout = i;

        if( i_track > CDIO_CD_MAX_TRACKS || i_track < CDIO_CD_MIN_TRACK_NO )
          continue;

        if (p_env->gen.i_first_track > i_track) 
          p_env->gen.i_first_track = i_track;
        
        if (p_env->i_last_track < i_track) 
          p_env->i_last_track = i_track;
        
        if (p_env->i_first_session > i_session) 
          p_env->i_first_session = i_session;
        
        if (p_env->i_last_session < i_session) 
          p_env->i_last_session = i_session;
      }

    /* Now that we know what the first track number is, we can make sure
       index positions are ordered starting at 0.
     */
    for( i = 0; i < p_env->i_descriptors; i++ )
      {
        track_t i_track = pTrackDescriptors[i].point;

        if( i_track > CDIO_CD_MAX_TRACKS || i_track < CDIO_CD_MIN_TRACK_NO )
          continue;

        /* Note what OSX calls a LBA we call an LSN. So below re we 
           really have have MSF -> LSN -> LBA.
         */
        p_env->pp_lba[i_track - p_env->gen.i_first_track] =
          cdio_lsn_to_lba(CDConvertMSFToLBA( pTrackDescriptors[i].p ));
        set_track_flags(&(p_env->gen.track_flags[i_track]), 
                        pTrackDescriptors[i].control);
      }
    
    if( i_leadout == -1 )
      {
        cdio_warn( "CD leadout not found" );
        free( p_env->pp_lba );
        free( (void *) p_env->pTOC );
        return false;
      }
    
    /* Set leadout sector. 
       Note what OSX calls a LBA we call an LSN. So below re we 
       really have have MSF -> LSN -> LBA.
    */
    p_env->pp_lba[TOTAL_TRACKS] =
      cdio_lsn_to_lba(CDConvertMSFToLBA( pTrackDescriptors[i_leadout].p ));
    p_env->gen.i_tracks = TOTAL_TRACKS;
  }

  p_env->gen.toc_init   = true;

  return( true ); 

}

/*!  
  Return the starting LSN track number
  i_track in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lsn_t
get_track_lba_osx(void *p_user_data, track_t i_track)
{
  _img_private_t *p_env = p_user_data;

  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;
  if (!p_env->gen.toc_init) return CDIO_INVALID_LSN;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = p_env->i_last_track+1;

  if (i_track > p_env->i_last_track + 1 || i_track < p_env->gen.i_first_track) {
    return CDIO_INVALID_LSN;
  } else {
    return p_env->pp_lba[i_track - p_env->gen.i_first_track];
  }
}

/*!
  Eject media . Return DRIVER_OP_SUCCESS if successful.

  The only way to cleanly unmount the disc under MacOS X (before
  Tiger) is to use the 'disktool' command line utility. It uses the
  non-public DiskArbitration API, which can not be used by Cocoa or
  Carbon applications.

  Since Tiger (MacOS X 10.4), DiskArbitration is a public framework
  and we can use it as needed.

 */

#ifndef HAVE_DISKARBITRATION
static driver_return_code_t
_eject_media_osx (void *user_data) {

  _img_private_t *p_env = user_data;

  FILE *p_file;
  char *psz_drive;
  char sz_cmd[32];

  if( ( psz_drive = (char *)strstr( p_env->gen.source_name, "disk" ) ) != NULL &&
      strlen( psz_drive ) > 4 )
    {
#define EJECT_CMD "/usr/sbin/hdiutil eject %s"
      snprintf( sz_cmd, sizeof(sz_cmd), EJECT_CMD, psz_drive );
#undef EJECT_CMD
      
      if( ( p_file = popen( sz_cmd, "r" ) ) != NULL )
        {
          char psz_result[0x200];
          int i_ret = fread( psz_result, 1, sizeof(psz_result) - 1, p_file );
          
          if( i_ret == 0 && ferror( p_file ) != 0 )
            {
              pclose( p_file );
              return DRIVER_OP_ERROR;
            }
          
          pclose( p_file );
          
          psz_result[ i_ret ] = 0;
          
          if( strstr( psz_result, "Disk Ejected" ) != NULL )
            {
              return DRIVER_OP_SUCCESS;
            }
        }
    }
  
  return DRIVER_OP_ERROR;
}
#else /* HAVE_DISKARBITRATION */
typedef struct dacontext_s {
    int                 result;
    Boolean             completed;
    DASessionRef        session;
    CFRunLoopRef        runloop;
    CFRunLoopSourceRef  cancel;
} dacontext_t;

static void cancel_runloop(void *info) { /* do nothing */ }

static CFRunLoopSourceContext cancelRunLoopSourceContext = {
    .perform = cancel_runloop
};

static void media_eject_callback(DADiskRef disk, DADissenterRef dissenter, void *context)
{
    dacontext_t *dacontext = (dacontext_t *)context;

    if ( dissenter )
      {
	CFStringRef status = DADissenterGetStatusString(dissenter);
	if (status)
	{ 
		size_t cstr_size = CFStringGetLength(status);
		char *cstr = malloc(cstr_size);
		if ( CFStringGetCString( status,
				 cstr, cstr_size,
				 kCFStringEncodingASCII ) )
	  	CFRelease( status );

		cdio_warn("%s", cstr);

		free(cstr);
	}
      }

    dacontext->result    = (dissenter ? DRIVER_OP_ERROR : DRIVER_OP_SUCCESS);
    dacontext->completed = TRUE;
    CFRunLoopSourceSignal(dacontext->cancel);
    CFRunLoopWakeUp(dacontext->runloop);
}

static void media_unmount_callback(DADiskRef disk, DADissenterRef dissenter, void *context)
{
    dacontext_t *dacontext = (dacontext_t *)context;

    if (!dissenter) {
        DADiskEject(disk, kDADiskEjectOptionDefault, media_eject_callback, context);
        dacontext->result = dacontext->result == DRIVER_OP_UNINIT ? DRIVER_OP_SUCCESS : dacontext->result;
    }
    else {
        dacontext->result    = DRIVER_OP_ERROR;
        dacontext->completed = TRUE;
        CFRunLoopSourceSignal(dacontext->cancel);
        CFRunLoopWakeUp(dacontext->runloop);
    }
}

static driver_return_code_t
_eject_media_osx (void *user_data) {

  _img_private_t *p_env = user_data;
  char *psz_drive;

  DADiskRef       disk;
  dacontext_t     dacontext;
  CFDictionaryRef description;

  if( ( psz_drive = (char *)strstr( p_env->gen.source_name, "disk" ) ) == NULL ||
      strlen( psz_drive ) <= 4 )
    {
      return DRIVER_OP_ERROR;
    }

  if (p_env->gen.fd != -1)
    close(p_env->gen.fd);
  p_env->gen.fd = -1;

  dacontext.result    = DRIVER_OP_UNINIT;
  dacontext.completed = FALSE;
  dacontext.runloop   = CFRunLoopGetCurrent();
  dacontext.cancel    = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &cancelRunLoopSourceContext);
  
  if (!dacontext.cancel)
    {
      return DRIVER_OP_ERROR;
    }
  
  if (!(dacontext.session = DASessionCreate(kCFAllocatorDefault)))
    {
      CFRelease(dacontext.cancel);
      return DRIVER_OP_ERROR;
    }
  
  if ((disk = DADiskCreateFromBSDName(kCFAllocatorDefault, dacontext.session, psz_drive)) != NULL)
    {
      if ((description = DADiskCopyDescription(disk)) != NULL)
	{
	  /* Does the device need to be unmounted first? */
	  DASessionScheduleWithRunLoop(dacontext.session, dacontext.runloop, kCFRunLoopDefaultMode);
	  CFRunLoopAddSource(dacontext.runloop, dacontext.cancel, kCFRunLoopDefaultMode);

	  if (CFDictionaryGetValueIfPresent(description, kDADiskDescriptionVolumePathKey, NULL))
	    {
	      DADiskUnmount(disk, kDADiskUnmountOptionDefault, media_unmount_callback, &dacontext);
            }
	  else
	    {
	      DADiskEject(disk, kDADiskEjectOptionDefault, media_eject_callback, &dacontext);
	      dacontext.result = dacontext.result == DRIVER_OP_UNINIT ? DRIVER_OP_SUCCESS : dacontext.result;
            }
	  if (!dacontext.completed)
	    {
	      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 30.0, TRUE);  /* timeout after 30 seconds */
            }
	  CFRunLoopRemoveSource(dacontext.runloop, dacontext.cancel, kCFRunLoopDefaultMode);
	  DASessionUnscheduleFromRunLoop(dacontext.session, dacontext.runloop, kCFRunLoopDefaultMode);
	  CFRelease(description);
        }
      CFRelease(disk);
    }
  
  CFRunLoopSourceInvalidate(dacontext.cancel);
  CFRelease(dacontext.cancel);
  CFRelease(dacontext.session);
  return dacontext.result;
}
#endif

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static lsn_t
get_disc_last_lsn_osx (void *user_data)
{
  return get_track_lba_osx(user_data, CDIO_CDROM_LEADOUT_TRACK);
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_osx (void *user_data, const char key[])
{
  _img_private_t *p_env = user_data;

  if (!strcmp (key, "source")) {
    return p_env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (p_env->access_mode) {
    case _AM_OSX:
      return "OS X";
    case _AM_NONE:
      return "no access method";
    }
  } 
  return NULL;
}

/*!
  Return the media catalog number MCN.
 */
static char *
get_mcn_osx (const void *user_data) {
  const _img_private_t *p_env = user_data;
  dk_cd_read_mcn_t cd_read;

  memset( &cd_read, 0, sizeof(cd_read) );

  if( ioctl( p_env->gen.fd, DKIOCCDREADMCN, &cd_read ) < 0 )
  {
    cdio_debug( "could not read MCN, %s", strerror(errno) );
    return NULL;
  }
  return strdup((char*)cd_read.mcn);
}


/*!  
  Get format of track. 
*/
static track_format_t
get_track_format_osx(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  dk_cd_read_track_info_t cd_read;
  CDTrackInfo a_track;

  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;

  if (i_track > p_env->i_last_track || i_track < p_env->gen.i_first_track)
    return TRACK_FORMAT_ERROR;
    
  memset( &cd_read, 0, sizeof(cd_read) );

  cd_read.address = i_track;
  cd_read.addressType = kCDTrackInfoAddressTypeTrackNumber;
  
  cd_read.buffer = &a_track;
  cd_read.bufferLength = sizeof(CDTrackInfo);
  
  if( ioctl( p_env->gen.fd, DKIOCCDREADTRACKINFO, &cd_read ) == -1 )
  {
    cdio_warn( "could not read trackinfo for track %d:\n%s", i_track,
               strerror(errno));
    return TRACK_FORMAT_ERROR;
  }

  cdio_debug( "%d: trackinfo trackMode: %x dataMode: %x", i_track, 
              a_track.trackMode, a_track.dataMode );

  if (a_track.trackMode == CDIO_CDROM_DATA_TRACK) {
    if (a_track.dataMode == CDROM_CDI_TRACK) {
      return TRACK_FORMAT_CDI;
    } else if (a_track.dataMode == CDROM_XA_TRACK) {
      return TRACK_FORMAT_XA;
    } else {
      return TRACK_FORMAT_DATA;
    }
  } else {
    return TRACK_FORMAT_AUDIO;
  }

}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8

  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
get_track_green_osx(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  CDTrackInfo a_track;

  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;

  if ( i_track > p_env->i_last_track || i_track < p_env->gen.i_first_track )
    return false;

  else {

    dk_cd_read_track_info_t cd_read;
    
    memset( &cd_read, 0, sizeof(cd_read) );
    
    cd_read.address      = i_track;
    cd_read.addressType  = kCDTrackInfoAddressTypeTrackNumber;
    
    cd_read.buffer       = &a_track;
    cd_read.bufferLength = sizeof(CDTrackInfo);
    
    if( ioctl( p_env->gen.fd, DKIOCCDREADTRACKINFO, &cd_read ) == -1 ) {
      cdio_warn( "could not read trackinfo for track %d:\n%s", i_track,
                 strerror(errno));
      return false;
    }
    return ((a_track.trackMode & CDIO_CDROM_DATA_TRACK) != 0);
  }
}

/* Set CD-ROM drive speed */
static int 
set_speed_osx (void *p_user_data, int i_speed)
{
  const _img_private_t *p_env = p_user_data;

  if (!p_env) return -1;
  return ioctl(p_env->gen.fd, DKIOCCDSETSPEED, i_speed);
}

#endif /* HAVE_DARWIN_CDROM */

/*!
  Close tray on CD-ROM.
  
  @param psz_drive the CD-ROM drive to be closed.
  
*/

/* FIXME: We don't use the device name because we don't how 
   to.
 */
#define CLOSE_TRAY_CMD "/usr/sbin/drutil tray close"
driver_return_code_t 
close_tray_osx (const char *psz_drive)
{
#ifdef HAVE_DARWIN_CDROM
  FILE *p_file;
  char sz_cmd[80];

  if ( !psz_drive) return DRIVER_OP_UNINIT;

  /* Right now we really aren't making use of snprintf, but 
     possibly someday we will.
   */
  snprintf( sz_cmd, sizeof(sz_cmd), CLOSE_TRAY_CMD );

  if( ( p_file = popen( sz_cmd, "r" ) ) != NULL )
    {
      char psz_result[0x200];
      int i_ret = fread( psz_result, 1, sizeof(psz_result) - 1, p_file );
      
      if( i_ret == 0 && ferror( p_file ) != 0 )
        {
          pclose( p_file );
          return DRIVER_OP_ERROR;
        }
      
      pclose( p_file );
      
      psz_result[ i_ret ] = 0;
      
      if( 0 == i_ret )
        {
          return DRIVER_OP_SUCCESS;
        }
    }
  
  return DRIVER_OP_ERROR;
#else 
  return DRIVER_OP_NO_DRIVER;
#endif /*HAVE_DARWIN_CDROM*/
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char **
cdio_get_devices_osx(void)
{
#ifndef HAVE_DARWIN_CDROM
  return NULL;
#else
  io_object_t   next_media;
  mach_port_t   master_port;
  kern_return_t kern_result;
  io_iterator_t media_iterator;
  CFMutableDictionaryRef classes_to_match;
  char        **drives = NULL;
  unsigned int  num_drives=0;
  
  kern_result = IOMasterPort( MACH_PORT_NULL, &master_port );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  classes_to_match = IOServiceMatching( kIOMediaClass );
  if( classes_to_match == NULL )
    {
      return( NULL );
    }
  
  CFDictionarySetValue( classes_to_match, CFSTR(kIOMediaEjectableKey),
                        kCFBooleanTrue );
  
  kern_result = IOServiceGetMatchingServices( master_port, 
                                              classes_to_match,
                                              &media_iterator );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  next_media = IOIteratorNext( media_iterator );
  if( next_media != 0 )
    {
      char psz_buf[0x32];
      size_t dev_path_length;
      CFTypeRef str_bsd_path;
      
      do
        {
          str_bsd_path = 
            IORegistryEntryCreateCFProperty( next_media,
                                             CFSTR( kIOBSDNameKey ),
                                             kCFAllocatorDefault,
                                             0 );
          if( str_bsd_path == NULL )
            {
              IOObjectRelease( next_media );
              continue;
            }
          
          /* Below, by appending 'r' to the BSD node name, we indicate
             a raw disk. Raw disks receive I/O requests directly and
             don't go through a buffer cache. */        
          snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
          dev_path_length = strlen( psz_buf );
          
          if( CFStringGetCString( str_bsd_path,
                                  (char*)&psz_buf + dev_path_length,
                                  sizeof(psz_buf) - dev_path_length,
                                  kCFStringEncodingASCII ) )
            {
              cdio_add_device_list(&drives, strdup(psz_buf), &num_drives);
            }
          CFRelease( str_bsd_path );
          IOObjectRelease( next_media );
          
        } while( ( next_media = IOIteratorNext( media_iterator ) ) != 0 );
    }
  IOObjectRelease( media_iterator );
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /* HAVE_DARWIN_CDROM */
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char *
cdio_get_default_device_osx(void)
{
#ifndef HAVE_DARWIN_CDROM
  return NULL;
#else
  io_object_t   next_media;
  kern_return_t kern_result;
  io_iterator_t media_iterator;
  CFMutableDictionaryRef classes_to_match;
  
  classes_to_match = IOServiceMatching( kIOMediaClass );
  if( classes_to_match == NULL )
    {
      return( NULL );
    }
  
  CFDictionarySetValue( classes_to_match, CFSTR(kIOMediaEjectableKey),
                        kCFBooleanTrue );
  
  kern_result = IOServiceGetMatchingServices( kIOMasterPortDefault, 
                                              classes_to_match,
                                              &media_iterator );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  next_media = IOIteratorNext( media_iterator );
  if( next_media != 0 )
    {
      char psz_buf[0x32];
      size_t dev_path_length;
      CFTypeRef str_bsd_path;
      
      do
        {
          str_bsd_path = IORegistryEntryCreateCFProperty( next_media,
                                                          CFSTR( kIOBSDNameKey ),
                                                          kCFAllocatorDefault,
                                                          0 );
          if( str_bsd_path == NULL )
            {
              IOObjectRelease( next_media );
              continue;
            }
          
          snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
          dev_path_length = strlen( psz_buf );
          
          if( CFStringGetCString( str_bsd_path,
                                  (char*)&psz_buf + dev_path_length,
                                  sizeof(psz_buf) - dev_path_length,
                                  kCFStringEncodingASCII ) )
            {
              CFRelease( str_bsd_path );
              IOObjectRelease( next_media );
              IOObjectRelease( media_iterator );
              return strdup( psz_buf );
            }
          
          CFRelease( str_bsd_path );
          IOObjectRelease( next_media );
          
        } while( ( next_media = IOIteratorNext( media_iterator ) ) != 0 );
    }
  IOObjectRelease( media_iterator );
  return NULL;
#endif /* HAVE_DARWIN_CDROM */
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_am_osx (const char *psz_source_name, const char *psz_access_mode)
{

  if (psz_access_mode != NULL)
    cdio_warn ("there is only one access mode for OS X. Arg %s ignored",
               psz_access_mode);
  return cdio_open_osx(psz_source_name);
}


/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_osx (const char *psz_orig_source)
{
#ifdef HAVE_DARWIN_CDROM
  CdIo_t *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs_t _funcs = {
    .eject_media           = _eject_media_osx,
    .free                  = _free_osx,
    .get_arg               = _get_arg_osx,
    .get_cdtext            = get_cdtext_osx,
    .get_default_device    = cdio_get_default_device_osx,
    .get_devices           = cdio_get_devices_osx,
    .get_disc_last_lsn     = get_disc_last_lsn_osx,
    .get_discmode          = get_discmode_osx,
    .get_drive_cap         = get_drive_cap_osx,
    .get_first_track_num   = get_first_track_num_generic,
    .get_hwinfo            = get_hwinfo_osx,
    .get_mcn               = get_mcn_osx,
    .get_num_tracks        = get_num_tracks_generic,
    .get_track_channels    = get_track_channels_generic,
    .get_track_copy_permit = get_track_copy_permit_generic,
    .get_track_format      = get_track_format_osx,
    .get_track_green       = get_track_green_osx,
    .get_track_lba         = get_track_lba_osx,
    .get_track_msf         = NULL,
    .get_track_preemphasis = get_track_preemphasis_generic,
    .lseek                 = cdio_generic_lseek,
    .read                  = cdio_generic_read,
    .read_audio_sectors    = read_audio_sectors_osx,
    .read_data_sectors     = read_data_sectors_osx,
    .read_mode1_sector     = read_mode1_sector_osx,
    .read_mode1_sectors    = read_mode1_sectors_osx,
    .read_mode2_sector     = read_mode2_sector_osx,
    .read_mode2_sectors    = read_mode2_sectors_osx,
    .read_toc              = read_toc_osx,
    .run_mmc_cmd           = run_mmc_cmd_osx,
    .set_arg               = _set_arg_osx,
    .set_speed             = set_speed_osx,
  };

  _data                     = calloc (1, sizeof (_img_private_t));
  _data->access_mode        = _AM_OSX;
  _data->MediaClass_service = 0;
  _data->gen.init           = false;
  _data->gen.fd             = -1;
  _data->gen.toc_init       = false;
  _data->gen.b_cdtext_init  = false;
  _data->gen.b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_osx();
    if (NULL == psz_source) {
      cdio_generic_free(_data);
      return NULL;
    }
    _set_arg_osx(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_osx(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is a not a device", psz_orig_source);
#endif
      cdio_generic_free(_data);
      return NULL;
    }
  }

  ret = cdio_new ((void *)_data, &_funcs);
  if (ret == NULL) {
    cdio_generic_free(_data);
    return NULL;
  }

  ret->driver_id = DRIVER_OSX;
  if (cdio_generic_init(_data, O_RDONLY | O_NONBLOCK) && init_osx(_data))
    return ret;
  else {
    cdio_generic_free (_data);
    return NULL;
  }
  
#else 
  return NULL;
#endif /* HAVE_DARWIN_CDROM */

}

bool
cdio_have_osx (void)
{
#ifdef HAVE_DARWIN_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_DARWIN_CDROM */
}
