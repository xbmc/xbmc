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
bool												m_bPal60Allowed=true;
float												m_fScreenCompensationX=1.0f;
float												m_fScreenCompensationY=1.0f;

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


//********************************************************************************************************
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
	m_fScreenCompensationX=1.0f;
	m_fScreenCompensationY=1.0f;
}

//********************************************************************************************************
void choose_best_resolution(bool bPal60Allowed)
{
	if (!g_graphicsContext.IsFullScreenVideo() )
	{
		m_iDeviceWidth  = g_graphicsContext.GetWidth();
		m_iDeviceHeight = g_graphicsContext.GetHeight();
		m_fScreenCompensationX=1.0f;
		m_fScreenCompensationY=1.0f;
		return;
	}

  D3DPRESENT_PARAMETERS params, orgparams;
	g_application.GetD3DParameters(params);
	g_application.GetD3DParameters(orgparams);
	DWORD dwStandard = XGetVideoStandard();
	DWORD dwFlags	   = XGetVideoFlags();
	bool bPal60=false;
	bool bWideScreen=false;
	if (params.Flags&D3DPRESENTFLAG_WIDESCREEN) bWideScreen=true;
	if (bPal60Allowed&& (dwFlags&XC_VIDEO_FLAGS_PAL_60Hz) && !bWideScreen )
	{
		bPal60=true;
	}
	if ( (dwFlags&XC_VIDEO_FLAGS_HDTV_1080i) && bWideScreen )
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


	if ( (dwFlags&XC_VIDEO_FLAGS_HDTV_720p) &&bWideScreen )
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
		if (image_width <= 640 && image_height <=480)
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
		g_graphicsContext.Get3DDevice()->Reset(&params);
	}
	m_iDeviceWidth  = params.BackBufferWidth;
	m_iDeviceHeight = params.BackBufferHeight;

	m_fScreenCompensationX = ( (float)m_iDeviceWidth  ) / ( (float)orgparams.BackBufferWidth  );
	m_fScreenCompensationY = ( (float)m_iDeviceHeight ) / ( (float)orgparams.BackBufferHeight );
}


//********************************************************************************************************
static void Directx_CreateOverlay(unsigned int uiFormat)
{
	/*
			D3DFMT_YUY2 format (PC98 compliance). 
			Two pixels are stored in each YUY2 word. 
			Each data value (Y0, U, Y1, V) is 8 bits. This format is identical to the UYVY format described 
			except that the ordering of the bits is changed.

			D3DFMT_UYVY 
			UYVY format (PC98 compliance). Two pixels are stored in each UYVY word. 
			Each data value (U, Y0, V, Y1) is 8 bits. The U and V specify the chrominance and are 
			shared by both pixels. The Y0 and Y1 values specify the luminance of the respective pixels. 
			The chrominance and luminance can be computed from RGB values using the following formulas: 
			Y = 0.299 * R + 0.587 * G + 0.114 * B
			U = -0.168736 * R + -0.331264 * G + 0.5 * B
			V = 0.5 * R + -0.418688 * G + -0.081312 * B 
	*/
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

//********************************************************************************************************
static unsigned int Directx_ManageDisplay(unsigned int width,unsigned int height)
{
	float fOffsetX1 = (float)g_stSettings.m_iMoviesOffsetX1;
	float fOffsetY1 = (float)g_stSettings.m_iMoviesOffsetY1;
	float fOffsetX2 = (float)g_stSettings.m_iMoviesOffsetX2;
	float fOffsetY2 = (float)g_stSettings.m_iMoviesOffsetY2;

	fOffsetX1 *= m_fScreenCompensationX;
	fOffsetX2 *= m_fScreenCompensationX;
	fOffsetY1 *= m_fScreenCompensationY;
	fOffsetY2 *= m_fScreenCompensationY;
	
	float iScreenWidth =(float)m_iDeviceWidth  + fOffsetX2-fOffsetX1;
	float iScreenHeight=(float)m_iDeviceHeight + fOffsetY2-fOffsetY1;
	if( g_graphicsContext.IsFullScreenVideo() )
  {
		if (g_stSettings.m_bStretch)
		{
			// stretch the movie so it occupies the entire screen (aspect ratio = gone)
			rs.left		= 0;
			rs.top    = 0;
			rs.right	= image_width;
			rs.bottom = image_height ;
			
			rd.left   = (int)fOffsetX1;
			rd.right  = (int)rd.left+(int)iScreenWidth;
			rd.top    = (int)fOffsetY1;
			rd.bottom = (int)rd.top+(int)iScreenHeight;

			// place subtitles @ bottom of the screen
			iSubTitlePos			  = image_height - iSubTitleHeight;
			bClearSubtitleRegion= false;
			return 0;
		}

		if (g_stSettings.m_bZoom)
		{
				// zoom / panscan the movie so that it fills the entire screen
				// and keeps the Aspect ratio
				float fAR = ( (float)image_width ) / (  (float)image_height );
				if (g_graphicsContext.IsWidescreen())
				{
					fAR = ( (float)image_width ) / (  ((float)image_height)*1.33333f );
				}
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
				rs.right	= (int)image_width  - (int)fHorzBorder;
				rs.bottom = (int)image_height - (int)fVertBorder;

				rd.left   = (int)fOffsetX1;
				rd.right  = (int)rd.left + (int)iScreenWidth;
				rd.top    = (int)fOffsetY1;
				rd.bottom = (int)rd.top + (int)iScreenHeight;

				iSubTitlePos = rs.bottom - iSubTitleHeight;
				bClearSubtitleRegion= false;
				return 0;
		}


		// scale up image as much as possible
		// and keep the aspect ratio (introduces with black bars)
		float fAR = ( (float)image_width ) / (  (float)image_height );
		if (g_graphicsContext.IsWidescreen())
		{
			fAR = ( (float)image_width ) / (  ((float)image_height)*1.33333f );
		}
		float fNewWidth  = (float)( iScreenWidth);
		float fNewHeight = fNewWidth/fAR;
		if ( image_height > image_width)
		{
			fNewHeight=(float)iScreenHeight - (float)iSubTitleHeight;
			fNewWidth = fNewHeight*fAR;
		}
		if (fNewWidth > iScreenWidth || fNewHeight > iScreenHeight)
		{
			fNewWidth=(float)image_width;
			fNewHeight=(float)image_height;
		}

		rs.left		= 0;
		rs.top    = 0;
		rs.right	= image_width;
		rs.bottom = image_height + iSubTitleHeight;

		float iPosY = iScreenHeight - fNewHeight;
		float iPosX = iScreenWidth  - fNewWidth	;
		iPosY /= 2;
		iPosX /= 2;
		rd.left   = (int)iPosX + (int)fOffsetX1;
		rd.right  = (int)rd.left + (int)fNewWidth;
		rd.top    = (int)iPosY  + (int)fOffsetY1;
		rd.bottom = (int)rd.top + (int)fNewHeight + iSubTitleHeight;

		iSubTitlePos = image_height;
		bClearSubtitleRegion= true;

		if (iSubTitlePos  + 2*iSubTitleHeight >= m_iDeviceHeight - fOffsetY2)
		{
			bClearSubtitleRegion= false;
			iSubTitlePos = m_iDeviceHeight - (int)fOffsetY2 - iSubTitleHeight*2;
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

/********************************************************************************************************
  draw_osd(): this displays subtitles and OSD.
	It's a bit tricky to use it, since it's a callback-style stuff.
	It should call vo_draw_text() with screen dimension and your
	draw_alpha implementation for the pixelformat (function pointer).
	The vo_draw_text() checks the characters to draw, and calls
	draw_alpha() for each. As a help, osd.c contains draw_alpha for
	each pixelformats, use this if possible!
	NOTE: this one will be obsolete soon! But it's still usefull when
	you want to do tricks, like rendering osd _after_ hardware scaling
	(tdfxfb) or render subtitles under of the image (vo_mpegpes, sdl)
*/
static void video_draw_osd(void)
{
	vo_draw_text(image_width, image_height, draw_alpha);
}

/********************************************************************************************************
	 VOCTRL_QUERY_FORMAT  -  queries if a given pixelformat is supported.
	  It also returns various flags decsirbing the capabilities
		of the driver with teh given mode. for the flags, see file vfcaps.h !
		the most important flags, every driver must properly report
		these:
		    0x1	 -  supported (with or without conversion)
		    0x2  -  supported without conversion (define 0x1 too!)
		    0x100  -  driver/hardware handles timing (blocking)
		also SET sw/hw scaling and osd support flags, and flip,
		and accept_stride if you implement VOCTRL_DRAW_IMAGE (see bellow)
		NOTE: VOCTRL_QUERY_FORMAT may be called _before_ first config()
		but is always called between preinit() and uninit()
*/
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

//********************************************************************************************************
//      Uninit the whole system, this is on the same "level" as preinit.
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

//***********************************************************************************************************
/*init the video system (to support querying for supported formats)*/
static unsigned int video_preinit(const char *arg)
{
	m_fScreenCompensationX=1.0f;
	m_fScreenCompensationY=1.0f;
	iSubTitleHeight=0;
	iSubTitlePos=0;
	bClearSubtitleRegion=false;
	m_dwVisibleOverlay=0;
	m_bFlip=false;
	m_bFullScreen=false;
	fs=1;
  return 0;
}
/********************************************************************************************************
  draw_slice(): this displays YV12 pictures (3 planes, one full sized that
	contains brightness (Y), and 2 quarter-sized which the colour-info
	(U,V). MPEG codecs (libmpeg2, opendivx) use this. This doesn't have
	to display the whole frame, only update small parts of it.
*/
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
//********************************************************************************************************
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
		g_application.RenderFullScreen();
	  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();      
		g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_dwVisibleOverlay], &rs, &rd, TRUE, 0x00010001  );
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
				*(image + dstride*(iSubTitlePos + y)+x   ) = 0x10;	// for black Y=0x10  U=0x80 V=0x80
				*(image + dstride*(iSubTitlePos + y)+x+1 ) = 0x80;
			}
		}
	}

	if (m_bFullScreen!=g_graphicsContext.IsFullScreenVideo())
	{
		m_bFullScreen=g_graphicsContext.IsFullScreenVideo();
		g_graphicsContext.Lock();
		if (m_bFullScreen) choose_best_resolution(m_bPal60Allowed);
		else restore_resolution();
		g_graphicsContext.Unlock();
	}
}
/********************************************************************************************************
  draw_frame(): this is the older interface, this displays only complete
	frames, and can do only packed format (YUY2, RGB/BGR).
	Win32 codecs use this (DivX, Indeo, etc).
	If you implement VOCTRL_DRAW_IMAGE, you can left draw_frame.
*/
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
//********************************************************************************************************
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
/********************************************************************************************************
  Set up the video system. You get the dimensions and flags.
		width, height: size of the source image
    d_width, d_height: wanted scaled/display size (it's a hint)
    Flags:
      0x01	- force fullscreen (-fs)
			0x02	- allow mode switching (-vm)
			0x04	- allow software scaling (-zoom)
			0x08	- flipping (-flip)
      They're defined as VOFLAG_* (see libvo/video_out.h)
      
      IMPORTAMT NOTE: config() may be called 0 (zero), 1 or more (2,3...)
      times between preinit() and uninit() calls. You MUST handle it, and
      you shouldn't crash at second config() call or at uninit() without
      any config() call! To make your life easier, vo_config_count is
      set to the number of previous config() call, counted from preinit().
      It's set by the caller (vf_vo.c), you don't have to increase it!
      So, you can check for vo_config_count>0 in uninit() when freeing
      resources allocated in config() to avoid crash!
*/
static unsigned int video_config(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, unsigned int options, char *title, unsigned int format)
{
	char strFourCC[12];
	char strVideoCodec[256];
	float fps;
	unsigned int iWidth,iHeight;
	long tooearly, toolate;
	mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fps, &iWidth,&iHeight, &tooearly, &toolate);
	if (fps ==25.0f)
	{
		m_bPal60Allowed=false;
	}
	OutputDebugString("video_config\n");
  fs = options & 0x01;
  image_format	 =  format;
  image_width		 = width;
  image_height	 = height;
  d_image_width  = d_width;
  d_image_height = d_height;

	m_bFullScreen=g_graphicsContext.IsFullScreenVideo();
	choose_best_resolution(m_bPal60Allowed);

	aspect_save_orig(image_width,image_height);
  aspect_save_prescale(d_image_width,d_image_height);
  aspect_save_screenres( m_iDeviceWidth, m_iDeviceHeight );
  aspect(&d_image_width,&d_image_height,A_NOZOOM);

	fs=1;//fullscreen

	Directx_ManageDisplay(d_image_width,d_image_height);
  Directx_CreateOverlay(image_format);
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

//********************************************************************************************************
static unsigned int video_control(unsigned int request, void *data, ...)
{
  switch (request)
  {
    case VOCTRL_GET_IMAGE:
					//libmpcodecs Direct Rendering interface
					//You need to update mpi (mp_image.h) structure, for example,
					//look at vo_x11, vo_sdl, vo_xv or mga_common.
		return VO_NOTIMPL;

    case VOCTRL_QUERY_FORMAT:
      return query_format(*((unsigned int*)data));

    case VOCTRL_DRAW_IMAGE:
		/*	replacement for the current draw_slice/draw_frame way of
				passing video frames. by implementing SET_IMAGE, you'll get
				image in mp_image struct instead of by calling draw_*.
				unless you return VO_TRUE for VOCTRL_DRAW_IMAGE call, the
				old-style draw_* functils will be called!
				Note: draw_slice is still mandatory, for per-slice rendering!
		*/

//      return put_image( (mp_image_t*)data );
			return VO_NOTIMPL;

    case VOCTRL_FULLSCREEN:
    {
				//TODO
			return VO_NOTIMPL;
    }
		case VOCTRL_PAUSE:
		case VOCTRL_RESUME:
		case VOCTRL_RESET:
		case VOCTRL_GUISUPPORT:
		case VOCTRL_SET_EQUALIZER:
		case VOCTRL_GET_EQUALIZER:
		break;

    return VO_TRUE;
  };
  return VO_NOTIMPL;
}


//********************************************************************************************************
static vo_info_t video_info =
{
    "XBOX Direct3D8 YUV renderer",
    "directx",
    "Frodo",
    ""
};
//********************************************************************************************************
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
