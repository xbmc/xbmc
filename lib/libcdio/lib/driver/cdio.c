/*
    $Id: cdio.c,v 1.12 2005/01/27 03:10:06 rocky Exp $

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

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "cdio_assert.h"
#include <cdio/cdio.h>
#include <cdio/util.h>
#include "cdio_private.h"

static const char _rcsid[] = "$Id: cdio.c,v 1.12 2005/01/27 03:10:06 rocky Exp $";


/*!
  Return the value associatied with key. NULL is returned if obj is NULL
  or "key" does not exist.
 */
const char *
cdio_get_arg (const CdIo *obj, const char key[])
{
  if (obj == NULL) return NULL;
  
  if (obj->op.get_arg) {
    return obj->op.get_arg (obj->env, key);
  } else {
    return NULL;
  }
}

/*! 
  Get cdtext information for a CdIo object .
  
  @param obj the CD object that may contain CD-TEXT information.
  @return the CD-TEXT object or NULL if obj is NULL
  or CD-TEXT information does not exist.
*/
cdtext_t *
cdio_get_cdtext (CdIo *obj, track_t i_track)
{
  if (obj == NULL) return NULL;
  
  if (obj->op.get_cdtext) {
    return obj->op.get_cdtext (obj->env, i_track);
  } else {
    return NULL;
  }
}

CdIo_t *
cdio_new (generic_img_private_t *p_env, cdio_funcs_t *p_funcs)
{
  CdIo_t *p_new_cdio = _cdio_malloc (sizeof (CdIo_t));

  if (NULL == p_new_cdio) return NULL;
  
  p_new_cdio->env = p_env;      /* This is the private "environment" that
                                   driver-dependent routines use. */
  p_new_cdio->op  = *p_funcs;
  p_env->cdio     = p_new_cdio; /* A way for the driver-dependent routines 
                                   to access the higher-level general cdio 
                                   object. */
  return p_new_cdio;
}

/*!
  Set the arg "key" with "value" in the source device.
*/
driver_return_code_t
cdio_set_arg (CdIo_t *p_cdio, const char key[], const char value[])
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (!p_cdio->op.set_arg) return DRIVER_OP_UNSUPPORTED;
  if (!key) return DRIVER_OP_ERROR;

  return p_cdio->op.set_arg (p_cdio->env, key, value);
}



/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
