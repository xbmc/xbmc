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
#include "../../settings.h"

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
int													iSubTitleHeight=0;
int													iSubTitlePos=0;
int												  m_dwVisibleOverlay=0;
LPDIRECT3DTEXTURE8					m_pOverlay[2]={NULL,NULL};					// Overlay textures
LPDIRECT3DSURFACE8					m_pSurface[2]={NULL,NULL};				  // Overlay Surfaces
bool												m_bFlip=false;
typedef struct directx_fourcc_caps
{
    char*							img_format_name;      //human readable name
    unsigned int			img_format;						//as MPlayer image format
    unsigned int			drv_caps;							//what hw supports with this format
} directx_fourcc_caps;


#define DIRECT3D8CAPS VFCAP_CSP_SUPPORTED |VFCAP_OSD |VFCAP_HWSCALE_UP|VFCAP_HWSCALE_DOWN
static directx_fourcc_caps g_ddpf[] =
{
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
		g_graphicsContext.Get3DDevice()->CreateTexture( g_graphicsContext.GetWidth(),
																										g_graphicsContext.GetHeight(),
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
	int iScreenWidth =g_graphicsContext.GetWidth()  +g_stSettings.m_iMoviesOffsetX2-g_stSettings.m_iMoviesOffsetX1;
	int iScreenHeight=g_graphicsContext.GetHeight() +g_stSettings.m_iMoviesOffsetY2-g_stSettings.m_iMoviesOffsetY1;
	if( g_graphicsContext.IsFullScreenVideo() )
  {
		if (g_stSettings.m_bStretch)
		{
			// stretch the movie so it occupies the entire screen (aspect ratio = gone)
			rs.left		= 0;
			rs.top    = 0;
			rs.right	= image_width;
			rs.bottom = image_height ;
			
			rd.left   = g_stSettings.m_iMoviesOffsetX1;
			rd.right  = iScreenWidth;
			rd.top    = g_stSettings.m_iMoviesOffsetY1;
			rd.bottom = iScreenHeight;
			return 0;
		}

		if (g_stSettings.m_bZoom)
		{
			// scale up image as much as possible
			// and keep the aspect ratio
			float fAR = ( (float)image_width ) / (  (float)image_height );
			float fNewWidth  = (float)( iScreenWidth);
			float fNewHeight = fNewWidth/fAR;

			rs.left		= 0;
			rs.top    = 0;
			rs.right	= image_width;
			rs.bottom = image_height + iSubTitleHeight;

			int iPosY = iScreenHeight - (int)(fNewHeight);
			int iPosX = iScreenWidth  - (int)(fNewWidth)	;
			iPosY /= 2;
			iPosX /= 2;
			rd.left   = iPosX + g_stSettings.m_iMoviesOffsetX1;
			rd.right  = rd.left + (int)fNewWidth;
			rd.top    = iPosY  + g_stSettings.m_iMoviesOffsetY1;
			rd.bottom = rd.top + (int)fNewHeight + iSubTitleHeight;
			return 0;
		}

		// normal way
		// show image centered on screen. Keep aspect ratio
		rs.left		= 0;
		rs.top    = 0;
		rs.right	= image_width;
		rs.bottom = image_height + iSubTitleHeight;
		
		int iPosY = iScreenHeight - (rs.bottom-rs.top);
		int iPosX = iScreenWidth  - (rs.right-rs.left);
		iPosY /= 2;
		iPosX /= 2;
		rd.left   = iPosX + g_stSettings.m_iMoviesOffsetX1;
		rd.right  = rd.left + image_width;
		rd.top    = iPosY + g_stSettings.m_iMoviesOffsetY1;
		rd.bottom = rd.top + image_height + iSubTitleHeight;
		return 0;
	}
	else
	{
		// preview window.
		rs.left		= 0;
		rs.top    = 0;
		rs.right	= image_width;
		rs.bottom = image_height;
		
		const RECT& rv = g_graphicsContext.GetViewWindow();
		rd.left   = rv.left;
		rd.right  = rv.right;
		rd.top    = rv.top;
		rd.bottom = rv.bottom;

	}

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
	// if we draw text on the bottom then it must b the subtitles
	// if we're not in stretch mode try to put the subtitles below the video
	if (y0 > (int)(image_height/2)  && !g_stSettings.m_bStretch)
	{
		// get new height of subtitles
		if (h > iSubTitleHeight) iSubTitleHeight  = h;

		// calculate the position of the subtitle
		iSubTitlePos    = image_height;
		if ( (int)image_height+h+10 >= (int) g_graphicsContext.GetHeight() )
		{
			iSubTitlePos = g_graphicsContext.GetHeight() - h - 10;
		}
		// clear subtitle area
		for (int y=0; y < h; y++)
		{
			for (int x=0; x < (int)(dstride); x+=2)
			{
				*(image + dstride*(iSubTitlePos + y)+x   )   = 0x15;
				*(image + dstride*(iSubTitlePos + y)+x+1 ) = 0x80;
			}
		}
		vo_draw_alpha_yuy2(w,h,src,srca,stride,((unsigned char *) image) + dstride*iSubTitlePos + 2*x0,dstride);
		return;
	}
	vo_draw_alpha_yuy2(w,h,src,srca,stride,((unsigned char *) image) + dstride*y0 + 2*x0,dstride);
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
	OutputDebugString("video_uninit\n");
	g_graphicsContext.Get3DDevice()->EnableOverlay(FALSE);
	for (int i=0; i <=1; i++)
	{
		if ( m_pSurface[i]) m_pSurface[i]->Release();
		if ( m_pOverlay[i]) m_pOverlay[i]->Release();
		m_pSurface[i]=NULL;
		m_pOverlay[i]=NULL;
	}
	OutputDebugString("video_uninit done\n");
}

//***********************************************************************************************************
static void video_check_events(void)
{
}

static unsigned int video_preinit(const char *arg)
{
	iSubTitleHeight=0;
	iSubTitlePos=0;
	m_dwVisibleOverlay=0;
	m_bFlip=false;
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

	if (y+h+16 >= (int)image_height) m_bFlip=true;
//  yv12toyuy2(src[0],src[1],src[2],image + dstride*y + 2*x ,w,h,stride[0],stride[1],dstride);
  return 0;
}

static void video_flip_page(void)
{
	if (!m_bFlip) return;
	m_bFlip=false;
	m_pOverlay[m_dwVisibleOverlay]->UnlockRect(0);			

	Directx_ManageDisplay(d_image_width,d_image_height);
	if ( g_graphicsContext.IsFullScreenVideo())
	{
		g_graphicsContext.Lock();
		g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff202020, 1.0f, 0L );
	  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();      
		g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_dwVisibleOverlay], &rs, &rd, FALSE, 0x00010001  );
		g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
		g_graphicsContext.Unlock();
	}
	else
	{
		g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();      
		g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_dwVisibleOverlay], &rs, &rd, TRUE, 0x00010001  );
	}

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
	OutputDebugString("video_config\n");
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
	OutputDebugString("video_config() done\n");
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
//      return put_image( (mp_image_t*)data );
			return VO_NOTIMPL;

    case VOCTRL_FULLSCREEN:
    {
				//TODO
			return VO_NOTIMPL;
    }
    return VO_TRUE;
  };
  return VO_NOTIMPL;
}

void mplayer_reset_video_window()
{
	Directx_ManageDisplay(d_image_width,d_image_height);
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
