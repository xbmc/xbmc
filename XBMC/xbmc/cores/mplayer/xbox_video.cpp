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

#include "video.h"
#include "mplayer.h"
#include "GraphicContext.h"

static RECT                 rd;                     						//rect of our stretched image
static RECT                 rs;                     						//rect of our source image
static unsigned int 				image_width, image_height;          //image width and height
static unsigned int 				d_image_width, d_image_height;      //image width and height zoomed
static unsigned char*				image=NULL;                         //image data
static unsigned int 				image_format=0;                     //image format
static unsigned int 				primary_image_format;
static unsigned int 				fs = 0;                             //display in window or fullscreen
static unsigned int 				dstride;                            //surface stride
static DWORD    						destcolorkey;                       //colorkey for our surface
int												  m_dwVisibleOverlay=0;
LPDIRECT3DTEXTURE8					m_pOverlay[2]={NULL,NULL};					// Overlay textures
LPDIRECT3DSURFACE8					m_pSurface[2]={NULL,NULL};				  // Overlay Surfaces
	

typedef struct directx_fourcc_caps
{
    char*							img_format_name;      //human readable name
    unsigned int			img_format;						//as MPlayer image format
    unsigned int			drv_caps;							//what hw supports with this format
} directx_fourcc_caps;


#define DIRECT3D8CAPS VFCAP_CSP_SUPPORTED |VFCAP_OSD 
static directx_fourcc_caps g_ddpf[] =
{
    {"YUY2 ",IMGFMT_YUY2 ,DIRECT3D8CAPS|VFCAP_CSP_SUPPORTED_BY_HW},
    {"YV12 ",IMGFMT_YV12 ,DIRECT3D8CAPS},
//    {"I420 ",IMGFMT_I420 ,DIRECT3D8CAPS},//yv12 with swapped uv
 //   {"IYUV ",IMGFMT_IYUV ,DIRECT3D8CAPS},//same as i420
   // {"YVU9 ",IMGFMT_YVU9 ,DIRECT3D8CAPS},
  //  {"RGB15",IMGFMT_RGB15,DIRECT3D8CAPS},   //RGB 5:5:5
   // {"BGR15",IMGFMT_BGR15,DIRECT3D8CAPS},
   // {"RGB16",IMGFMT_RGB16,DIRECT3D8CAPS},   //RGB 5:6:5
   // {"BGR16",IMGFMT_BGR16,DIRECT3D8CAPS},
   // {"RGB24",IMGFMT_RGB24,DIRECT3D8CAPS},
   // {"BGR24",IMGFMT_BGR24,DIRECT3D8CAPS},
   // {"RGB32",IMGFMT_RGB32,DIRECT3D8CAPS},
   // {"BGR32",IMGFMT_BGR32,DIRECT3D8CAPS}
};
#define NUM_FORMATS (sizeof(g_ddpf) / sizeof(g_ddpf[0]))

static void Directx_CreateOverlay(unsigned int uiFormat)
{
	for (int i=0; i <=1; i++)
	{
		if ( m_pSurface[i]) m_pSurface[i]->Release();
		if ( m_pOverlay[i]) m_pOverlay[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture( image_width,
																										image_height,
																										1,
																										0,
																										D3DFMT_YUY2,
																										0,
																										&m_pOverlay[i] ) ;
		m_pOverlay[i]->GetSurfaceLevel( 0, &m_pSurface[i] );
	}
	g_graphicsContext.Get3DDevice()->EnableOverlay(TRUE);
}

static unsigned int Directx_ManageDisplay(unsigned int width,unsigned int height)
{
	//TODO
  RECT            rd_window;
  DWORD           dwUpdateFlags=0;
  unsigned int    xscreen = g_graphicsContext.GetWidth();
  unsigned int    yscreen = g_graphicsContext.GetHeight();
  if(fs)
  {
    /*center and zoom image*/
    rd_window.top = 0;
    rd_window.left = 0;
    rd_window.right = xscreen;
    rd_window.bottom = yscreen;
    aspect(&width,&height,A_ZOOM);
    rd.left = (xscreen-width)/2;
    rd.right = rd.left+width;
    rd.top = (yscreen-height)/2;
    rd.bottom = rd.top + height;
  }
	unsigned int uStretchFactor1000;  //minimum stretch 
	unsigned int xstretch1000,ystretch1000; 

	/*get minimum stretch, depends on display adaptor and mode (refresh rate!) */
	uStretchFactor1000 = 1000;
	rd.right = ((width+rd.left)*uStretchFactor1000+999)/1000;
	rd.bottom = (height+rd.top)*uStretchFactor1000/1000;
	/*calculate xstretch1000 and ystretch1000*/
	xstretch1000 = ((rd.right - rd.left)* 1000)/image_width ;
	ystretch1000 = ((rd.bottom - rd.top)* 1000)/image_height;
  /*handle move outside of window with cropping not really needed with colorkey, but shouldn't hurt*/
	rs.left=0;
	rs.right=image_width;
	rs.top=0;
	rs.bottom=image_height;
  if(!fs)
		rd_window = rd;         /*don't crop the window !!!*/
	if(rd.left < 0)         //move out left
	{
			rs.left=(-rd.left*1000)/xstretch1000;
			rd.left = 0; 
	}
	else rs.left=0;
	if(rd.top < 0)          //move out up
	{
		rs.top=(-rd.top*1000)/ystretch1000;
		rd.top = 0;
	}
	else rs.top = 0;
	if(rd.right > (int)xscreen)  //move out right
	{
		rs.right=((xscreen-rd.left)*1000)/xstretch1000;
		rd.right= xscreen;
	}
	else rs.right = image_width;
	if(rd.bottom > (int)yscreen) //move out down
	{
		rs.bottom=((yscreen-rd.top)*1000)/ystretch1000;
		rd.bottom= yscreen;
	}
	else rs.bottom= image_height;

	//printf("Window:x:%i,y:%i,w:%i,h:%i\n",rd_window.left,rd_window.top,rd_window.right - rd_window.left,rd_window.bottom - rd_window.top);
  //printf("Overlay:x1:%i,y1:%i,x2:%i,y2:%i,w:%i,h:%i\n",rd.left,rd.top,rd.right,rd.bottom,rd.right - rd.left,rd.bottom - rd.top);
  //printf("Source:x1:%i,x2:%i,y1:%i,y2:%i\n",rs.left,rs.right,rs.top,rs.bottom);
  //printf("Image:x:%i->%i,y:%i->%i\n",image_width,d_image_width,image_height,d_image_height);

  return 0;
}

//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************

static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,unsigned char *srca, int stride)
{
	vo_draw_alpha_yv12(w,h,src,srca,stride,((unsigned char *) image) + dstride*y0 + x0,dstride);
	//vo_draw_alpha_yuy2(w,h,src,srca,stride,((unsigned char *) image) + dstride*y0 + 2*x0 + 1,dstride);
}

static void video_draw_osd(void)
{
	vo_draw_text(image_width, image_height, draw_alpha);
}

static unsigned int query_format(unsigned int format)
{
  unsigned int i=0;
  while ( i < NUM_FORMATS )
  {
      if (g_ddpf[i].img_format == format)
          return g_ddpf[i].drv_caps;
      i++;
  }
  return 0;
}

static void video_uninit(void)
{
	g_graphicsContext.Get3DDevice()->EnableOverlay(FALSE);
	for (int i=0; i <=1; i++)
	{
		if ( m_pSurface[i]) m_pSurface[i]->Release();
		if ( m_pOverlay[i]) m_pOverlay[i]->Release();
		m_pSurface[i]=NULL;
		m_pOverlay[i]=NULL;
	}
}

//***********************************************************************************************************
static void video_check_events(void)
{
}

static unsigned int video_preinit(const char *arg)
{
	fs=1;
  return 0;
}

static unsigned int video_draw_slice(unsigned char *src[], int stride[], int w,int h,int x,int y )
{
	IMAGE img;
	img.y=src[0];
	img.u=src[1];
	img.v=src[2];
	image_output( &img,									// image
								 w,										// width
								 h,										// height
								 stride[0],						// edged_width=stride of Y plane
								 image+dstride*y+2*x,         
								 dstride>>1,							//	dst stride
								 XVID_CSP_YUY2 ,0);

//  yv12toyuy2(src[0],src[1],src[2],image + dstride*y + 2*x ,w,h,stride[0],stride[1],dstride);
  return 0;
}

static void video_flip_page(void)
{
	m_pOverlay[m_dwVisibleOverlay]->UnlockRect(0);			
	g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_dwVisibleOverlay], &rs, &rd, FALSE, 0x00010001  );

	m_dwVisibleOverlay=1-m_dwVisibleOverlay;
	D3DLOCKED_RECT rectLocked;
	if ( D3D_OK == m_pOverlay[m_dwVisibleOverlay]->LockRect(0,&rectLocked,NULL,0L  ))
	{
		dstride=rectLocked.Pitch;
		image  =(unsigned char*)rectLocked.pBits;
	}
}

static unsigned int video_draw_frame(unsigned char *src[])
{
	IMAGE img;
	img.y=src[0];
	img.u=src[1];
	img.v=src[2];
	image_output( &img,									// image
								 image_width,					// width
								 image_height,				// height
								 image_width,					// edged_width=stride of Y plane
								 image,         
								 dstride>>1,							//	dst stride
								 XVID_CSP_YUY2 ,0);

	//yv12toyuy2(src[0],src[1],src[2],image,image_width,image_height,image_width,image_width>>1,dstride);
  return 0;
}

static unsigned int get_image(mp_image_t *mpi)
{
	return 0;
}

static unsigned int put_image(mp_image_t *mpi)
{
  unsigned int x = mpi->x;
  unsigned int y = mpi->y;
  unsigned int w = mpi->w;
  unsigned int h = mpi->h;
	IMAGE img;
	img.y=mpi->planes[0];
	img.u=mpi->planes[1];
	img.v=mpi->planes[2];
	
	image_output( &img,							// image
								 w,								// width
								 h,								// height
								 mpi->stride[0],	// edged_width=stride of Y plane
								 image+dstride*y+2*x,         						
								 dstride>>1,			//	dst stride
								 XVID_CSP_YUY2 ,0);

	//yv12toyuy2(src[0],src[1],src[2],image,image_width,image_height,image_width,image_width>>1,dstride);
  return VO_TRUE;
}

static unsigned int video_config(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, unsigned int options, char *title, unsigned int format)
{
  fs = options & 0x01;
  image_format	 =  format;
  image_width		 = width;
  image_height	 = height;
  d_image_width  = d_width;
  d_image_height = d_height;
  

	aspect_save_orig(image_width,image_height);
  aspect_save_prescale(d_image_width,d_image_height);
  aspect_save_screenres( g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight() );
  aspect(&d_image_width,&d_image_height,A_NOZOOM);

	fs=1;//fullscreen

  Directx_CreateOverlay(image_format);
	Directx_ManageDisplay(d_image_width,d_image_height);

	// get stride
	D3DLOCKED_RECT rectLocked;
	if ( D3D_OK == m_pOverlay[m_dwVisibleOverlay]->LockRect(0,&rectLocked,NULL,0L  ) )
	{
		dstride=rectLocked.Pitch;
		image  =(unsigned char*)rectLocked.pBits;
	}
  return 0;
}
static unsigned int video_control(unsigned int request, void *data, ...)
{
  switch (request)
  {
    case VOCTRL_GET_IMAGE:
    //  return get_image( (mp_image_t*)data);
		return VO_NOTIMPL;

    case VOCTRL_QUERY_FORMAT:
      return query_format(*((unsigned int*)data));

    case VOCTRL_DRAW_IMAGE:
      return put_image( (mp_image_t*)data );

    case VOCTRL_FULLSCREEN:
    {
				//TODO
			return VO_NOTIMPL;
    }
    return VO_TRUE;
  };
  return VO_NOTIMPL;
}

static vo_info_t video_info =
{
    "XBOX Direct3D8 YUV renderer",
    "directx",
    "Frodo",
    ""
};

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
