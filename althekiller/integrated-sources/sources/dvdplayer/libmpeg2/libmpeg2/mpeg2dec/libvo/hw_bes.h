/*
 * hw_bes.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_HW_BES_H
#define __LINUX_HW_BES_H

typedef struct {
    uint32_t card_type;
    uint32_t ram_size;
    uint32_t src_width;
    uint32_t src_height;
    uint32_t dest_width;
    uint32_t dest_height;
    uint32_t x_org;
    uint32_t y_org;
    uint8_t  colkey_on;
    uint8_t  colkey_red;
    uint8_t  colkey_green;
    uint8_t  colkey_blue;
} mga_vid_config_t;

#define MGA_VID_CONFIG    _IOR('J', 1, mga_vid_config_t)
#define MGA_VID_ON        _IO ('J', 2)
#define MGA_VID_OFF       _IO ('J', 3)
#define MGA_VID_FSEL _IOR('J', 4, int)

#define MGA_G200 0x1234
#define MGA_G400 0x5678

#endif
