/*
    $Id: device.c,v 1.39 2006/04/04 02:06:13 rocky Exp $

    Copyright (C) 2005, 2006 Rocky Bernstein <rocky@panix.com>

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
/*! device- and driver-related routines. */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cdio/logging.h>
#include "cdio_private.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* This probably will get moved to driver code, i.e _cdio_linux.c */
#ifdef HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

/* The below array gives of the drivers that are currently available for 
   on a particular host. */

CdIo_driver_t CdIo_driver[CDIO_MAX_DRIVER] = { {0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL} };

/* The last valid entry of Cdio_driver. 
   -1 or (CDIO_DRIVER_UNINIT) means uninitialzed. 
   -2 means some sort of error.
*/

#define CDIO_DRIVER_UNINIT -1
int CdIo_last_driver = CDIO_DRIVER_UNINIT;

#ifdef HAVE_AIX_CDROM
const driver_id_t cdio_os_driver = DRIVER_AIX;
#elif HAVE_BSDI_CDROM
const driver_id_t cdio_os_driver = DRIVER_BSDI;
#elif  HAVE_FREEBSD_CDROM
const driver_id_t cdio_os_driver = DRIVER_FREEBSD;
#elif  HAVE_LINUX_CDROM
const driver_id_t cdio_os_driver = DRIVER_LINUX;
#elif  HAVE_DARWIN_CDROM
const driver_id_t cdio_os_driver = DRIVER_OSX;
#elif  HAVE_DARWIN_SOLARIS
const driver_id_t cdio_os_driver = DRIVER_SOLARIS;
#elif  HAVE_DARWIN_WIN32
const driver_id_t cdio_os_driver = DRIVER_WIN32;
#else 
const driver_id_t cdio_os_driver = DRIVER_UNKNOWN;
#endif

/** The below variables are trickery to force enum symbol values to be
    recorded in debug symbol tables. They are used to allow one to refer
    to the enumeration value names in the typedefs above in a debugger
    and debugger expressions.
*/
cdio_drive_cap_misc_t          debug_cdio_drive_cap_misc;
cdio_drive_cap_read_t          debug_cdio_drive_cap_read_t;
cdio_drive_cap_write_t         debug_drive_cap_write_t;
cdio_mmc_hw_len_t              debug_cdio_mmc_hw_len;
cdio_src_category_mask_t       debug_cdio_src_category_mask;

static bool 
cdio_have_false(void)
{
  return false;
}

/* The below array gives all drivers that can possibly appear.
   on a particular host. */

CdIo_driver_t CdIo_all_drivers[CDIO_MAX_DRIVER+1] = {
  {DRIVER_UNKNOWN, 
   0,
   "Unknown", 
   "No driver",
   &cdio_have_false,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
  },

  {DRIVER_BSDI, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "AIX",
   "AIX SCSI driver",
   &cdio_have_aix,
   &cdio_open_aix,
   &cdio_open_am_aix,
   &cdio_get_default_device_aix,
   &cdio_is_device_generic,
   &cdio_get_devices_aix,
   NULL
  },

  {DRIVER_BSDI, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "BSDI",
   "BSDI ATAPI and SCSI driver",
   &cdio_have_bsdi,
   &cdio_open_bsdi,
   &cdio_open_am_bsdi,
   &cdio_get_default_device_bsdi,
   &cdio_is_device_generic,
   &cdio_get_devices_bsdi,
   &close_tray_bsdi
  },

  {DRIVER_FREEBSD, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "FreeBSD",
   "FreeBSD driver",
   &cdio_have_freebsd,
   &cdio_open_freebsd,
   &cdio_open_am_freebsd,
   &cdio_get_default_device_freebsd,
   &cdio_is_device_generic,
   &cdio_get_devices_freebsd,
   &close_tray_freebsd
  },

  {DRIVER_LINUX, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK,
   "GNU/Linux", 
   "GNU/Linux ioctl and MMC driver",
   &cdio_have_linux,
   &cdio_open_linux,
   &cdio_open_am_linux,
   &cdio_get_default_device_linux,
   &cdio_is_device_generic,
   &cdio_get_devices_linux,
   &close_tray_linux
  },

  {DRIVER_SOLARIS, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "Solaris",
   "Solaris ATAPI and SCSI driver",
   &cdio_have_solaris,
   &cdio_open_solaris,
   &cdio_open_am_solaris,
   &cdio_get_default_device_solaris,
   &cdio_is_device_generic,
   &cdio_get_devices_solaris,
   &close_tray_solaris
  },

  {DRIVER_OSX, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "OS X",
   "Apple Darwin OS X driver",
   &cdio_have_osx,
   &cdio_open_osx,
   &cdio_open_am_osx,
   &cdio_get_default_device_osx,
   &cdio_is_device_generic,
   &cdio_get_devices_osx,
   &close_tray_osx
  },

  {DRIVER_WIN32, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "WIN32",
   "MS Windows ASPI and ioctl driver",
   &cdio_have_win32,
   &cdio_open_win32,
   &cdio_open_am_win32,
   &cdio_get_default_device_win32,
   &cdio_is_device_win32,
   &cdio_get_devices_win32,
   &close_tray_win32
  },

  {DRIVER_CDRDAO,
   CDIO_SRC_IS_DISK_IMAGE_MASK,
   "CDRDAO",
   "cdrdao (TOC) disk image driver",
   &cdio_have_cdrdao,
   &cdio_open_cdrdao,
   &cdio_open_am_cdrdao,
   &cdio_get_default_device_cdrdao,
   NULL,
   &cdio_get_devices_cdrdao,
   NULL
  },

  {DRIVER_BINCUE,
   CDIO_SRC_IS_DISK_IMAGE_MASK,
   "BIN/CUE",
   "bin/cuesheet disk image driver",
   &cdio_have_bincue,
   &cdio_open_bincue,
   &cdio_open_am_bincue,
   &cdio_get_default_device_bincue,
   NULL,
   &cdio_get_devices_bincue,
   NULL
  },

  {DRIVER_NRG,
   CDIO_SRC_IS_DISK_IMAGE_MASK,
   "NRG",
   "Nero NRG disk image driver",
   &cdio_have_nrg,
   &cdio_open_nrg,
   &cdio_open_am_nrg,
   &cdio_get_default_device_nrg,
   NULL,
   &cdio_get_devices_nrg,
   NULL
  }

};

const char *
cdio_driver_errmsg(driver_return_code_t drc)
{
  switch(drc) {
  case DRIVER_OP_SUCCESS: 
    return "driver operation was successful";
  case DRIVER_OP_ERROR:
    return "driver I/O error";
  case DRIVER_OP_UNSUPPORTED:
    return "driver operatation not supported";
  case DRIVER_OP_UNINIT:
    return "driver not initialized";
  case DRIVER_OP_NOT_PERMITTED:
    return "driver operatation not permitted";
  case DRIVER_OP_BAD_PARAMETER:
    return "bad parameter passed";
  case DRIVER_OP_BAD_POINTER:
    return "bad pointer to memory area";
  case DRIVER_OP_NO_DRIVER:
    return "driver not available";
  default:
    return "unknown or bad driver return status";
  }
}

static CdIo *
scan_for_driver(driver_id_t start, driver_id_t end, 
                const char *psz_source, const char *access_mode) 
{
  driver_id_t driver_id;
  
  for (driver_id=start; driver_id<=end; driver_id++) {
    if ((*CdIo_all_drivers[driver_id].have_driver)()) {
      CdIo *ret=
        (*CdIo_all_drivers[driver_id].driver_open_am)(psz_source, access_mode);
      if (ret != NULL) {
        ret->driver_id = driver_id;
        return ret;
      }
    }
  }
  return NULL;
}

const char *
cdio_driver_describe(driver_id_t driver_id)
{
  return CdIo_all_drivers[driver_id].describe;
}

/*!
  Initialize CD Reading and control routines. Should be called first.
  May be implicitly called by other routines if not called first.
*/
bool
cdio_init(void)
{
  
  CdIo_driver_t *all_dp;
  CdIo_driver_t *dp = CdIo_driver;
  driver_id_t driver_id;

  if (CdIo_last_driver != CDIO_DRIVER_UNINIT) {
    cdio_warn ("Init routine called more than once.");
    return false;
  }

  for (driver_id=DRIVER_UNKNOWN+1; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
    all_dp = &CdIo_all_drivers[driver_id];
    if ((*CdIo_all_drivers[driver_id].have_driver)()) {
      *dp++ = *all_dp;
      CdIo_last_driver++;
    }
  }

  return true;
}

/*!
  Free any resources associated with cdio.
*/
void
cdio_destroy (CdIo_t *p_cdio)
{
  CdIo_last_driver = CDIO_DRIVER_UNINIT;
  if (p_cdio == NULL) return;

  if (p_cdio->op.free != NULL && p_cdio->env) 
    p_cdio->op.free (p_cdio->env);
  p_cdio->env = NULL;
  free (p_cdio);
}

/*!
  Close media tray in CD drive if there is a routine to do so. 
  
  @param psz_drive the name of CD-ROM to be closed. If NULL, we will
  use the default device.
  @param p_driver_id is the driver to be used or that got used if
  it was DRIVER_UNKNOWN or DRIVER_DEVICE; If this is NULL, we won't
  report back the driver used.
*/
driver_return_code_t 
cdio_close_tray (const char *psz_orig_drive, /*in/out*/ driver_id_t
                 *p_driver_id)
{
  driver_id_t temp_driver_id = DRIVER_DEVICE;
  char *psz_drive;
  driver_return_code_t drc;

  if (!p_driver_id) p_driver_id = &temp_driver_id;
  
  if (!psz_orig_drive || !*psz_orig_drive)
    psz_drive = cdio_get_default_device_driver(p_driver_id);
  else 
    psz_drive = strdup(psz_orig_drive);
  
  if (DRIVER_UNKNOWN == *p_driver_id || DRIVER_DEVICE == *p_driver_id) {
    *p_driver_id = CDIO_MIN_DEVICE_DRIVER;
    
    /* Scan for driver */
    for ( ; *p_driver_id<=CDIO_MAX_DRIVER; (*p_driver_id)++) {
      if ( (*CdIo_all_drivers[*p_driver_id].have_driver)() &&
           *CdIo_all_drivers[*p_driver_id].close_tray ) {
        drc = (*CdIo_all_drivers[*p_driver_id].close_tray)(psz_drive);
        free(psz_drive);
        return drc;
      }
    }
    return DRIVER_OP_UNSUPPORTED;
  }
  
  /* The driver id was specified. Use that. */
  if ( (*CdIo_all_drivers[*p_driver_id].have_driver)() &&
       *CdIo_all_drivers[*p_driver_id].close_tray ) {
    drc = (*CdIo_all_drivers[*p_driver_id].close_tray)(psz_drive);
    free(psz_drive);
    return drc;
  }
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Eject media in CD drive if there is a routine to do so. 
  
  @param p_cdio the CD object to be acted upon.
  If the CD is ejected *p_cdio is freed and p_cdio set to NULL.
*/
driver_return_code_t
cdio_eject_media (CdIo_t **pp_cdio)
{
  if ((pp_cdio == NULL) || (*pp_cdio == NULL)) return DRIVER_OP_UNINIT;

  if ((*pp_cdio)->op.eject_media) {
    int ret = (*pp_cdio)->op.eject_media ((*pp_cdio)->env);
    if (0 == ret) {
      cdio_destroy(*pp_cdio);
      *pp_cdio = NULL;
    }
    return ret;
  } else {
    cdio_destroy(*pp_cdio);
    *pp_cdio = NULL;
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Eject media in CD drive if there is a routine to do so. If you want
  to scan for any CD-ROM and eject that, pass NULL for psz_drive.
  
  @param psz_drive the CD object to be acted upon.
  If NULL is given as the drive, we'll use the default driver device.
*/
driver_return_code_t
cdio_eject_media_drive (const char *psz_drive)
{
  CdIo_t *p_cdio = cdio_open (psz_drive, DRIVER_DEVICE);
  if (p_cdio) {
    return cdio_eject_media(&p_cdio);
  } else {
    return DRIVER_OP_UNINIT;
  }
}

/*!
  Free device list returned by cdio_get_devices or
  cdio_get_devices_with_cap.
*/
void 
cdio_free_device_list (char * ppsz_device_list[]) 
{
  char **ppsz_device_list_save=ppsz_device_list;
  if (!ppsz_device_list) return;
  for ( ; NULL != *ppsz_device_list ; ppsz_device_list++ ) {
    free(*ppsz_device_list);
    *ppsz_device_list = NULL;
  }
  free(ppsz_device_list_save);
}


/*!
  Return a string containing the default CD device if none is specified.
  if p_cdio is NULL (we haven't initialized a specific device driver), 
  then find a suitable one and return the default device for that.

  NULL is returned if we couldn't get a default device.
 */
char *
cdio_get_default_device (const CdIo_t *p_cdio)
{
  if (p_cdio == NULL) {
    driver_id_t driver_id;
    /* Scan for driver */
    for (driver_id=DRIVER_UNKNOWN+1; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
      if ( (*CdIo_all_drivers[driver_id].have_driver)() &&
           *CdIo_all_drivers[driver_id].get_default_device ) {
        return (*CdIo_all_drivers[driver_id].get_default_device)();
      }
    }
    return NULL;
  }
  
  if (p_cdio->op.get_default_device) {
    return p_cdio->op.get_default_device ();
  } else {
    return NULL;
  }
}

/*!
  Return a string containing the default CD device if none is specified.
  if p_driver_id is DRIVER_UNKNOWN or DRIVER_DEVICE
  then find a suitable one set the default device for that.

  NULL is returned if we couldn't get a default device.
 */
char *
cdio_get_default_device_driver (/*in/out*/ driver_id_t *p_driver_id)
{
  if (DRIVER_UNKNOWN == *p_driver_id || DRIVER_DEVICE == *p_driver_id) {
    if (DRIVER_UNKNOWN == *p_driver_id) 
      (*p_driver_id)++;
    else 
      *p_driver_id = CDIO_MIN_DEVICE_DRIVER;
    
    /* Scan for driver */
    for ( ; *p_driver_id<=CDIO_MAX_DRIVER; (*p_driver_id)++) {
      if ( (*CdIo_all_drivers[*p_driver_id].have_driver)() &&
           *CdIo_all_drivers[*p_driver_id].get_default_device ) {
        return (*CdIo_all_drivers[*p_driver_id].get_default_device)();
      }
    }
    return NULL;
  }
  
  /* The driver id was specified. Use that. */
  if ( (*CdIo_all_drivers[*p_driver_id].have_driver)() &&
       *CdIo_all_drivers[*p_driver_id].get_default_device ) {
    return (*CdIo_all_drivers[*p_driver_id].get_default_device)();
  }
  return NULL;
}

/*!Return an array of device names. If you want a specific
  devices, dor a driver give that device, if you want hardware
  devices, give DRIVER_DEVICE and if you want all possible devices,
  image drivers and hardware drivers give DRIVER_UNKNOWN.
  
  NULL is returned if we couldn't return a list of devices.
*/
char **
cdio_get_devices (driver_id_t driver_id)
{
  /* Probably could get away with &driver_id below. */
  driver_id_t driver_id_temp = driver_id; 
  return cdio_get_devices_ret (&driver_id_temp);
}

char **
cdio_get_devices_ret (/*in/out*/ driver_id_t *p_driver_id)
{
  CdIo_t *p_cdio;

  switch (*p_driver_id) {
    /* FIXME: spit out unknown to give image drivers as well.  */
  case DRIVER_DEVICE:
    p_cdio = scan_for_driver(DRIVER_UNKNOWN+1, CDIO_MAX_DEVICE_DRIVER, 
                             NULL, NULL);
    *p_driver_id = cdio_get_driver_id(p_cdio);
    break;
  case DRIVER_UNKNOWN:
    p_cdio = scan_for_driver(DRIVER_UNKNOWN+1, CDIO_MAX_DRIVER, NULL, NULL);
    *p_driver_id = cdio_get_driver_id(p_cdio);
    break;
  default:
    return (*CdIo_all_drivers[*p_driver_id].get_devices)();
  }
  
  if (p_cdio == NULL) return NULL;
  if (p_cdio->op.get_devices) {
    char **devices = p_cdio->op.get_devices ();
    cdio_destroy(p_cdio);
    return devices;
  } else {
    return NULL;
  }
}

/*!
  Return an array of device names in search_devices that have at
  least the capabilities listed by cap.  If search_devices is NULL,
  then we'll search all possible CD drives.  
  
  If "any" is set false then every capability listed in the extended
  portion of capabilities (i.e. not the basic filesystem) must be
  satisified. If "any" is set true, then if any of the capabilities
  matches, we call that a success.
  
  To find a CD-drive of any type, use the mask CDIO_FS_MATCH_ALL.
  
  NULL is returned if we couldn't get a default device.
  It is also possible to return a non NULL but after dereferencing the 
  the value is NULL. This also means nothing was found.
*/
char **
cdio_get_devices_with_cap (/*in*/ char* search_devices[], 
                           cdio_fs_anal_t capabilities, bool any)
{
  driver_id_t p_driver_id;
  return cdio_get_devices_with_cap_ret (search_devices, capabilities, any,
                                        &p_driver_id);
}

char **
cdio_get_devices_with_cap_ret (/*in*/ char* search_devices[], 
                               cdio_fs_anal_t need_cap, bool b_any,
                               /*out*/ driver_id_t *p_driver_id)
{
  char **ppsz_drives=search_devices;
  char **ppsz_drives_ret=NULL;
  unsigned int i_drives=0;
  bool b_free_ppsz_drives = false;

  *p_driver_id = DRIVER_DEVICE;

  if (!ppsz_drives) {
    ppsz_drives=cdio_get_devices_ret(p_driver_id);
    b_free_ppsz_drives = true;
  }
  
  if (!ppsz_drives) return NULL;

  if (need_cap == CDIO_FS_MATCH_ALL) {
    /* Duplicate drives into drives_ret. */
    char **d = ppsz_drives;
    
    for( ; *d != NULL; d++ ) {
      cdio_add_device_list(&ppsz_drives_ret, *d, &i_drives);
    }
  } else {
    const cdio_fs_anal_t need_fs = CDIO_FSTYPE(need_cap);
    char **d = ppsz_drives;
      
    for( ;  *d != NULL; d++ ) {
      CdIo_t *p_cdio = cdio_open(*d, *p_driver_id);
      
      if (NULL != p_cdio) {
        track_t i_first_track = cdio_get_first_track_num(p_cdio);
        cdio_iso_analysis_t cdio_iso_analysis; 

        if (CDIO_INVALID_TRACK != i_first_track) {
          const cdio_fs_anal_t got_cap = 
            cdio_guess_cd_type(p_cdio, 0, i_first_track, &cdio_iso_analysis);

          /* Match on filesystem. Here either we don't know what the
             filesystem is - automatic match, or we no that the file
             system is in the set of those specified.
             We refine the logic further after this initial test. */
          if ( CDIO_FS_UNKNOWN == need_fs || 0 == need_fs
               || (CDIO_FSTYPE(got_cap) == need_fs) ) {
              /* Match on analysis type. If we haven't set any 
                 analysis type, then an automatic match. Otherwise
                 a match is determined by whether we need all 
                 analysis types or any of them. */
              const cdio_fs_anal_t need_anal = need_cap & ~CDIO_FS_MASK;
              const cdio_fs_anal_t got_anal  = got_cap  & ~CDIO_FS_MASK;
              const bool b_match = !need_anal 
                || (b_any  
                    ? (got_anal & need_anal) != 0
                    : (got_anal & need_anal) == need_anal);
              if (b_match) 
                cdio_add_device_list(&ppsz_drives_ret, *d, &i_drives);
            }
        }
             
        cdio_destroy(p_cdio);
      }
    }
  }
  cdio_add_device_list(&ppsz_drives_ret, NULL, &i_drives);
  if (b_free_ppsz_drives) {
    cdio_free_device_list(ppsz_drives);
  }
  return ppsz_drives_ret;
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
void
cdio_get_drive_cap (const CdIo_t *p_cdio, 
                    cdio_drive_read_cap_t  *p_read_cap,
                    cdio_drive_write_cap_t *p_write_cap,
                    cdio_drive_misc_cap_t  *p_misc_cap)
{
  /* This seems like a safe bet. */
  *p_read_cap  = CDIO_DRIVE_CAP_UNKNOWN;
  *p_write_cap = CDIO_DRIVE_CAP_UNKNOWN;
  *p_misc_cap  = CDIO_DRIVE_CAP_UNKNOWN;
  
  if (p_cdio && p_cdio->op.get_drive_cap) {
    p_cdio->op.get_drive_cap(p_cdio->env, p_read_cap, p_write_cap, p_misc_cap);
  }
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
void
cdio_get_drive_cap_dev (const char *device,
			cdio_drive_read_cap_t  *p_read_cap,
			cdio_drive_write_cap_t *p_write_cap,
			cdio_drive_misc_cap_t  *p_misc_cap)
{
  /* This seems like a safe bet. */
  CdIo_t *cdio=scan_for_driver(CDIO_MIN_DRIVER, CDIO_MAX_DRIVER, 
                             device, NULL);
  if (cdio) {
    cdio_get_drive_cap(cdio, p_read_cap, p_write_cap, p_misc_cap);
    cdio_destroy(cdio);
  } else {
    *p_read_cap  = CDIO_DRIVE_CAP_UNKNOWN;
    *p_write_cap = CDIO_DRIVE_CAP_UNKNOWN;
    *p_misc_cap  = CDIO_DRIVE_CAP_UNKNOWN;
  }
}


/*!
  Return a string containing the name of the driver in use.
  if CdIo is NULL (we haven't initialized a specific device driver), 
  then return NULL.
*/
const char *
cdio_get_driver_name (const CdIo_t *p_cdio) 
{
  if (NULL==p_cdio) return NULL;
  return CdIo_all_drivers[p_cdio->driver_id].name;
}

/*!
  Return the driver id. 
  if CdIo is NULL (we haven't initialized a specific device driver), 
  then return DRIVER_UNKNOWN.
*/
driver_id_t
cdio_get_driver_id (const CdIo_t *p_cdio) 
{
  if (!p_cdio) return DRIVER_UNKNOWN;
  return p_cdio->driver_id;
}

/*!
  Return a string containing the name of the driver in use.
  if CdIo is NULL (we haven't initialized a specific device driver), 
  then return NULL.
*/
bool
cdio_get_hwinfo (const CdIo_t *p_cdio, cdio_hwinfo_t *hw_info) 
{
  if (!p_cdio) return false;
  if (p_cdio->op.get_hwinfo) {
    return p_cdio->op.get_hwinfo (p_cdio, hw_info);
  } else {
    /* Perhaps driver forgot to initialize.  We are no worse off Using
      mmc than returning false here. */
    return mmc_get_hwinfo(p_cdio, hw_info);
  }
}

/*!
  Return the session number of the last on the CD. 
  
  @param p_cdio the CD object to be acted upon.
  @param i_last_session pointer to the session number to be returned.
*/
driver_return_code_t cdio_get_last_session (CdIo_t *p_cdio,
                                            /*out*/ lsn_t *i_last_session)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (p_cdio->op.get_last_session)
    return p_cdio->op.get_last_session(p_cdio->env, i_last_session);
  return DRIVER_OP_UNSUPPORTED;
}

/*! 
  Find out if media has changed since the last call.
  @param p_cdio the CD object to be acted upon.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
*/
int
cdio_get_media_changed(CdIo_t *p_cdio)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (p_cdio->op.get_media_changed)
    return p_cdio->op.get_media_changed(p_cdio->env);
  return DRIVER_OP_UNSUPPORTED;
}

bool_3way_t
cdio_have_atapi(CdIo_t *p_cdio)
{
  bool_3way_t i_status;

  if (!p_cdio) return nope;
  i_status = mmc_have_interface(p_cdio, CDIO_MMC_FEATURE_INTERFACE_ATAPI);
  if (dunno != i_status) return i_status;

  {
    /* cdparanoia seems to think that if we have a mode sense command
       we have an atapi drive or is atapi compatible.
     */
    uint8_t buf[22];
    if (DRIVER_OP_SUCCESS == mmc_mode_sense(p_cdio, buf, sizeof(buf),
                                            CDIO_MMC_CAPABILITIES_PAGE) ) {
      uint8_t *b = buf;
      b+=b[3]+4;
      if( CDIO_MMC_CAPABILITIES_PAGE == (b[0]&0x3F) ) {
        /* MMC style drive! */
        return yep;
      }
    }
  }
  
  /* Put these in the various drivers? If we get more, yes!
   */
#ifdef HAVE_LINUX_MAJOR_H
  {
    /* This too is from cdparanoia. */
    struct stat st;
    generic_img_private_t *p_env  = p_cdio->env;
    if ( 0 == lstat(p_env->source_name, &st) ) {
      if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
        int drive_type=(int)(st.st_rdev>>8);
        switch (drive_type) {
        case IDE0_MAJOR:
        case IDE1_MAJOR:
        case IDE2_MAJOR:
        case IDE3_MAJOR:
          /* Yay, ATAPI... */
          return yep;
          break;
        case CDU31A_CDROM_MAJOR:
        case CDU535_CDROM_MAJOR:
        case MATSUSHITA_CDROM_MAJOR:
        case MATSUSHITA_CDROM2_MAJOR:
        case MATSUSHITA_CDROM3_MAJOR:
        case MATSUSHITA_CDROM4_MAJOR:
        case SANYO_CDROM_MAJOR:
        case MITSUMI_CDROM_MAJOR:
        case MITSUMI_X_CDROM_MAJOR:
        case OPTICS_CDROM_MAJOR:
        case AZTECH_CDROM_MAJOR:
        case GOLDSTAR_CDROM_MAJOR:
        case CM206_CDROM_MAJOR:
        case SCSI_CDROM_MAJOR:   
        case SCSI_GENERIC_MAJOR: 
          return nope;
	break;
        default:
          return dunno;
        }
      }
    }
  }
#endif /*HAVE_LINUX_MAJOR_H*/
  return dunno;
}

bool
cdio_have_driver(driver_id_t driver_id)
{
  return (*CdIo_all_drivers[driver_id].have_driver)();
}

bool
cdio_is_device(const char *psz_source, driver_id_t driver_id)
{
  if (DRIVER_UNKNOWN == driver_id || DRIVER_DEVICE == driver_id) {
    if (DRIVER_UNKNOWN == driver_id) 
      driver_id++;
    else 
      driver_id = CDIO_MIN_DEVICE_DRIVER;
    
    /* Scan for driver */
    for ( ; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
      if ( (*CdIo_all_drivers[driver_id].have_driver)() &&
           CdIo_all_drivers[driver_id].is_device ) {
        return (*CdIo_all_drivers[driver_id].is_device)(psz_source);
      }
    }
  }
  if (CdIo_all_drivers[driver_id].is_device == NULL) return false;
  return (*CdIo_all_drivers[driver_id].is_device)(psz_source);
}


/*! Sets up to read from place specified by source_name and
  driver_id. This should be called before using any other routine,
  except cdio_init. This will call cdio_init, if that hasn't been
  done previously.
  
  NULL is returned on error.
*/
CdIo_t *
cdio_open (const char *orig_source_name, driver_id_t driver_id)
{
  return cdio_open_am(orig_source_name, driver_id, NULL);
}

/*! Sets up to read from place specified by source_name and
  driver_id. This should be called before using any other routine,
  except cdio_init. This will call cdio_init, if that hasn't been
  done previously.
  
  NULL is returned on error.
*/
CdIo_t *
cdio_open_am (const char *psz_orig_source, driver_id_t driver_id,
              const char *psz_access_mode)
{
  char *psz_source;
  
  if (CdIo_last_driver == -1) cdio_init();

  if (!psz_orig_source || !*psz_orig_source)
    psz_source = cdio_get_default_device(NULL);
  else 
    psz_source = strdup(psz_orig_source);
  
  switch (driver_id) {
  case DRIVER_UNKNOWN: 
    {
      CdIo_t *p_cdio=scan_for_driver(CDIO_MIN_DRIVER, CDIO_MAX_DRIVER, 
                                     psz_source, psz_access_mode);
      free(psz_source);
      return p_cdio;
    }
  case DRIVER_DEVICE: 
    {  
      /* Scan for a driver. */
      CdIo_t *ret = cdio_open_am_cd(psz_source, psz_access_mode);
      free(psz_source);
      return ret;
    }
    break;
  case DRIVER_AIX:
  case DRIVER_BSDI:
  case DRIVER_FREEBSD:
  case DRIVER_LINUX:
  case DRIVER_SOLARIS:
  case DRIVER_WIN32:
  case DRIVER_OSX:
  case DRIVER_NRG:
  case DRIVER_BINCUE:
  case DRIVER_CDRDAO:
    if ((*CdIo_all_drivers[driver_id].have_driver)()) {
      CdIo_t *ret = 
        (*CdIo_all_drivers[driver_id].driver_open_am)(psz_source, 
                                                      psz_access_mode);
      if (ret) ret->driver_id = driver_id;
      free(psz_source);
      return ret;
    }
  }

  free(psz_source);
  return NULL;
}


/*! 
  Set up CD-ROM for reading. The device_name is
  the some sort of device name.
  
  @return the cdio object for subsequent operations. 
  NULL on error or there is no driver for a some sort of hardware CD-ROM.
*/
CdIo_t *
cdio_open_cd (const char *psz_source)
{
  return cdio_open_am_cd(psz_source, NULL);
}

/*! 
  Set up CD-ROM for reading. The device_name is
  the some sort of device name.
  
  @return the cdio object for subsequent operations. 
  NULL on error or there is no driver for a some sort of hardware CD-ROM.
*/
/* In the future we'll have more complicated code to allow selection
   of an I/O routine as well as code to find an appropriate default
   routine among the "registered" routines. Possibly classes too
   disk-based, SCSI-based, native-based, vendor (e.g. Sony, or
   Plextor) based 

   For now though, we'll start more simply...
*/
CdIo_t *
cdio_open_am_cd (const char *psz_source, const char *psz_access_mode)
{
  if (CdIo_last_driver == -1) cdio_init();

  /* Scan for a driver. */
  return scan_for_driver(CDIO_MIN_DEVICE_DRIVER, CDIO_MAX_DEVICE_DRIVER, 
                         psz_source, psz_access_mode);
}

/*!
  Set the blocksize for subsequent reads. 
*/
driver_return_code_t 
cdio_set_blocksize ( const CdIo_t *p_cdio, int i_blocksize )
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (p_cdio->op.set_blocksize) return DRIVER_OP_UNSUPPORTED;
  return p_cdio->op.set_blocksize(p_cdio->env, i_blocksize);
}

/*!
  Set the drive speed. 
  
  @param p_cdio          CD structure set by cdio_open().
  @param i_drive_speed   speed in CD-ROM speed units. Note this
                         not Kbs as would be used in the MMC spec or
		         in mmc_set_speed(). To convert CD-ROM speed units 
			 to Kbs, multiply the number by 176 (for raw data)
			 and by 150 (for filesystem data). On many CD-ROM 
			 drives, specifying a value too large will result 
			 in using the fastest speed.

  @see mmc_set_speed and mmc_set_drive_speed
*/
driver_return_code_t
cdio_set_speed (const CdIo_t *p_cdio, int i_speed) 
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (!p_cdio->op.set_speed) return DRIVER_OP_UNSUPPORTED;
  return p_cdio->op.set_speed(p_cdio->env, i_speed);
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
