/* -*- c -*-
    $Id: device.h,v 1.9 2005/01/29 20:54:20 rocky Exp $

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

/** \file device.h 
 *
 *  \brief The top-level header for driver- or device-related libcdio
 *          calls.  ("device" include CD-image reading devices).
 */
#ifndef __CDIO_DEVICE_H__
#define __CDIO_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! Size of fields returned by an INQUIRY command */
#define CDIO_MMC_HW_VENDOR_LEN    8 /**< length of vendor field */
#define CDIO_MMC_HW_MODEL_LEN    16 /**< length of model field */
#define CDIO_MMC_HW_REVISION_LEN  4 /**< length of revision field */

  /*! \brief Structure to return CD vendor, model, and revision-level
      strings obtained via the INQUIRY command  */
  typedef struct cdio_hwinfo 
  {
    char psz_vendor  [CDIO_MMC_HW_VENDOR_LEN+1];
    char psz_model   [CDIO_MMC_HW_MODEL_LEN+1];
    char psz_revision[CDIO_MMC_HW_REVISION_LEN+1];
  } cdio_hwinfo_t;

  /**! Flags specifying the category of device to open or is opened. */

#define CDIO_SRC_IS_DISK_IMAGE_MASK 0x0001 /**< Read source is a CD image. */
#define CDIO_SRC_IS_DEVICE_MASK     0x0002 /**< Read source is a CD device. */
#define CDIO_SRC_IS_SCSI_MASK       0x0004 /**< Read source SCSI device. */
#define CDIO_SRC_IS_NATIVE_MASK     0x0008

  /** The driver_id_t enumerations may be used to tag a specific driver
   * that is opened or is desired to be opened. Note that this is
   * different than what is available on a given host.
   *
   * Order is a little significant since the order is used in scans.
   * We have to start with DRIVER_UNKNOWN and devices should come before
   * disk-image readers. By putting something towards the top (a lower
   * enumeration number), in an iterative scan we prefer that to
   * something with a higher enumeration number.
   *
   * NOTE: IF YOU MODIFY ENUM MAKE SURE INITIALIZATION IN CDIO.C AGREES.
   *     
   */
  typedef enum  {
    DRIVER_UNKNOWN, /**< Used as input when we don't care what kind 
		         of driver to use. */
    DRIVER_AIX,     /**< AIX driver */
    DRIVER_BSDI,    /**< BSDI driver */
    DRIVER_FREEBSD, /**< FreeBSD driver - includes CAM and ioctl access */
    DRIVER_LINUX,   /**< GNU/Linux Driver */
    DRIVER_SOLARIS, /**< Sun Solaris Driver */
    DRIVER_OSX,     /**< Apple OSX Driver */
    DRIVER_WIN32,   /**< Microsoft Windows Driver. Includes ASPI and 
		         ioctl acces. */
    DRIVER_CDRDAO,  /**< cdrdao format CD image. This is listed
		         before BIN/CUE, to make the code prefer cdrdao
		         over BIN/CUE when both exist. */
    DRIVER_BINCUE,  /**< CDRWIN BIN/CUE format CD image. This is
		         listed before NRG, to make the code prefer
		         BIN/CUE over NRG when both exist. */
    DRIVER_NRG,     /**< Nero NRG format CD image. */
    DRIVER_DEVICE   /**< Is really a set of the above; should come last */
  } driver_id_t;

  /** There will generally be only one hardware for a given
     build/platform from the list above. You can use the variable
     below to determine which you've got. If the build doesn't make an
     hardware driver, then the value will be DRIVER_UNKNOWN.
  */
  extern const driver_id_t cdio_os_driver;
  

/** Make sure what's listed for CDIO_MIN_DRIVER is the last
    enumeration in driver_id_t. Since we have a bogus (but useful) 0th
    entry above we don't have to add one.
*/
#define CDIO_MIN_DRIVER        DRIVER_AIX
#define CDIO_MIN_DEVICE_DRIVER CDIO_MIN_DRIVER
#define CDIO_MAX_DRIVER        DRIVER_NRG
#define CDIO_MAX_DEVICE_DRIVER DRIVER_WIN32

  /** The following are status codes for completion of a given cdio
     operation. By design 0 is successful completion and -1 is error
     completion. This is compatable with ioctl so those routines that
     call ioctl can just pass the value the get back (cast as this
     enum). Also, by using negative numbers for errors, the
     enumeration values below can be used in places where a positive
     value is expected when things complete successfully. For example,
     get_blocksize returns the blocksize, but on error uses the error
     codes below. So note that this enumeration is often cast to an
     integer.  C seems to tolerate this.
  */
  typedef enum  {
    DRIVER_OP_UNINIT = -3,     /**< returned when a particular driver
				   hasn't been initialized or a null
				   CdIo_t pointer has been passed.
				*/
    DRIVER_OP_UNSUPPORTED = -2, /**< returned when a particular driver
				   doesn't support a particular operation.
				   For example an image driver which doesn't
				   really "eject" a CD.
				*/
    DRIVER_OP_ERROR = -1,       /**< operation returned an error */
    DRIVER_OP_SUCCESS = 0,      /**< in cases where an int is returned,
				     like cdio_set_speed, more the negative
				     return codes are for errors and the 
				     positive ones for success. */
  } driver_return_code_t;

  /*!
    Eject media in CD drive if there is a routine to do so. 

    @param p_cdio the CD object to be acted upon.
    If the CD is ejected *p_cdio is freed and p_cdio set to NULL.
  */
  driver_return_code_t cdio_eject_media (CdIo_t **p_cdio);

  /*!
    Free device list returned by cdio_get_devices or
    cdio_get_devices_with_cap.
    
    @param device_list list returned by cdio_get_devices or
    cdio_get_devices_with_cap

    @see cdio_get_devices, cdio_get_devices_with_cap

  */
  void cdio_free_device_list (char * device_list[]);

  /*!
    Get the default CD device.
    if p_cdio is NULL (we haven't initialized a specific device driver), 
    then find a suitable one and return the default device for that.
    
    @param p_cdio the CD object queried
    @return a string containing the default CD device or NULL is
    if we couldn't get a default device.

    In some situations of drivers or OS's we can't find a CD device if
    there is no media in it and it is possible for this routine to return
    NULL even though there may be a hardware CD-ROM.
  */
  char * cdio_get_default_device (const CdIo_t *p_cdio);

  /*! Return an array of device names. If you want a specific
    devices for a driver, give that device. If you want hardware
    devices, give DRIVER_DEVICE and if you want all possible devices,
    image drivers and hardware drivers give DRIVER_UNKNOWN.
    
    NULL is returned if we couldn't return a list of devices.

    In some situations of drivers or OS's we can't find a CD device if
    there is no media in it and it is possible for this routine to return
    NULL even though there may be a hardware CD-ROM.
  */
  char ** cdio_get_devices (driver_id_t driver_id);

  /*!
    Get an array of device names in search_devices that have at least
    the capabilities listed by the capabities parameter.  If
    search_devices is NULL, then we'll search all possible CD drives.
    
    If "b_any" is set false then every capability listed in the
    extended portion of capabilities (i.e. not the basic filesystem)
    must be satisified. If "any" is set true, then if any of the
    capabilities matches, we call that a success.

    To find a CD-drive of any type, use the mask CDIO_FS_MATCH_ALL.

    @return the array of device names or NULL if we couldn't get a
    default device.  It is also possible to return a non NULL but
    after dereferencing the the value is NULL. This also means nothing
    was found.
  */
  char ** cdio_get_devices_with_cap (char* ppsz_search_devices[],
				     cdio_fs_anal_t capabilities, bool b_any);

  /*!
    Like cdio_get_devices_with_cap but we return the driver we found
    as well. This is because often one wants to search for kind of drive
    and then *open* it afterwards. Giving the driver back facilitates this,
    and speeds things up for libcdio as well.
  */
  char ** cdio_get_devices_with_cap_ret (/*out*/ char* ppsz_search_devices[],
					 cdio_fs_anal_t capabilities, 
					 bool b_any,
					 /*out*/ driver_id_t *p_driver_id);

  /*! Like cdio_get_devices, but we may change the p_driver_id if we
      were given DRIVER_DEVICE or DRIVER_UNKNOWN. This is because
      often one wants to get a drive name and then *open* it
      afterwards. Giving the driver back facilitates this, and speeds
      things up for libcdio as well.
   */
    
  char ** cdio_get_devices_ret (/*in/out*/ driver_id_t *p_driver_id);

  /*!
    Get the what kind of device we've got.

    @param p_cdio the CD object queried
    @param p_read_cap pointer to return read capabilities
    @param p_write_cap pointer to return write capabilities
    @param p_misc_cap pointer to return miscellaneous other capabilities

    In some situations of drivers or OS's we can't find a CD device if
    there is no media in it and it is possible for this routine to return
    NULL even though there may be a hardware CD-ROM.
  */
  void cdio_get_drive_cap (const CdIo_t *p_cdio,
			   cdio_drive_read_cap_t  *p_read_cap,
			   cdio_drive_write_cap_t *p_write_cap,
			   cdio_drive_misc_cap_t  *p_misc_cap);
  
  /*!
    Get the drive capabilities for a specified device.

    @return a list of device capabilities.

    In some situations of drivers or OS's we can't find a CD device if
    there is no media in it and it is possible for this routine to return
    NULL even though there may be a hardware CD-ROM.
  */
  void cdio_get_drive_cap_dev (const char *device,
			       cdio_drive_read_cap_t  *p_read_cap,
			       cdio_drive_write_cap_t *p_write_cap,
			       cdio_drive_misc_cap_t  *p_misc_cap);

  /*!
    Get a string containing the name of the driver in use.

    @return a string with driver name or NULL if CdIo_t is NULL (we
    haven't initialized a specific device.
  */
  const char * cdio_get_driver_name (const CdIo_t *p_cdio);

  /*!
    Get the driver id. 
    if CdIo_t is NULL (we haven't initialized a specific device driver), 
    then return DRIVER_UNKNOWN.

    @return the driver id..
  */
  driver_id_t cdio_get_driver_id (const CdIo_t *p_cdio);

  /*! 
    Get the CD-ROM hardware info via a SCSI MMC INQUIRY command.
    False is returned if we had an error getting the information.
  */
  bool cdio_get_hwinfo ( const CdIo_t *p_cdio, 
			 /* out*/ cdio_hwinfo_t *p_hw_info );


  /*!
    Get the drive speed. 

    @return the drive speed if greater than 0. DRIVER_OP_ERROR if we
    had an error, DRIVER_OP_UNSUPPORTED if this is not implemented for
    the current driver.
    
    @see cdio_set_speed
  */
  int cdio_get_speed ( const CdIo_t *p_cdio, int i_speed );

  /*! True if AIX driver is available. */
  bool cdio_have_aix    (void);

  /*! True if BSDI driver is available. */
  bool cdio_have_bsdi    (void);

  /*! True if FreeBSD driver is available. */
  bool cdio_have_freebsd (void);

  /*! True if GNU/Linux driver is available. */
  bool cdio_have_linux   (void);

  /*! True if Sun Solaris driver is available. */
  bool cdio_have_solaris (void);

  /*! True if Apple OSX driver is available. */
  bool cdio_have_osx     (void);

  /*! True if Microsoft Windows driver is available. */
  bool cdio_have_win32   (void);

  /*! True if Nero driver is available. */
  bool cdio_have_nrg     (void);

  /*! True if BIN/CUE driver is available. */
  bool cdio_have_bincue  (void);

  /*! True if cdrdao CDRDAO driver is available. */
  bool cdio_have_cdrdao  (void);

  /*! Like cdio_have_xxx but uses an enumeration instead. */
  bool cdio_have_driver (driver_id_t driver_id);
  
  /*!
    Free any resources associated with p_cdio. Call this when done using p_cdio
    and using CD reading/control operations.

    @param p_cdio the CD object to eliminated.
   */
  void cdio_destroy (CdIo_t *p_cdio);

  /*! 
    Get a string decribing driver_id. 

    @param driver_id the driver you want the description for
    @return a sring of driver description
  */
  const char *cdio_driver_describe (driver_id_t driver_id);
  
  /*! Sets up to read from place specified by psz_source and
     driver_id. This or cdio_open_* should be called before using any
     other routine, except cdio_init. This will call cdio_init, if
     that hasn't been done previously.  to call one of the specific
     cdio_open_xxx routines.

     @return the cdio object or NULL on error or no device.
  */
  CdIo_t * cdio_open (const char *psz_source, driver_id_t driver_id);

  /*! Sets up to read from place specified by psz_source, driver_id
     and access mode. This or cdio_open should be called before using
     any other routine, except cdio_init. This will call cdio_init, if
     that hasn't been done previously.  to call one of the specific
     cdio_open_xxx routines.

     @return the cdio object or NULL on error or no device.
  */
  CdIo_t * cdio_open_am (const char *psz_source, 
			 driver_id_t driver_id, const char *psz_access_mode);

  /*! Set up BIN/CUE CD disk-image for reading. Source is the .bin or 
      .cue file

     @return the cdio object or NULL on error or no device.
   */
  CdIo_t * cdio_open_bincue (const char *psz_cue_name);
  
  /*! Set up BIN/CUE CD disk-image for reading. Source is the .bin or 
      .cue file

     @return the cdio object or NULL on error or no device..
   */
  CdIo_t * cdio_open_am_bincue (const char *psz_cue_name, 
				const char *psz_access_mode);
  
  /*! Set up cdrdao CD disk-image for reading. Source is the .toc file

     @return the cdio object or NULL on error or no device.
   */
  CdIo_t * cdio_open_cdrdao (const char *psz_toc_name);
  
  /*! Set up cdrdao CD disk-image for reading. Source is the .toc file

     @return the cdio object or NULL on error or no device..
   */
  CdIo_t * cdio_open_am_cdrdao (const char *psz_toc_name, 
				const char *psz_access_mode);
  
  /*! Return a string containing the default CUE file that would
      be used when none is specified.

     @return the cdio object or NULL on error or no device.
   */
  char * cdio_get_default_device_bincue(void);

  char **cdio_get_devices_bincue(void);

  /*! Return a string containing the default CUE file that would
      be used when none is specified.

     NULL is returned on error or there is no device.
   */
  char * cdio_get_default_device_cdrdao(void);

  char **cdio_get_devices_cdrdao(void);

  /*! Set up CD-ROM for reading. The device_name is
      the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no driver for a some sort of hardware CD-ROM.
   */
  CdIo_t * cdio_open_cd (const char *device_name);

  /*! Set up CD-ROM for reading. The device_name is
      the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no driver for a some sort of hardware CD-ROM.
   */
  CdIo_t * cdio_open_am_cd (const char *psz_device,
			    const char *psz_access_mode);

  /*! CDRWIN BIN/CUE CD disc-image routines. Source is the .cue file

     @return the cdio object for subsequent operations. 
     NULL on error.
   */
  CdIo_t * cdio_open_cue (const char *cue_name);

  /*! Set up CD-ROM for reading using the AIX driver. The device_name is
      the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no BSDI driver.

     @see cdio_open
   */
  CdIo_t * cdio_open_am_aix (const char *psz_source,
			     const char *psz_access_mode);
  
  /*! Set up CD-ROM for reading using the AIX driver. The device_name is
      the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no BSDI driver.

     @see cdio_open
   */
  CdIo_t * cdio_open_aix (const char *psz_source);
  
  /*! Return a string containing the default device name that the 
      BSDI driver would use when none is specified.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no AIX driver.

     @see cdio_open_cd, cdio_open
   */
  char * cdio_get_default_device_aix(void);

  /*! Return a list of all of the CD-ROM devices that the AIX driver
      can find.

      In some situations of drivers or OS's we can't find a CD device if
      there is no media in it and it is possible for this routine to return
      NULL even though there may be a hardware CD-ROM.
   */
  char **cdio_get_devices_aix(void);
  
  /*! Set up CD-ROM for reading using the BSDI driver. The device_name is
      the some sort of device name.

     @param psz_source the name of the device to open
     @return the cdio object for subsequent operations. 
     NULL on error or there is no BSDI driver.

     @see cdio_open
   */
  CdIo_t * cdio_open_bsdi (const char *psz_source);
  
  /*! Set up CD-ROM for reading using the BSDI driver. The device_name is
      the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no BSDI driver.

     @see cdio_open
   */
  CdIo_t * cdio_open_am_bsdi (const char *psz_source,
			      const char *psz_access_mode);
  
  /*! Return a string containing the default device name that the 
      BSDI driver would use when none is specified.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no BSDI driver.

     @see cdio_open_cd, cdio_open
   */
  char * cdio_get_default_device_bsdi(void);

  /*! Return a list of all of the CD-ROM devices that the BSDI driver
      can find.

      In some situations of drivers or OS's we can't find a CD device if
      there is no media in it and it is possible for this routine to return
      NULL even though there may be a hardware CD-ROM.
   */
  char **cdio_get_devices_bsdi(void);
  
  /*! Set up CD-ROM for reading using the FreeBSD driver. The device_name is
      the some sort of device name.

     NULL is returned on error or there is no FreeBSD driver.

     @see cdio_open_cd, cdio_open
   */
  CdIo_t * cdio_open_freebsd (const char *paz_psz_source);
  
  /*! Set up CD-ROM for reading using the FreeBSD driver. The device_name is
      the some sort of device name.

     NULL is returned on error or there is no FreeBSD driver.

     @see cdio_open_cd, cdio_open
   */
  CdIo_t * cdio_open_am_freebsd (const char *psz_source,
				 const char *psz_access_mode);
  
  /*! Return a string containing the default device name that the 
      FreeBSD driver would use when none is specified.

     NULL is returned on error or there is no CD-ROM device.
   */
  char * cdio_get_default_device_freebsd(void);

  /*! Return a list of all of the CD-ROM devices that the FreeBSD driver
      can find.
   */
  char **cdio_get_devices_freebsd(void);
  
  /*! Set up CD-ROM for reading using the GNU/Linux driver. The device_name is
      the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no GNU/Linux driver.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.
   */
  CdIo_t * cdio_open_linux (const char *psz_source);

  /*! Set up CD-ROM for reading using the GNU/Linux driver. The
      device_name is the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no GNU/Linux driver.
   */
  CdIo_t * cdio_open_am_linux (const char *psz_source,
			       const char *access_mode);

  /*! Return a string containing the default device name that the 
      GNU/Linux driver would use when none is specified. A scan is made
      for CD-ROM drives with CDs in them.

     NULL is returned on error or there is no CD-ROM device.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.

     @see cdio_open_cd, cdio_open
   */
  char * cdio_get_default_device_linux(void);

  /*! Return a list of all of the CD-ROM devices that the GNU/Linux driver
      can find.
   */
  char **cdio_get_devices_linux(void);
  
  /*! Set up CD-ROM for reading using the Sun Solaris driver. The
      device_name is the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no Solaris driver.
   */
  CdIo_t * cdio_open_solaris (const char *psz_source);
  
  /*! Set up CD-ROM for reading using the Sun Solaris driver. The
      device_name is the some sort of device name.

     @return the cdio object for subsequent operations. 
     NULL on error or there is no Solaris driver.
   */
  CdIo_t * cdio_open_am_solaris (const char *psz_source, 
				 const char *psz_access_mode);
  
  /*! Return a string containing the default device name that the 
      Solaris driver would use when none is specified. A scan is made
      for CD-ROM drives with CDs in them.

     NULL is returned on error or there is no CD-ROM device.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.

     @see cdio_open_cd, cdio_open
   */
  char * cdio_get_default_device_solaris(void);
  
  /*! Return a list of all of the CD-ROM devices that the Solaris driver
      can find.
   */
  char **cdio_get_devices_solaris(void);
  
  /*! Set up CD-ROM for reading using the Apple OSX driver. The
      device_name is the some sort of device name.

     NULL is returned on error or there is no OSX driver.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.

     @see cdio_open_cd, cdio_open
   */
  CdIo_t * cdio_open_osx (const char *psz_source);

  /*! Set up CD-ROM for reading using the Apple OSX driver. The
      device_name is the some sort of device name.

     NULL is returned on error or there is no OSX driver.

     @see cdio_open_cd, cdio_open
   */
  CdIo_t * cdio_open_am_osx (const char *psz_source,
			     const char *psz_access_mode);

  /*! Return a string containing the default device name that the 
      OSX driver would use when none is specified. A scan is made
      for CD-ROM drives with CDs in them.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.
   */
  char * cdio_get_default_device_osx(void);
  
  /*! Return a list of all of the CD-ROM devices that the OSX driver
      can find.
   */
  char **cdio_get_devices_osx(void);
  
  /*! Set up CD-ROM for reading using the Microsoft Windows driver. The
      device_name is the some sort of device name.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.
   */
  CdIo_t * cdio_open_win32 (const char *psz_source);
  
  /*! Set up CD-ROM for reading using the Microsoft Windows driver. The
      device_name is the some sort of device name.

     NULL is returned on error or there is no Microsof Windows driver.
   */
  CdIo_t * cdio_open_am_win32 (const char *psz_source,
			       const char *psz_access_mode);
  
  /*! Return a string containing the default device name that the 
      Win32 driver would use when none is specified. A scan is made
      for CD-ROM drives with CDs in them.

     In some situations of drivers or OS's we can't find a CD device if
     there is no media in it and it is possible for this routine to return
     NULL even though there may be a hardware CD-ROM.

     @see cdio_open_cd, cdio_open
   */
  char * cdio_get_default_device_win32(void);

  char **cdio_get_devices_win32(void);
  
  /*! Set up CD-ROM for reading using the Nero driver. The
      device_name is the some sort of device name.

     @return true on success; NULL on error or there is no Nero driver. 
   */
  CdIo_t * cdio_open_nrg (const char *psz_source);
  
  /*! Set up CD-ROM for reading using the Nero driver. The
      device_name is the some sort of device name.

     @return true on success; NULL on error or there is no Nero driver. 
   */
  CdIo_t * cdio_open_am_nrg (const char *psz_source,
			     const char *psz_access_mode);
  
  /*! Get a string containing the default device name that the NRG
      driver would use when none is specified. A scan is made for NRG
      disk images in the current directory.

      @return string containing the default device. NULL on error or
      there is no CD-ROM device.
   */
  char * cdio_get_default_device_nrg(void);

  char **cdio_get_devices_nrg(void);

  /*! 

    Determine if bin_name is the bin file part of  a CDRWIN CD disk image.

    @param bin_name location of presumed CDRWIN bin image file.
    @return the corresponding CUE file if bin_name is a BIN file or
    NULL if not a BIN file.
  */
  char *cdio_is_binfile(const char *bin_name);
  
  /*! 
    Determine if cue_name is the cue sheet for a CDRWIN CD disk image.

    @return corresponding BIN file if cue_name is a CDRWIN cue file or
    NULL if not a CUE file.
  */
  char *cdio_is_cuefile(const char *cue_name);
  
  /*! 
    Determine if psg_nrg is a Nero CD disk image.

    @param psz_nrg location of presumed NRG image file.
    @return true if psz_nrg is a Nero NRG image or false
    if not a NRG image.
  */
  bool cdio_is_nrg(const char *psz_nrg);
  
  /*! 
    Determine if psg_toc is a TOC file for a cdrdao CD disk image.

    @param psz_toc location of presumed TOC image file.
    @return true if toc_name is a cdrdao TOC file or false
    if not a TOC file.
  */
  bool cdio_is_tocfile(const char *psz_toc);
  
  /*! 
    Determine if psz_source refers to a real hardware CD-ROM.

    @param psz_source location name of object
    @param driver_id   driver for reading object. Use DRIVER_UNKNOWN if you
    don't know what driver to use.
    @return true if psz_source is a device; If false is returned we
    could have a CD disk image. 
  */
  bool cdio_is_device(const char *psz_source, driver_id_t driver_id);

  /*!
    Set the blocksize for subsequent reads. 
  */
  driver_return_code_t cdio_set_blocksize ( const CdIo_t *p_cdio, 
					    int i_blocksize );

  /*!
    Set the drive speed. 

    @see cdio_get_speed
  */
  driver_return_code_t cdio_set_speed ( const CdIo_t *p_cdio, int i_speed );

  /*!
    Get the value associatied with key. 

    @param p_cdio the CD object queried
    @param key the key to retrieve
    @return the value associatd with "key" or NULL if p_cdio is NULL
    or "key" does not exist.
  */
  const char * cdio_get_arg (const CdIo_t *p_cdio,  const char key[]);

  /*!
    Set the arg "key" with "value" in "p_cdio".

    @param p_cdio the CD object to set
    @param key the key to set
    @param value the value to assocaiate with key
  */
  driver_return_code_t cdio_set_arg (CdIo_t *p_cdio, const char key[], 
				     const char value[]);
  
  /*!
    Initialize CD Reading and control routines. Should be called first.
  */
  bool cdio_init(void);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_TRACK_H__ */
