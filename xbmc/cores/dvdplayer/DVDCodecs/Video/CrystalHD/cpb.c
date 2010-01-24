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
 * cpb.c: Coded Picture Buffer
 */

#include "cpb.h"

#include <stdlib.h>

struct coded_picture* create_coded_picture()
{
  struct coded_picture* pic = calloc(1, sizeof(struct coded_picture));
  return pic;
}

void free_coded_picture(struct coded_picture *pic)
{
  if(!pic)
    return;

  release_nal_unit(pic->sei_nal);
  release_nal_unit(pic->sps_nal);
  release_nal_unit(pic->pps_nal);
  release_nal_unit(pic->slc_nal);

  free(pic);
}

