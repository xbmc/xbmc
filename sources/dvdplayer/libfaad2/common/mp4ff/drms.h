/*****************************************************************************
 * drms.h : DRMS
 *****************************************************************************
 * Copyright (C) 2004 VideoLAN
 * $Id: drms.h,v 1.3 2004/01/11 15:52:18 menno Exp $
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


#define DRMS_INIT_UKEY  0
#define DRMS_INIT_IVIV  1
#define DRMS_INIT_NAME  2
#define DRMS_INIT_PRIV  3

extern int drms_get_sys_key( uint32_t *p_sys_key );
extern int drms_get_user_key( uint32_t *p_sys_key,
                              uint32_t *p_user_key );

extern void *drms_alloc();
extern void drms_free( void *p_drms );
extern int drms_init( void *p_drms, uint32_t i_type,
                      uint8_t *p_info, uint32_t i_len );
extern void drms_decrypt( void *p_drms, uint32_t *p_buffer,
                          uint32_t i_len );

