/*
    $Id: dvd.h,v 1.4 2004/09/04 23:49:47 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
    Modeled after GNU/Linux definitions in linux/cdrom.h

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

/*!
   \file dvd.h 
   \brief Definitions for DVD access.
*/

#ifndef __CDIO_DVD_H__
#define __CDIO_DVD_H__

#include <cdio/types.h>

/*! Values used in a READ DVD STRUCTURE */

#define CDIO_DVD_STRUCT_PHYSICAL	0x00
#define CDIO_DVD_STRUCT_COPYRIGHT	0x01
#define CDIO_DVD_STRUCT_DISCKEY	        0x02
#define CDIO_DVD_STRUCT_BCA		0x03
#define CDIO_DVD_STRUCT_MANUFACT	0x04

/*! Media definitions for "Book Type" */
#define CDIO_DVD_BOOK_DVD_ROM 0
#define CDIO_DVD_BOOK_DVD_RAM 1
#define CDIO_DVD_BOOK_DVD_R   2 /**< DVD-R  */
#define CDIO_DVD_BOOK_DVD_RW  3 /**< DVD-RW */
#define CDIO_DVD_BOOK_DVD_PR  8 /**< DVD+R  */
#define CDIO_DVD_BOOK_DVD_PRW 9 /**< DVD+RW */

typedef struct cdio_dvd_layer {
  uint8_t book_version	: 4;
  uint8_t book_type	: 4;
  uint8_t min_rate	: 4;
  uint8_t disc_size	: 4;
  uint8_t layer_type	: 4;
  uint8_t track_path	: 1;
  uint8_t nlayers	: 2;
  uint8_t track_density	: 4;
  uint8_t linear_density: 4;
  uint8_t bca		: 1;
  uint32_t start_sector;
  uint32_t end_sector;
  uint32_t end_sector_l0;
} cdio_dvd_layer_t;

/*! Maximum number of layers in a DVD.  */
#define CDIO_DVD_MAX_LAYERS	4

typedef struct cdio_dvd_physical {
  uint8_t type;
  uint8_t layer_num;
  cdio_dvd_layer_t layer[CDIO_DVD_MAX_LAYERS];
} cdio_dvd_physical_t;

typedef struct cdio_dvd_copyright {
  uint8_t type;
  
  uint8_t layer_num;
  uint8_t cpst;
  uint8_t rmi;
} cdio_dvd_copyright_t;

typedef struct cdio_dvd_disckey {
  uint8_t type;
  
  unsigned agid	: 2;
  uint8_t value[2048];
} cdio_dvd_disckey_t;

typedef struct cdio_dvd_bca {
  uint8_t type;
  
  int len;
  uint8_t value[188];
} cdio_dvd_bca_t;

typedef struct cdio_dvd_manufact {
  uint8_t type;
  
  uint8_t layer_num;
  int len;
  uint8_t value[2048];
} cdio_dvd_manufact_t;

typedef union {
  uint8_t type;
  
  cdio_dvd_physical_t	physical;
  cdio_dvd_copyright_t	copyright;
  cdio_dvd_disckey_t	disckey;
  cdio_dvd_bca_t	bca;
  cdio_dvd_manufact_t	manufact;
} cdio_dvd_struct_t;

#endif /* __SCSI_MMC_H__ */
