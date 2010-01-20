/*
    $Id: audio.c,v 1.8 2005/03/14 02:02:49 rocky Exp $

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
/*! Audio (via line output) related routines. */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/util.h>
#include <cdio/audio.h>
#include "cdio_private.h"

/* Return the number of seconds (discarding frame portion) of an MSF */
uint32_t
cdio_audio_get_msf_seconds(msf_t *p_msf) 
{
  return 
    cdio_from_bcd8(p_msf->m)*CDIO_CD_SECS_PER_MIN + cdio_from_bcd8(p_msf->s);
}

/*!
  Get volume of an audio CD.
  
  @param p_cdio the CD object to be acted upon.
  
*/
driver_return_code_t 
cdio_audio_get_volume (CdIo_t *p_cdio,  /*out*/ cdio_audio_volume_t *p_volume)
{
  cdio_audio_volume_t temp_audio_volume;
  
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (!p_volume) p_volume = &temp_audio_volume;
  if (p_cdio->op.audio_get_volume) {
    return p_cdio->op.audio_get_volume (p_cdio->env, p_volume);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}
/*!
  Playing CD through analog output
  
  @param p_cdio the CD object to be acted upon.
*/
driver_return_code_t 
cdio_audio_pause (CdIo_t *p_cdio) 
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_pause) {
    return p_cdio->op.audio_pause (p_cdio->env);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Playing CD through analog output at the given MSF.
  
  @param p_cdio the CD object to be acted upon.
*/
driver_return_code_t 
cdio_audio_play_msf (CdIo_t *p_cdio, msf_t *p_start_msf, msf_t *p_end_msf)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_play_msf) {
    return p_cdio->op.audio_play_msf (p_cdio->env, p_start_msf, p_end_msf);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Playing CD through analog output
  
  @param p_cdio the CD object to be acted upon.
*/
driver_return_code_t 
cdio_audio_play_track_index (CdIo_t *p_cdio, cdio_track_index_t *p_track_index)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_play_track_index) {
    return p_cdio->op.audio_play_track_index (p_cdio->env, p_track_index);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Get subchannel information.
  
  @param p_cdio the CD object to be acted upon.
*/
driver_return_code_t 
cdio_audio_read_subchannel (CdIo_t *p_cdio, cdio_subchannel_t *p_subchannel)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_read_subchannel) {
    return p_cdio->op.audio_read_subchannel(p_cdio->env, p_subchannel);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Resume playing an audio CD.
  
  @param p_cdio the CD object to be acted upon.
  
*/
driver_return_code_t 
cdio_audio_resume (CdIo_t *p_cdio)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_resume) {
    return p_cdio->op.audio_resume(p_cdio->env);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Set volume of an audio CD.
  
  @param p_cdio the CD object to be acted upon.
  
*/
driver_return_code_t 
cdio_audio_set_volume (CdIo_t *p_cdio, cdio_audio_volume_t *p_volume)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_set_volume) {
    return p_cdio->op.audio_set_volume(p_cdio->env, p_volume);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}

/*!
  Resume playing an audio CD.
  
  @param p_cdio the CD object to be acted upon.
  
*/
driver_return_code_t 
cdio_audio_stop (CdIo_t *p_cdio)
{
  if (!p_cdio) return DRIVER_OP_UNINIT;

  if (p_cdio->op.audio_stop) {
    return p_cdio->op.audio_stop(p_cdio->env);
  } else {
    return DRIVER_OP_UNSUPPORTED;
  }
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
