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
#include "../../application.h"

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
bool												bClearSubtitleRegion=false;
int												  m_dwVisibleOverlay=0;
LPDIRECT3DTEXTURE8					m_pOverlay[2]={NULL,NULL};					// Overlay textures
LPDIRECT3DSURFACE8					m_pSurface[2]={NULL,NULL};				  // Overlay Surfaces
bool												m_bFlip=false;
static int									m_iDeviceWidth;
static int									m_iDeviceHeight;
bool												m_bFullScreen=false;

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
};
#define NUM_FORMATS (sizeof(g_ddpf) / sizeof(g_ddpf[0]))

void restore_resolution()
{
  D3DPRESENT_PARAMETERS params;
	g_application.GetD3DParameters(params);
	if (params.BackBufferWidth != m_iDeviceWidth ||
		  params.BackBufferHeight !=m_iDeviceHeight)
	{
		m_iDeviceWidth  = g_graphicsContext.GetWidth();
		m_iDeviceHeight = g_graphicsContext.GetHeight();
		g_graphicsContext.Get3DDevice()->Reset(&params);
	}
}

void choose_best_resolution()
{
	if (!g_graphicsContext.IsFullScreenVideo() )
	{
		m_iDeviceWidth  = g_graphicsContext.GetWidth();
		m_iDeviceHeight = g_graphicsContext.GetHeight();
		return;
	}

  D3DPRESENT_PARAMETERS params, orgparams;
	g_application.GetD3DParameters(params);
	g_application.GetD3DParameters(orgparams);
	DWORD dwStandard = XGetVideoStandard();
	DWORD dwFlags	   = XGetVideoFlags();
	bool bWideScreen=false;
	bool bPal60=false;

	if ( dwFlags & XC_VIDEO_FLAGS_WIDESCREEN)
	{
		bWideScreen=true;
	}

	if ( (dwFlags&XC_VIDEO_FLAGS_PAL_60Hz) && !bWideScreen )
	{
		bPal60=true;
	}
	if ( (dwFlags&XC_VIDEO_FLAGS_HDTV_1080i) && g_graphicsContext.IsWidescreen() )
	{
		//1920x1080 16:9
		if (dwStandard==XC_VIDEO_STANDARD_PAL_I)
		{
			if (image_width>720 || image_height>576)
			{
				params.BackBufferWidth =1920;
				params.BackBufferHeight=1080;
				params.Flags=D3DPRESENTFLAG_WIDESCREEN;
			}
		}
		else
		{
			if (image_width>720 || image_height>480)
			{
				params.BackBufferWidth =1920;
				params.BackBufferHeight=1080;
				params.Flags=D3DPRESENTFLAG_WIDESCREEN;
			}
		}
	}


	if ( (dwFlags&XC_VIDEO_FLAGS_HDTV_720p) && g_graphicsContext.IsWidescreen() )
	{
			//1280x720 16:9
		if (dwStandard==XC_VIDEO_STANDARD_PAL_I)
		{
			if (image_width>720 || image_height>576)
			{
				params.BackBufferWidth =1280;
				params.BackBufferHeight=720;
				params.Flags=D3DPRESENTFLAG_WIDESCREEN|D3DPRESENTFLAG_PROGRESSIVE;
			}
		}
		else
		{
			if (image_width>720 || image_height>480)
			{
				params.BackBufferWidth =1280;
				params.BackBufferHeight=720;
				params.Flags=D3DPRESENTFLAG_WIDESCREEN|D3DPRESENTFLAG_PROGRESSIVE;
			}
		}
	}


	if (dwStandard==XC_VIDEO_STANDARD_PAL_I)
	{
		if (image_width <= 720)
		{
			params.BackBufferWidth = 720;
			if (image_height <= 576) params.BackBufferHeight=576;
			if (image_height <= 480) params.BackBufferHeight=480;
		}
		if (image_width <= 640)
		{
			params.BackBufferWidth =640;
			params.BackBufferHeight=480;
		}
		if (bPal60)
		{
			params.FullScreen_RefreshRateInHz=60;
		}
	}
	else
	{
		if (image_width <= 720)
		{
			params.BackBufferWidth =720;
			params.BackBufferHeight=480;
		}
		if (image_width <= 640)
		{
			params.BackBufferWidth =640;
			params.BackBufferHeight=480;
		}
	}
	if (params.BackBufferHeight != orgparams.BackBufferHeight ||
		  params.BackBufferWidth  != orgparams.BackBufferWidth)
	{
		m_iDeviceWidth  = params.BackBufferWidth;
		m_iDeviceHeight = params.BackBufferHeight;
		g_graphicsContext.Get3DDevice()->Reset(&params);
	}
}

static void Directx_CreateOverlay(unsigned int uiFormat)
{
	for (int i=0; i <=1; i++)
	{
		if ( m_pSurface[i]) m_pSurface[i]->Release();
		if ( m_pOverlay[i]) m_pOverlay[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture( m_iDeviceWidth,
																										m_iDeviceHeight,
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
	int iScreenWidth =m_iDeviceWidth  +g_stSettings.m_iMoviesOffsetX2-g_stSettings.m_iMoviesOffsetX1;
	int iScreenHeight=m_iDeviceHeight +g_stSettings.m_iMoviesOffsetY2-g_stSettings.m_iMoviesOffsetY1;
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
			rd.right  = rd.left+iScreenWidth;
			rd.top    = g_stSettings.m_iMoviesOffsetY1;
			rd.bottom = rd.top+iScreenHeight;

			// place subtitles @ bottom of the screen
			iSubTitlePos			  = image_height - iSubTitleHeight;
			bClearSubtitleRegion= false;
			return 0;
		}

		if (g_stSettings.m_bZoom)
		{
				float fAR = ( (float)image_width ) / (  (float)image_height );
				float fNewWidth;
				float fNewHeight;
				if ( image_width >= image_height)
				{
					fNewHeight=(float)iScreenHeight;
					fNewWidth = fNewHeight*fAR;
				}
				else
				{
					fNewWidth  = (float)( iScreenWidth);
					fNewHeight = fNewWidth/fAR;
				}
				float fHorzBorder=(fNewWidth  - (float)iScreenWidth)/2.0f;
				float fVertBorder=(fNewHeight - (float)iScreenHeight)/2.0f;
				fHorzBorder =  (fHorzBorder/fNewWidth ) * ((float)image_width);
				fVertBorder =  (fVertBorder/fNewHeight) * ((float)image_height);
				rs.left		= (int)fHorzBorder;
				rs.top    = (int)fVertBorder;
				rs.right	= image_width  - (int)fHorzBorder;
				rs.bottom = image_height - (int)fVertBorder;

				rd.left   = g_stSettings.m_iMoviesOffsetX1;
				rd.right  = rd.left + iScreenWidth;
				rd.top    = g_stSettings.m_iMoviesOffsetY1;
				rd.bottom = rd.top + iScreenHeight;

				iSubTitlePos = rs.bottom - iSubTitleHeight;
				bClearSubtitleRegion= false;
				return 0;
		}

		// scale up image as much as possible
		// and keep the aspect ratio
		float fAR = ( (float)image_width ) / (  (float)image_height );
		float fNewWidth  = (float)( iScreenWidth);
		float fNewHeight = fNewWidth/fAR;
		if ( image_height > image_width)
		{
			fNewHeight=(float)iScreenHeight - (float)iSubTitleHeight;
			fNewWidth = fNewHeight*fAR;
		}

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

		iSubTitlePos = image_height;
		bClearSubtitleRegion= true;

		if (iSubTitlePos  + 2*iSubTitleHeight >= m_iDeviceHeight -g_stSettings.m_iMoviesOffsetY2)
		{
			bClearSubtitleRegion= false;
			iSubTitlePos = m_iDeviceHeight - g_stSettings.m_iMoviesOffsetY2 - iSubTitleHeight*2;
		}

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
	if ( y0 > (int)(image_height/2)  )
	{
		// get new height of subtitles
		iSubTitleHeight  = h;

		if (iSubTitlePos ==0) return;
		
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
	restore_resolution();
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
	bClearSubtitleRegion=false;
	m_dwVisibleOverlay=0;
	m_bFlip=false;
	m_bFullScreen=false;
	fs=1;
  return 0;
}

static unsigned int video_draw_slice(unsigned char *src[], int stride[], int w,int h,int x,int y )
{
	IMAGE img;
	img.y=src[0];
	img.u=src[1];
	img.v=src[2];
	while (w % 16) w++;
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
		g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
	  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();      
		g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_dwVisibleOverlay], &rs, &rd, FALSE, 0x00010001  );
		g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
		g_graphicsContext.Unlock();
	}
	else
	{
		g_graphicsContext.Lock();
		g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_dwVisibleOverlay], &rs, &rd, TRUE, 0x00010001  );
		g_graphicsContext.Unlock();
	}

	m_dwVisibleOverlay=1-m_dwVisibleOverlay;
	D3DLOCKED_RECT rectLocked;
	if ( D3D_OK == m_pOverlay[m_dwVisibleOverlay]->LockRect(0,&rectLocked,NULL,0L  ))
	{
		dstride=rectLocked.Pitch;
		image  =(unsigned char*)rectLocked.pBits;
	}

	if (bClearSubtitleRegion && iSubTitlePos>0)
	{
		// calculate the position of the subtitle
		// clear subtitle area (2 rows)
		for (int y=0; y < iSubTitleHeight*2; y++)
		{
			for (int x=0; x < (int)(dstride); x+=2)
			{
				*(image + dstride*(iSubTitlePos + y)+x   ) = 0x15;
				*(image + dstride*(iSubTitlePos + y)+x+1 ) = 0x80;
			}
		}
	}

	if (m_bFullScreen!=g_graphicsContext.IsFullScreenVideo())
	{
		m_bFullScreen=g_graphicsContext.IsFullScreenVideo();
		g_graphicsContext.Lock();
		if (m_bFullScreen) choose_best_resolution();
		else restore_resolution();
		g_graphicsContext.Unlock();
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

	m_bFullScreen=g_graphicsContext.IsFullScreenVideo();
	choose_best_resolution();

	aspect_save_orig(image_width,image_height);
  aspect_save_prescale(d_image_width,d_image_height);
  aspect_save_screenres( m_iDeviceWidth, m_iDeviceHeight );
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
