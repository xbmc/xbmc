/*
 * video_out_mga.c
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

/* FIXME this should allocate AGP memory via agpgart and then we */
/* can use AGP transfers to the framebuffer */

#include "config.h"

#ifdef LIBVO_MGA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

#include "hw_bes.h"
#include "attributes.h"
#include "mmx.h"

static void yuvinterleave (uint8_t * dst, uint8_t * pu, uint8_t * pv,
			   int width)
{
    width >>= 3;
    do {
	dst[0] = pu[0];
	dst[1] = pv[0];
	dst[2] = pu[1];
	dst[3] = pv[1];
	dst[4] = pu[2];
	dst[5] = pv[2];
	dst[6] = pu[3];
	dst[7] = pv[3];
	dst += 8;
	pu += 4;
	pv += 4;
    } while (--width);
}

static void yuv2g200_c (uint8_t * dst, uint8_t * py,
			uint8_t * pu, uint8_t * pv,
			int width, int height,
			int bes_stride, int y_stride, int uv_stride)
{
    int i;

    i = height;
    do {
	memcpy (dst, py, width);
	py += y_stride;
	dst += bes_stride;
    } while (--i);

    i = height >> 1;
    do {
	yuvinterleave (dst, pu, pv, width);
	pu += uv_stride;
	pv += uv_stride;
	dst += bes_stride;
    } while (--i);
}

static void yuv2g400_c (uint8_t * dst, uint8_t * py,
			uint8_t * pu, uint8_t * pv,
			int width, int height,
			int bes_stride, int y_stride, int uv_stride)
{
    int i;

    i = height;
    do {
	memcpy (dst, py, width);
	py += y_stride;
	dst += bes_stride;
    } while (--i);

    width >>= 1;
    bes_stride >>= 1;
    i = height >> 1;
    do {
	memcpy (dst, pu, width);
	pu += uv_stride;
	dst += bes_stride;
    } while (--i);

    i = height >> 1;
    do {
	memcpy (dst, pv, width);
	pv += uv_stride;
	dst += bes_stride;
    } while (--i);
}

typedef struct {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t * frame_ptr[3];
    vo_frame_t frame[3];

    int fd;
    mga_vid_config_t mga_vid_config;
    uint8_t * vid_data;
    uint8_t * frame0;
    uint8_t * frame1;
    int next_frame;
    int stride;
} mga_instance_t;

static void mga_draw_frame (vo_frame_t * frame)
{
    mga_instance_t * instance;

    instance = (mga_instance_t *) frame->instance;

    yuv2g400_c (instance->vid_data,
		frame->base[0], frame->base[1], frame->base[2],
		instance->mga_vid_config.src_width,
		instance->mga_vid_config.src_height,
		instance->stride, instance->mga_vid_config.src_width,
		instance->mga_vid_config.src_width >> 1);

    ioctl (instance->fd, MGA_VID_FSEL, &instance->next_frame);

    instance->next_frame ^= 2; /* switch between fields A1 and B1 */
    if (instance->next_frame)
	instance->vid_data = instance->frame1;
    else
	instance->vid_data = instance->frame0;
}

static void mga_close (vo_instance_t * _instance)
{
    mga_instance_t * instance;

    instance = (mga_instance_t *) _instance;

    close (instance->fd);
    libvo_common_free_frames ((vo_instance_t *) instance);
}

static int mga_setup (vo_instance_t * _instance, unsigned int width,
		      unsigned int height, unsigned int chroma_width,
		      unsigned int chroma_height, vo_setup_result_t * result)
{
    mga_instance_t * instance;
    char * frame_mem;
    int frame_size;

    instance = (mga_instance_t *) _instance;

    if (ioctl (instance->fd, MGA_VID_ON, 0)) {
	close (instance->fd);
	return 1;
    }

    instance->mga_vid_config.src_width = width;
    instance->mga_vid_config.src_height = height;
    instance->mga_vid_config.dest_width = width;
    instance->mga_vid_config.dest_height = height;
    instance->mga_vid_config.x_org = 10;
    instance->mga_vid_config.y_org = 10;
    instance->mga_vid_config.colkey_on = 1;

    if (ioctl (instance->fd, MGA_VID_CONFIG, &(instance->mga_vid_config)))
	perror ("Error in instance->mga_vid_config ioctl");
    ioctl (instance->fd, MGA_VID_ON, 0);

    instance->stride = (width + 31) & ~31;
    frame_size = instance->stride * height * 3 / 2;
    frame_mem = (char*)mmap (0, frame_size*2, PROT_WRITE, MAP_SHARED, instance->fd, 0);
    instance->frame0 = frame_mem;
    instance->frame1 = frame_mem + frame_size;
    instance->vid_data = frame_mem;
    instance->next_frame = 0;

    return libvo_common_alloc_frames ((vo_instance_t *) instance,
				      width, height, sizeof (vo_frame_t),
				      NULL, NULL, mga_draw_frame);
}

vo_instance_t * vo_mga_open (void)
{
    mga_instance_t * instance;

    instance = malloc (sizeof (mga_instance_t));
    if (instance == NULL)
	return NULL;

    instance->fd = open ("/dev/mga_vid", O_RDWR);
    if (instance->fd < 0) {
	free (instance);
	return NULL;
    }

    instance->vo.setup = mga_setup;
    instance->vo.close = mga_close;
    instance->vo.set_frame = libvo_common_set_frame;

    return (vo_instance_t *) instance;
}
#endif
