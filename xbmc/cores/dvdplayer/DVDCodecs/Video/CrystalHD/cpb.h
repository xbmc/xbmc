/*
 * Copyright (C) 2009 Julian Scheel
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * cpb.h: Coded Picture Buffer
 */

#ifndef CPB_H_
#define CPB_H_

#include "nal.h"

enum picture_flags {
  IDR_PIC = 0x01,
  REFERENCE = 0x02,
  NOT_EXISTING = 0x04,
  INTERLACED = 0x08
};

struct coded_picture
{
  uint32_t flag_mask;

  uint32_t max_pic_num;
  int32_t pic_num;

  uint8_t used_for_long_term_ref;
  uint32_t long_term_pic_num;
  uint32_t long_term_frame_idx;

  int32_t top_field_order_cnt;
  int32_t bottom_field_order_cnt;

  uint8_t repeat_pic;

  /* buffer data for the image slices, which
   * are passed to the decoder
   */
  uint32_t slice_cnt;

  int64_t pts;

  struct nal_unit *sei_nal;
  struct nal_unit *sps_nal;
  struct nal_unit *pps_nal;
  struct nal_unit *slc_nal;
};

struct coded_picture* create_coded_picture();
void free_coded_picture(struct coded_picture *pic);

#endif /* CPB_H_ */
