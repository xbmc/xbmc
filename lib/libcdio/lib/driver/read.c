/*
    $Id: read.c,v 1.3 2005/01/23 19:16:58 rocky Exp $

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
/** \file read.h 
 *
 * \brief sector (block, frame)-related libcdio routines.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/logging.h>
#include "cdio_private.h"
#include "cdio_assert.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define check_read_parms(p_cdio, p_buf, i_lsn)                          \
  if (!p_cdio) return DRIVER_OP_UNINIT;                                 \
  if (!p_buf || CDIO_INVALID_LSN == i_lsn)                              \
    return DRIVER_OP_ERROR; 

#define check_lsn(i_lsn)                                                \
  check_read_parms(p_cdio, p_buf, i_lsn);                               \
  {                                                                     \
    lsn_t end_lsn =                                                     \
      cdio_get_track_lsn(p_cdio, CDIO_CDROM_LEADOUT_TRACK);             \
    if ( i_lsn > end_lsn ) {                                            \
      cdio_info("Trying to access past end of disk lsn: %ld, end lsn: %ld", \
                (long int) i_lsn, (long int) end_lsn);                  \
      return DRIVER_OP_ERROR;                                           \
    }                                                                   \
  }

#define check_lsn_blocks(i_lsn, i_blocks)                               \
  check_read_parms(p_cdio, p_buf, i_lsn);                               \
  {                                                                     \
    lsn_t end_lsn =                                                     \
      cdio_get_track_lsn(p_cdio, CDIO_CDROM_LEADOUT_TRACK);             \
    if ( i_lsn > end_lsn ) {                                            \
      cdio_info("Trying to access past end of disk lsn: %ld, end lsn: %ld", \
                (long int) i_lsn, (long int) end_lsn);                  \
      return DRIVER_OP_ERROR;                                           \
    }                                                                   \
    if ( i_lsn + i_blocks -1 > end_lsn ) {                              \
      cdio_info("Request truncated to end disk; lsn: %ld, end lsn: %ld", \
                (long int) i_lsn, (long int) end_lsn);                  \
      i_blocks = end_lsn - i_lsn + 1;                                   \
    }                                                                   \
  }



/*!
  lseek - reposition read/write file offset
  Returns (off_t) -1 on error. 
  Similar to (if not the same as) libc's lseek()
*/
off_t
cdio_lseek (const CdIo_t *p_cdio, off_t offset, int whence)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  
  if (p_cdio->op.lseek)
    return p_cdio->op.lseek (p_cdio->env, offset, whence);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Similar to (if not the same as) libc's read()
*/
ssize_t
cdio_read (const CdIo_t *p_cdio, void *p_buf, size_t size)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  
  if (p_cdio->op.read)
    return p_cdio->op.read (p_cdio->env, p_buf, size);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Reads an audio sector from cd device into data starting
  from lsn. Returns 0 if no error. 
*/
driver_return_code_t
cdio_read_audio_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn) 
{
  check_lsn(i_lsn);
  if  (p_cdio->op.read_audio_sectors != NULL)
    return p_cdio->op.read_audio_sectors (p_cdio->env, p_buf, i_lsn, 1);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Reads audio sectors from cd device into data starting
  from lsn. Returns 0 if no error. 
*/
driver_return_code_t
cdio_read_audio_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn,
                         unsigned int i_blocks) 
{
  check_lsn_blocks(i_lsn, i_blocks);
  if (p_cdio->op.read_audio_sectors != NULL)
    return p_cdio->op.read_audio_sectors (p_cdio->env, p_buf, i_lsn, i_blocks);
  return DRIVER_OP_UNSUPPORTED;
}

#ifndef SEEK_SET
#define SEEK_SET 0
#endif 

/*!
   Reads a single mode1 form1 or form2  sector from cd device 
   into data starting from lsn. Returns 0 if no error. 
 */
driver_return_code_t
cdio_read_mode1_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                        bool b_form2)
{
  uint32_t size = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE ;

  check_lsn(i_lsn);
  if (p_cdio->op.read_mode1_sector) {
    return p_cdio->op.read_mode1_sector(p_cdio->env, p_buf, i_lsn, b_form2);
  } else if (p_cdio->op.lseek && p_cdio->op.read) {
    char buf[CDIO_CD_FRAMESIZE] = { 0, };
    if (0 > cdio_lseek(p_cdio, CDIO_CD_FRAMESIZE*i_lsn, SEEK_SET))
      return -1;
    if (0 > cdio_read(p_cdio, buf, CDIO_CD_FRAMESIZE))
      return -1;
    memcpy (p_buf, buf, size);
    return DRIVER_OP_SUCCESS;
  } 

  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Reads mode 1 sectors
  
  @param p_cdio object to read from
  @param buf place to read data into
  @param lsn sector to read
  @param b_form2 true for reading mode 1 form 2 sectors or false for 
  mode 1 form 1 sectors.
  @param i_sectors number of sectors to read
*/
driver_return_code_t
cdio_read_mode1_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                         bool b_form2,  unsigned int i_blocks)
{
  check_lsn_blocks(i_lsn, i_blocks);
  if (p_cdio->op.read_mode1_sectors)
    return p_cdio->op.read_mode1_sectors (p_cdio->env, p_buf, i_lsn, b_form2, 
                                          i_blocks);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Reads a mode 2 sector
  
  @param p_cdio object to read from
  @param buf place to read data into
  @param lsn sector to read
  @param b_form2 true for reading mode 2 form 2 sectors or false for 
  mode 2 form 1 sectors.
*/
driver_return_code_t
cdio_read_mode2_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                        bool b_form2)
{
  check_lsn(i_lsn);
  if (p_cdio->op.read_mode2_sector)
    return p_cdio->op.read_mode2_sector (p_cdio->env, p_buf, i_lsn, b_form2);

  /* fallback */
  if (p_cdio->op.read_mode2_sectors != NULL)
    return cdio_read_mode2_sectors (p_cdio, p_buf, i_lsn, b_form2, 1);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Reads mode 2 sectors
  
  @param p_cdio object to read from
  @param buf place to read data into
  @param lsn sector to read
  @param b_form2 true for reading mode2 form 2 sectors or false for 
  mode 2  form 1 sectors.
  @param i_sectors number of sectors to read
*/
driver_return_code_t
cdio_read_mode2_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                         bool b_form2, unsigned int i_blocks)
{
  check_lsn_blocks(i_lsn, i_blocks);
  if (p_cdio->op.read_mode2_sectors) 
    return p_cdio->op.read_mode2_sectors (p_cdio->env, p_buf, i_lsn,
                                          b_form2, i_blocks);
  return DRIVER_OP_UNSUPPORTED;
  
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
