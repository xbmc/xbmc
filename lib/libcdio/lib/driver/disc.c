/*
    $Id: disc.c,v 1.4 2005/01/24 00:06:31 rocky Exp $

    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>

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


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include "cdio_private.h"

/* Must match discmode enumeration */
const char *discmode2str[] = {
  "CD-DA", 
  "CD-DATA (Mode 1)", 
  "CD DATA (Mode 2)", 
  "CD-ROM Mixed",
  "DVD-ROM", 
  "DVD-RAM", 
  "DVD-R", 
  "DVD-RW", 
  "DVD+R",
  "DVD+RW", 
  "Unknown/unclassified DVD", 
  "No information",
  "Error in getting information",
  "CD-i" 
};

/*!
  Get the size of the CD in logical block address (LBA) units.
  
  @param p_cdio the CD object queried
  @return the lsn. On error 0 or CDIO_INVALD_LSN.
*/
lsn_t 
cdio_get_disc_last_lsn(const CdIo_t *p_cdio)
{
  if (!p_cdio) return CDIO_INVALID_LSN;
  return p_cdio->op.get_disc_last_lsn (p_cdio->env);
}

/*! 
  Get medium associated with cd_obj.
*/
discmode_t
cdio_get_discmode (CdIo_t *cd_obj)
{
  if (!cd_obj) return CDIO_DISC_MODE_ERROR;
  
  if (cd_obj->op.get_discmode) {
    return cd_obj->op.get_discmode (cd_obj->env);
  } else {
    return CDIO_DISC_MODE_NO_INFO;
  }
}

/*!
  Return a string containing the name of the driver in use.
  if CdIo is NULL (we haven't initialized a specific device driver), 
  then return NULL.
*/
char *
cdio_get_mcn (const CdIo_t *p_cdio) 
{
  if (p_cdio->op.get_mcn) {
    return p_cdio->op.get_mcn (p_cdio->env);
  } else {
    return NULL;
  }
}
