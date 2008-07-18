/*****************************************************************************
 * drms.h : DRMS
 *****************************************************************************
 * Copyright (C) 2004 VideoLAN
 * $Id: drms.h,v 1.7 2005/02/01 13:15:55 menno Exp $
 *
 * Author: Jon Lech Johansen <jon-vl@nanocrew.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

extern void *drms_alloc( char *psz_homedir );
extern void drms_free( void *p_drms );
extern int drms_init( void *p_drms, uint32_t i_type,
                      uint8_t *p_info, uint32_t i_len );
extern void drms_decrypt( void *p_drms, uint32_t *p_buffer,
                          uint32_t i_len );

