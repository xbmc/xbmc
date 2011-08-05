/* 
 *  Copyright (C) 2006-2009 mplayerc
 *  Copyright (C) 2010 Team XBMC
 *  http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef AVCODEC_DIRECTSHOW_INTERNAL_H
#define AVCODEC_DIRECTSHOW_INTERNAL_H



#include <stdint.h>
/* able to only use the dxva2api for compiling the dll with mingw */
#include "avcodec.h"
#include "mpegvideo.h"

//int ff_decode_slice_header_noexecute(AVCodecContext *avctx);
//void ff_field_end_noexecute(AVCodecContext *avctx);
void ff_directshow_h264_fill_slice_long(MpegEncContext *s);
void ff_directshow_h264_picture_start(MpegEncContext *s);
void ff_directshow_h264_set_reference_frames(MpegEncContext *s);
void ff_directshow_h264_picture_complete(MpegEncContext *s);
void ff_directshow_h264_setpoc(MpegEncContext *s, int poc, int64_t start);


#endif
