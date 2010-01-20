/* -*- C++ -*-
    $Id: read.hpp,v 1.2 2006/01/18 21:31:37 rocky Exp $

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

/** \file read.hpp
 *
 *  \brief methods relating to reading blocks of Compact Discs. This file
 *  should not be #included directly.
 */

/*!
  Reposition read offset
  Similar to (if not the same as) libc's lseek()
  
  @param offset amount to seek
  @param whence  like corresponding parameter in libc's lseek, e.g. 
  SEEK_SET or SEEK_END.
  @return (off_t) -1 on error. 
*/

off_t lseek(off_t offset, int whence) 
{
  return cdio_lseek(p_cdio, offset, whence);
}

/*!
  Reads into buf the next size bytes.
  Similar to (if not the same as) libc's read()
  
  @param p_buf place to read data into. The caller should make sure
  this location can store at least i_size bytes.
  @param i_size number of bytes to read
  
  @return (ssize_t) -1 on error. 
*/
ssize_t read(void *p_buf, size_t i_size) 
{
  return cdio_read(p_cdio, p_buf, i_size);
}

/*!
  Reads a number of sectors (AKA blocks).
  
  @param p_buf place to read data into. The caller should make sure
  this location is large enough. See below for size information.
  @param read_mode the kind of "mode" to use in reading.
  @param i_lsn sector to read
  @param i_blocks number of sectors to read

  If read_mode is CDIO_MODE_AUDIO,
    *p_buf should hold at least CDIO_FRAMESIZE_RAW * i_blocks bytes.

  If read_mode is CDIO_MODE_DATA,
    *p_buf should hold at least i_blocks times either ISO_BLOCKSIZE, 
    M1RAW_SECTOR_SIZE or M2F2_SECTOR_SIZE depending on the kind of 
    sector getting read. If you don't know whether you have a Mode 1/2, 
    Form 1/ Form 2/Formless sector best to reserve space for the maximum
    which is M2RAW_SECTOR_SIZE.

  If read_mode is CDIO_MODE_M2F1,
    *p_buf should hold at least M2RAW_SECTOR_SIZE * i_blocks bytes.

  If read_mode is CDIO_MODE_M2F2,
    *p_buf should hold at least CDIO_CD_FRAMESIZE * i_blocks bytes.

  A DriverOpException is raised on error.
*/

void readSectors(void *p_buf, lsn_t i_lsn, cdio_read_mode_t read_mode, 
		 uint32_t i_blocks=1)
{
  driver_return_code_t drc = cdio_read_sectors(p_cdio, p_buf, i_lsn, read_mode,
					       i_blocks);
  possible_throw_device_exception(drc);
}

/*!
  Reads a number of data sectors (AKA blocks).
  
  @param p_buf place to read data into. The caller should make sure
  this location is large enough. See below for size information.

  *p_buf should hold at least i_blocks times either ISO_BLOCKSIZE, 
  M1RAW_SECTOR_SIZE or M2F2_SECTOR_SIZE depending on the kind of 
  sector getting read. If you don't know whether you have a Mode 1/2, 
  Form 1/ Form 2/Formless sector best to reserve space for the maximum
  which is M2RAW_SECTOR_SIZE.

  @param i_lsn sector to read

  @param i_blocksize size of block. Should be either CDIO_CD_FRAMESIZE, 
  M2RAW_SECTOR_SIZE, or M2F2_SECTOR_SIZE. See comment above under p_buf.

  @param i_blocks number of sectors to read

  A DriverOpException is raised on error.
  
*/

void readDataBlocks(void *p_buf, lsn_t i_lsn, uint16_t i_blocksize, 
		    uint32_t i_blocks=1) 
{
  driver_return_code_t drc = cdio_read_data_sectors (p_cdio, p_buf, i_lsn, 
						     i_blocksize, i_blocks);
  possible_throw_device_exception(drc);
}
