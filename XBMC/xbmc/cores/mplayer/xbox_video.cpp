/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg and xvid
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <xtl.h>
#include "xbox_video.h"
#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#pragma warning (disable:4018)
#pragma warning (disable:4244)
static vo_info_t video_info =
{
    "XBOX YUV/RGB/BGR renderer",
    "DirectX",
    "e.beckers",
    ""
};


//********************************************************************************************
BOOL video_out_should_flip()
{
	return TRUE;
}




//********************************************************************************************
static unsigned int video_query_format(unsigned int format)
{
  return 0;
}
//********************************************************************************************
static unsigned int video_get_image(mp_image_t *mpi)
{
    return VO_FALSE;
}

//********************************************************************************************
static unsigned int video_put_image(mp_image_t *mpi)
{
}

//********************************************************************************************
static unsigned int video_control(unsigned int request, void *data, ...)
{
  return VO_NOTIMPL;
}

//********************************************************************************************
static unsigned int video_preinit(const char *arg)
{
    return 0;
}


//********************************************************************************************
static unsigned int video_config(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, unsigned int options, char *title, unsigned int format)
{
  return 0;
}


//********************************************************************************************
static void video_uninit(void)
{
}


//********************************************************************************************
static unsigned int video_draw_frame(unsigned char *src[])
{
	return VO_TRUE;
}

//********************************************************************************************
static unsigned int video_draw_slice(unsigned char *src[], int* stride, int w,int h,int x,int y )
{
	return 0;
}

//********************************************************************************************
static void video_flip_page(void)
{
}

static void video_check_events(void)
{
}


static void video_draw_alpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
{
}

static void video_draw_osd(void)
{
}



vo_functions_t video_functions =
{
	  &video_info,
		video_preinit,
		video_config,
		video_control,
		video_draw_frame,
		video_draw_slice,
		video_draw_osd,
		video_flip_page,
		video_check_events,
		video_uninit
};
