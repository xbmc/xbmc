/*
 * video_out_fb.c
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "video_out.h"
#include "convert.h"

typedef struct {
    vo_instance_t vo;
    int fbWidth;
    int fbHeight;
    int imgWidth ;
    int imgHeight ;
    int fd_ ;
    unsigned long memSize_ ;
    unsigned char *fbMem_ ;
    unsigned char *fbTop_ ;
    unsigned       fbStride_ ;
    unsigned       imgStride_ ;
    unsigned       pixelsPerRow_ ;
    unsigned       numRows_ ;
} fb_instance_t;

static void fb_start_fbuf( vo_instance_t * instance,
      uint8_t * const * buf, void * id)
{
}

static void fb_draw_frame24( vo_instance_t * _instance,
        uint8_t * const * buf, void * id)
{
   fb_instance_t * instance = (fb_instance_t *)_instance ;
   unsigned char *dest = instance->fbTop_ ;
   unsigned char const *src = buf[0];
   unsigned i ;
   for( i = 0 ; i < instance->numRows_ ; i++ )
   {
      memcpy( dest, src, instance->pixelsPerRow_*3 );
   }
}

static void fb_draw_frame16(vo_instance_t * _instance,
       uint8_t * const * buf, void * id)
{
   fb_instance_t * instance = (fb_instance_t *)_instance ;
   unsigned char *dest = instance->fbTop_ ;
   unsigned char const *src = buf[0];
   unsigned i ;
   for( i = 0 ; i < instance->numRows_ ; i++ )
   {
      memcpy( dest, src, instance->pixelsPerRow_*sizeof(unsigned short) );
      dest += instance->fbStride_ ;
      src  += instance->imgStride_ ;
   }
}

static void fb_discard (vo_instance_t * _instance,
    uint8_t * const * buf, void * id)
{
}

static void fb_close (vo_instance_t * _instance)
{
   fb_instance_t * instance = (fb_instance_t *)_instance ;
   if( 0 <= instance->fd_ )
   {
      if( MAP_FAILED != instance->fbMem_ )
         munmap( instance->fbMem_, instance->memSize_ );
      close( instance->fd_ );
   }
}

static int fb_setup (vo_instance_t * _instance, unsigned int width,
		     unsigned int height, unsigned int chroma_width,
		     unsigned int chroma_height, vo_setup_result_t * result)
{
   fb_instance_t * instance = (fb_instance_t *) _instance;
   instance->fd_ = open( "/dev/fb", O_RDWR );
   if( 0 <= instance->fd_ )
   {
      struct fb_var_screeninfo variable_info;

      struct fb_fix_screeninfo fixed_info;
      int err = ioctl( instance->fd_, FBIOGET_FSCREENINFO, &fixed_info);
      if( 0 == err )
      {
         err = ioctl( instance->fd_, FBIOGET_VSCREENINFO, &variable_info );
         if( 0 == err )
         {
            instance->fbWidth = variable_info.xres ;
            instance->fbHeight = variable_info.yres ;
            instance->imgWidth  = width ;
            instance->imgHeight = height ;
            instance->memSize_ = fixed_info.smem_len ;
            instance->fbMem_ = mmap( 0, fixed_info.smem_len, PROT_WRITE|PROT_WRITE, MAP_SHARED, instance->fd_, 0 );
            if( MAP_FAILED != instance->fbMem_ )
            {
               if( 16 == variable_info.bits_per_pixel )
               {
                  result->convert = convert_rgb16 ;
                  instance->fbTop_     = (unsigned char *)instance->fbMem_;
                  instance->fbStride_  = variable_info.xres * sizeof(unsigned short );
                  instance->imgStride_ = width * sizeof(unsigned short) ; // rgb16
                  if( width > variable_info.xres )
                     instance->pixelsPerRow_ = variable_info.xres;
                  else
                     instance->pixelsPerRow_ = width ;
                  if( height > variable_info.yres )
                     instance->numRows_ = variable_info.yres ;
                  else
                     instance->numRows_ = height ;
                  instance->vo.draw = fb_draw_frame16 ;
                  return 0 ;
               }
               else if( 24 == variable_info.bits_per_pixel )
               {
                  result->convert = convert_rgb24 ;
                  instance->fbTop_ = (unsigned char *)instance->fbMem_ ;
                  instance->fbStride_  = variable_info.xres * 3 ;
                  instance->imgStride_ = width * 3 ;
                  if( width > variable_info.xres )
                     instance->pixelsPerRow_ = variable_info.xres;
                  else
                     instance->pixelsPerRow_ = width ;
                  if( height > variable_info.yres )
                     instance->numRows_ = variable_info.yres ;
                  else
                     instance->numRows_ = height ;
                  instance->vo.draw = fb_draw_frame24 ;
                  return 0 ;
               }
               else
               {
fprintf( stderr, "doesn't support this bpp. add code here\n" );
               }
            }
            else
               perror( "mmap" );
         }
         else
            perror( "GET_VSCREENINFO" );
      }
      else
         perror( "GET_FSCREENINFO" );
      close( instance->fd_ );
      instance->fd_ = -1 ;
   }
   else
      perror( "/dev/fb" );

   return -1 ;
}

vo_instance_t * vo_fb_open (void)
{
    fb_instance_t * instance;
    instance = (fb_instance_t *) malloc (sizeof (fb_instance_t));
    if (instance == NULL)
 return NULL;
    instance->fd_  = -1 ;
    instance->fbMem_ = MAP_FAILED ;
    instance->memSize_ = 0 ;
    instance->vo.setup = fb_setup;
    instance->vo.setup_fbuf = NULL ; // fb_setup_fbuf;
    instance->vo.set_fbuf = NULL;
    instance->vo.start_fbuf = fb_start_fbuf;
    instance->vo.discard = fb_discard;
    instance->vo.draw = NULL ;
    instance->vo.close = fb_close;

    return (vo_instance_t *) instance;
}
