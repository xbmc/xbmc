/* -*- C++ -*-
    $Id: device.hpp,v 1.6 2006/03/05 06:52:15 rocky Exp $

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

/** \file device.hpp
 *
 *  \brief C++ header for driver- or device-related libcdio calls.  
 *         ("device" includes CD-image reading devices.)
 */

/*! 
  Free resources associated with CD-ROM Device/Image. After this we 
  must do another open before any more reading.
*/
bool 
close()
{
  cdio_destroy(p_cdio);
  p_cdio = (CdIo_t *) NULL;
  return true;
}

/*!
  Eject media in CD drive if there is a routine to do so. 
  
  If the CD is ejected, object is destroyed.
*/
void
ejectMedia () 
{
  driver_return_code_t drc = cdio_eject_media(&p_cdio);
  possible_throw_device_exception(drc);
}

/*!
  Free device list returned by GetDevices
  
  @param device_list list returned by GetDevices
  
  @see GetDevices
  
*/
void 
freeDeviceList (char * device_list[]) 
{
  cdio_free_device_list(device_list);
}

/*!
    Get the value associatied with key. 
    
    @param key the key to retrieve
    @return the value associatd with "key" or NULL if p_cdio is NULL
    or "key" does not exist.
  */
const char * 
getArg (const char key[]) 
{
  return cdio_get_arg (p_cdio, key);
}

/*!  
  Return an opaque CdIo_t pointer for the given track object.
*/
CdIo_t *getCdIo()
{
  return p_cdio;
}

/*!  
  Return an opaque CdIo_t pointer for the given track object.
*/
cdtext_t *getCdtext(track_t i_track)
{
  return cdio_get_cdtext (p_cdio, i_track);
}

/*!
  Get the CD device name for the object.
  
  @return a string containing the CD device for this object or NULL is
  if we couldn't get a device anme.
  
  In some situations of drivers or OS's we can't find a CD device if
  there is no media in it and it is possible for this routine to return
  NULL even though there may be a hardware CD-ROM.
*/
char *
getDevice () 
{
  return cdio_get_default_device(p_cdio);
}

/*!
  Get the what kind of device we've got.
  
  @param p_read_cap pointer to return read capabilities
  @param p_write_cap pointer to return write capabilities
  @param p_misc_cap pointer to return miscellaneous other capabilities
  
  In some situations of drivers or OS's we can't find a CD device if
  there is no media in it and it is possible for this routine to return
  NULL even though there may be a hardware CD-ROM.
*/
void 
getDriveCap (cdio_drive_read_cap_t  &read_cap,
	     cdio_drive_write_cap_t &write_cap,
	     cdio_drive_misc_cap_t  &misc_cap) 
{
  cdio_get_drive_cap(p_cdio, &read_cap, &write_cap, &misc_cap);
}

/*!
  Get a string containing the name of the driver in use.
  
  @return a string with driver name or NULL if CdIo_t is NULL (we
  haven't initialized a specific device.
*/
const char *
getDriverName () 
{
  return cdio_get_driver_name(p_cdio);
}

/*!
  Get the driver id. 
  if CdIo_t is NULL (we haven't initialized a specific device driver), 
  then return DRIVER_UNKNOWN.
  
  @return the driver id..
*/
driver_id_t 
getDriverId () 
{
  return cdio_get_driver_id(p_cdio);
}

/*! 
  Get the CD-ROM hardware info via a SCSI MMC INQUIRY command.
  False is returned if we had an error getting the information.
*/
bool 
getHWinfo ( /*out*/ cdio_hwinfo_t &hw_info ) 
{
  return cdio_get_hwinfo(p_cdio, &hw_info);
}

/*! Get the LSN of the first track of the last session of
  on the CD.
  
  @param i_last_session pointer to the session number to be returned.
*/
void
getLastSession (/*out*/ lsn_t &i_last_session) 
{
  driver_return_code_t drc = cdio_get_last_session(p_cdio, &i_last_session);
  possible_throw_device_exception(drc);
}

/*! 
  Find out if media has changed since the last call.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
*/
int 
getMediaChanged() 
{
  return cdio_get_media_changed(p_cdio);
}

/*! True if CD-ROM understand ATAPI commands. */
bool_3way_t 
haveATAPI ()
{
  return cdio_have_atapi(p_cdio);
}

/*! 

  Sets up to read from the device specified by psz_source.  An open
  routine should be called before using any read routine. If device
  object was previously opened it is closed first.
  
  @return true if open succeeded or false if error.

*/
bool 
open(const char *psz_source)
{
  if (p_cdio) cdio_destroy(p_cdio);
  p_cdio = cdio_open_cd(psz_source);
  return NULL != p_cdio ;
}

/*! 

  Sets up to read from the device specified by psz_source and access
  mode.  An open routine should be called before using any read
  routine. If device object was previously opened it is "closed".
  
  @return true if open succeeded or false if error.
*/
bool 
open (const char *psz_source, driver_id_t driver_id, 
      const char *psz_access_mode = (const char *) NULL) 
{
  if (p_cdio) cdio_destroy(p_cdio);
  if (psz_access_mode)
    p_cdio = cdio_open_am(psz_source, driver_id, psz_access_mode);
  else 
    p_cdio = cdio_open(psz_source, driver_id);
  return NULL != p_cdio ;
}

/*!
  Set the blocksize for subsequent reads. 
*/
void
setBlocksize ( int i_blocksize )
{
  driver_return_code_t drc = cdio_set_blocksize ( p_cdio, i_blocksize );
  possible_throw_device_exception(drc);
}

/*!
    Set the drive speed. 
*/
void
setSpeed ( int i_speed ) 
{
  driver_return_code_t drc = cdio_set_speed ( p_cdio, i_speed );
  possible_throw_device_exception(drc);
}

/*!
    Set the arg "key" with "value" in "p_cdio".
    
    @param key the key to set
    @param value the value to assocaiate with key
*/
void
setArg (const char key[], const char value[]) 
{
  driver_return_code_t drc = cdio_set_arg (p_cdio, key, value);
  possible_throw_device_exception(drc);
}
