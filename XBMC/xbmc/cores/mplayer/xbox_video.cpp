/*
 * XBoxMediaCenter
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
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
#include "../../util.h"

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
bool												m_bWidescreen=false;
float												m_fFPS;
static int									m_iResolution=0;

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

static float fScreenModePixelRatio[9] = 
{
		1.0f,							// 0 = invalid resolution. Does not exists
		1.0f,							// 1 = 1920x1280
		1.0f,							// 2 = 1280x720
		1.0f,							// 3 = 640x480 NTSC
		10.0f/11.0f,					// 4 = 720x480 NTSC
		1.0f,							// 5 = 640x480 PAL  (PAL60)
		10.0f/11.0f,					// 6 = 720x480 PAL  (PAL60)
		6.0f/5.0f,						// 7 = 640x576 PAL  (PAL50) (Not 100% sure on this one)
		59.0f/54.0f						// 8 = 720x576 PAL  (PAL50)
};

#define NUM_FORMATS (sizeof(g_ddpf) / sizeof(g_ddpf[0]))



static void video_flip_page(void);

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
}

unsigned int get_resolution_width(int iRes)
{
	int iWidth[9] = { 0, 1920, 1280, 640, 720, 640, 720, 640, 720 };
	int iOffsetX1 = g_stSettings.m_rectMovieCalibration[iRes].left;
	int iOffsetX2 = g_stSettings.m_rectMovieCalibration[iRes].right;
	return iWidth[iRes]+iOffsetX2-iOffsetX1;
}

//********************************************************************************************************
void choose_best_resolution(float fps)
{
	D3DPRESENT_PARAMETERS params, orgparams;
	g_application.GetD3DParameters(params);
	g_application.GetD3DParameters(orgparams);
	DWORD dwStandard = XGetVideoStandard();
	DWORD dwFlags	   = XGetVideoFlags();


	bool bUsingPAL		= (dwStandard==XC_VIDEO_STANDARD_PAL_I);		// current video standard:PAL or NTSC 
	bool bCanDoWidescreen = (dwFlags & XC_VIDEO_FLAGS_WIDESCREEN);		// can widescreen be enabled?

	// Work out if the framerate suits PAL50 or PAL60
	bool bPal60SuitsFrameRate = true;
	if (floor(50.0f/fps)==50.0f/fps)
		bPal60SuitsFrameRate = false;

	// Work out if framesize suits 4:3 or 16:9
	bool bShouldUseWidescreen = false;
	if (bCanDoWidescreen && ((float)d_image_width/d_image_height > 4.0f/3.0f))
	{
		bShouldUseWidescreen = true;
	}

	bool bPal60=false;
	bool bUsingWidescreen=false;
	if (params.Flags&D3DPRESENTFLAG_WIDESCREEN) bUsingWidescreen=true;  // WIDESCREEN 16:9 ?
	if (bUsingPAL && bPal60SuitsFrameRate && g_stSettings.m_bAllowPAL60 && (dwFlags&XC_VIDEO_FLAGS_PAL_60Hz) && !bUsingWidescreen )
	{
		// yes we're in PAL
		// yes PAL60 is allowed
		// yes dashboard PAL60 settings is enabled
		// yep, we're using 4:3 (pal60 doesnt work in widescreen 16:9) <- CHECK THIS!!
		bPal60=true;
	}

	// if video switching is not allowed then use current resolution (with pal 60 if needed)
	// if we're not in fullscreen mode then use current resolution 
	// if we're calibrating the video  then use current resolution 
	if  ( (!g_stSettings.m_bAllowVideoSwitching) ||
		    (! ( g_graphicsContext.IsFullScreenVideo()|| g_graphicsContext.IsCalibrating())  )
			)
	{
		if (bPal60)
		{
			params.BackBufferHeight = 480;			// PAL60 must have 480 lines
			params.FullScreen_RefreshRateInHz=60;
		}
		else
		{
			params.BackBufferHeight = 576;			// PAL50 must have 576 lines
			params.FullScreen_RefreshRateInHz=50;
		}
		if (params.FullScreen_RefreshRateInHz != orgparams.FullScreen_RefreshRateInHz)
		{
			g_graphicsContext.Get3DDevice()->Reset(&params);
		}

		m_iDeviceWidth  = params.BackBufferWidth;
		m_iDeviceHeight = params.BackBufferHeight;

		m_iResolution=CUtil::GetResolution( m_iDeviceWidth, m_iDeviceHeight, bUsingPAL, bPal60);
		m_bWidescreen = bUsingWidescreen;
		g_graphicsContext.SetVideoResolution(m_iResolution);
		return;
	}

	if ( (dwFlags&XC_VIDEO_FLAGS_HDTV_1080i) && bUsingWidescreen )
	{
		//1920x1080 16:9
		if (bUsingPAL)
		{
			if (image_width>720 || image_height>576)
			{
				params.BackBufferWidth =1920;
				params.BackBufferHeight=1080;
				params.Flags=D3DPRESENTFLAG_INTERLACED|D3DPRESENTFLAG_WIDESCREEN;
				bShouldUseWidescreen=true;
			}
		}
		else
		{
			if (image_width>720 || image_height>480)
			{
				params.BackBufferWidth =1920;
				params.BackBufferHeight=1080;
				params.Flags=D3DPRESENTFLAG_INTERLACED|D3DPRESENTFLAG_WIDESCREEN;
				bShouldUseWidescreen=true;
			}
		}
	}

	if ( (dwFlags&XC_VIDEO_FLAGS_HDTV_720p) && bUsingWidescreen )
	{
		//1280x720 16:9
		if (bUsingPAL)
		{
			if (image_width>720 || image_height>576)
			{
				params.BackBufferWidth =1280;
				params.BackBufferHeight=720;
				params.Flags=D3DPRESENTFLAG_WIDESCREEN|D3DPRESENTFLAG_PROGRESSIVE;
				bShouldUseWidescreen=true;
			}
		}
		else
		{
			if (image_width>720 || image_height>480)
			{
				params.BackBufferWidth =1280;
				params.BackBufferHeight=720;
				params.Flags=D3DPRESENTFLAG_WIDESCREEN|D3DPRESENTFLAG_PROGRESSIVE;
				bShouldUseWidescreen=true;
			}
		}
	}

	if (bUsingPAL)
	{
		if (bPal60)	// PAL60
		{
			params.FullScreen_RefreshRateInHz=60;
			params.BackBufferWidth = 720;
			params.BackBufferHeight = 480;
			if (image_width <= get_resolution_width(5))
			{
				params.BackBufferWidth = 640;
				params.BackBufferHeight = 480;
			}
			if (dwFlags&XC_VIDEO_FLAGS_HDTV_480p) params.Flags = D3DPRESENTFLAG_PROGRESSIVE;	// 480p
		}
		else				// PAL50
		{
			params.FullScreen_RefreshRateInHz=50;
			params.BackBufferWidth = 720;
			params.BackBufferHeight = 576;
			if (image_width <= get_resolution_width(7))
			{
				params.BackBufferWidth = 640;
				params.BackBufferHeight = 576;
			}
		}
	}
	else	// using NTSC
	{
		params.BackBufferWidth =720;
		params.BackBufferHeight=480;
		if (image_width <= get_resolution_width(3))
		{
			params.BackBufferWidth =640;
			params.BackBufferHeight=480;
		}		
		if (dwFlags&XC_VIDEO_FLAGS_HDTV_480p) params.Flags = D3DPRESENTFLAG_PROGRESSIVE;
	}
	// turn on/off widescreen flag as necessary
	if (bShouldUseWidescreen)
	{
		params.Flags |= D3DPRESENTFLAG_WIDESCREEN;
		m_bWidescreen = true;
	}
	else // (!bShouldUseWidescreen)
	{
		params.Flags &= ~D3DPRESENTFLAG_WIDESCREEN;
		m_bWidescreen = false;
	}

	if (params.BackBufferHeight != orgparams.BackBufferHeight ||
		  params.BackBufferWidth  != orgparams.BackBufferWidth ||
			params.FullScreen_RefreshRateInHz !=orgparams.FullScreen_RefreshRateInHz ||
				params.Flags != orgparams.Flags	)
	{
		g_graphicsContext.Get3DDevice()->Reset(&params);
	}
	m_iDeviceWidth  = params.BackBufferWidth;
	m_iDeviceHeight = params.BackBufferHeight;


	m_iResolution=CUtil::GetResolution( m_iDeviceWidth, m_iDeviceHeight, bUsingPAL, bPal60);
	g_graphicsContext.SetVideoResolution(m_iResolution);

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

//***************************************************************************************
// GetSourcePixelRatio()
//
// Considers the source frame size and output frame size (as suggested by mplayer)
// to determine if the pixels in the source are not square.  It returns the pixel
// ratio.  We consider the cases of VCD, SVCD and DVD separately, as these are intended
// to be viewed on a non-square pixel TV set, so the pixels are defined to be the same
// ratio as the intended display pixels.  These formats are determined by frame size.
//***************************************************************************************
static float GetSourcePixelRatio(int iSourceWidth, int iSourceHeight, int iOutputWidth, int iOutputHeight)
{
  float fSourcePixelRatio = 1.0f;

  // Check whether mplayer has decided that the size of the video file should be changed
  // This indicates either a scaling has taken place (which we didn't ask for) or it has
  // found an aspect ratio parameter from the file, and is changing the frame size based
  // on that.
  if (iSourceWidth == iOutputWidth && iSourceHeight == iOutputHeight)
    return fSourcePixelRatio;

  // mplayer is scaling in one or both directions.  We must alter our Source Pixel Ratio

  // First check our input data isn't going to cause problems such as divisions by zero
  // and the like.
  if (iSourceWidth == 0 || iSourceHeight == 0 || iOutputWidth == 0 || iOutputHeight == 0)
    return fSourcePixelRatio;
  
  // Calculate the ratio of the output frame 
  float fOutputFrameRatio = (float)iOutputWidth/iOutputHeight;

  // Calculate the ratio of the input frame
  float fSourceFrameRatio = (float)iSourceWidth/iSourceHeight;

  // Use output frame ratio and source frame ratio to determine the pixel ratio
  fSourcePixelRatio = fOutputFrameRatio / fSourceFrameRatio;

  // OK, most sources will be correct now, except those that are intended
  // to be displayed on non-square pixel based output devices (ie PAL or NTSC TVs)
  // This includes VCD, SVCD, and DVD (and possibly others that we are not doing yet)
  // For this, we can base the pixel ratio on the pixel ratios of PAL and NTSC,
  // though we will need to adjust for anamorphic sources (ie those whose
  // output frame ratio is not 4:3) and for SVCDs which have 2/3rds the
  // horizontal resolution of the default NTSC or PAL frame sizes

  // The following are the defined standard ratios for PAL and NTSC pixels
  float fPALPixelRatio = 59.0f/54.0f;
  float fNTSCPixelRatio = 10.0f/11.0f;

  // Calculate the correction needed for anamorphic sources
  float fNon4by3Correction = (float)fOutputFrameRatio/(4.0f/3.0f);

  // Now use the helper functions to check for a VCD, SVCD or DVD frame size
  if (CUtil::IsNTSC_VCD(iSourceWidth,iSourceHeight))
     fSourcePixelRatio = fNTSCPixelRatio;
  if (CUtil::IsNTSC_SVCD(iSourceWidth,iSourceHeight))
     fSourcePixelRatio = 3.0f/2.0f*fNTSCPixelRatio*fNon4by3Correction;
  if (CUtil::IsNTSC_DVD(iSourceWidth,iSourceHeight))
     fSourcePixelRatio = fNTSCPixelRatio*fNon4by3Correction;
  if (CUtil::IsPAL_VCD(iSourceWidth,iSourceHeight))
     fSourcePixelRatio = fPALPixelRatio;
  if (CUtil::IsPAL_SVCD(iSourceWidth,iSourceHeight))
     fSourcePixelRatio = 3.0f/2.0f*fPALPixelRatio*fNon4by3Correction;
  if (CUtil::IsPAL_DVD(iSourceWidth,iSourceHeight))
     fSourcePixelRatio = fPALPixelRatio*fNon4by3Correction;

  // Done, return the result
  return fSourcePixelRatio;
}

//********************************************************************************************************
static unsigned int Directx_ManageDisplay()
{
	float fOffsetX1 = (float)g_stSettings.m_rectMovieCalibration[m_iResolution].left;
	float fOffsetY1 = (float)g_stSettings.m_rectMovieCalibration[m_iResolution].top;
	float fOffsetX2 = (float)g_stSettings.m_rectMovieCalibration[m_iResolution].right;
	float fOffsetY2 = (float)g_stSettings.m_rectMovieCalibration[m_iResolution].bottom;

	float iScreenWidth =(float)m_iDeviceWidth  + fOffsetX2-fOffsetX1;
	float iScreenHeight=(float)m_iDeviceHeight + fOffsetY2-fOffsetY1;
	if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
	{
		const RECT& rv = g_graphicsContext.GetViewWindow();
		iScreenWidth = (float)rv.right-rv.left;
		iScreenHeight= (float)rv.bottom-rv.top;
		fOffsetX1    = (float)rv.left;
		fOffsetY1    = (float)rv.top;
		fOffsetX2    = 0.0f;
		fOffsetY2    = 0.0f;
	}

	//if( g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() )
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

				// calculate AR compensation (see http://www.mir.com/DMG/aspect.html)

				float fScreenPixelRatio = fScreenModePixelRatio[m_iResolution];
				if (m_bWidescreen && ((m_iResolution >= 3) || (m_iResolution <= 8)))
				      fScreenPixelRatio *= 4.0f/3.0f;

				// Calculate frame ratio based on source frame size and pixel ratio (Added by JM)
				float fSourcePixelRatio = GetSourcePixelRatio(image_width, image_height, d_image_width, d_image_height);
				float fOutputFrameRatio = (float)image_width/((float)image_height)*fSourcePixelRatio / fScreenPixelRatio; 
				float fNewWidth;
				float fNewHeight;
				float fHorzBorder=0;
				float fVertBorder=0;

				if ( image_width >= image_height)
				{	
					fNewHeight=(float)iScreenHeight;	  // 538
					fNewWidth = fNewHeight*fOutputFrameRatio; // 968.4
					fHorzBorder= (fNewWidth-(float)iScreenWidth)/2.0f;

					float fFactor = fNewWidth / ((float)image_width);
					fHorzBorder = fHorzBorder/fFactor;
				}
				else
				{
					fNewWidth  = (float)( iScreenWidth);
					fNewHeight = fNewWidth/fOutputFrameRatio;
					fVertBorder= (fNewHeight-(float)iScreenHeight)/2.0f;
					float fFactor = fNewWidth / ((float)image_width);
					fVertBorder = fVertBorder/fFactor;
				}
				if ( (int)fNewWidth < iScreenWidth )
				{
					fHorzBorder=0;
					fNewWidth  = (float)( iScreenWidth);
					fNewHeight = fNewWidth/fOutputFrameRatio;
					fVertBorder= (fNewHeight-(float)iScreenHeight)/2.0f;
					float fFactor = fNewWidth / ((float)image_width);
					fVertBorder = fVertBorder/fFactor;
				}
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

		// NORMAL
		// scale up image as much as possible
		// and keep the aspect ratio (introduces with black bars)

		float fScreenPixelRatio = fScreenModePixelRatio[m_iResolution];
		if (m_bWidescreen && ((m_iResolution >= 3) || (m_iResolution <= 8)))
		      fScreenPixelRatio *= 4.0f/3.0f;
		
	        // Calculate frame ratio based on source frame size and pixel ratio (Added by JM)
		float fSourcePixelRatio = GetSourcePixelRatio(image_width, image_height, d_image_width, d_image_height);
		float fOutputFrameRatio = (float)image_width/((float)image_height)*fSourcePixelRatio/fScreenPixelRatio; 

		// maximize the movie width
		float fNewWidth  = (float)(iScreenWidth);
		float fNewHeight = fNewWidth/fOutputFrameRatio;

		if (fNewHeight > iScreenHeight)
		{
			fNewHeight = (float)iScreenHeight;   // POSSIBLY CORRECT FOR SUBTITLE HEIGHT AS WELL
			fNewWidth = fNewHeight*fOutputFrameRatio;
		}

		// this shouldnt happen, but just make sure that everything still fits onscreen
		if (fNewWidth > iScreenWidth || fNewHeight > iScreenHeight)
		{
			fNewWidth=(float)image_width;
			fNewHeight=(float)image_height;
		}

		// source rect
		rs.left		= 0;
		rs.top    = 0;
		rs.right	= image_width;
		rs.bottom = image_height + iSubTitleHeight;


		// destination rect 
		float iPosY = iScreenHeight - fNewHeight;
		float iPosX = iScreenWidth  - fNewWidth	;
		iPosY /= 2; // center the movie
		iPosX /= 2; // center the movie
		rd.left   = (int)(iPosX + fOffsetX1 + 0.5f);
		rd.right  = (int)(rd.left + fNewWidth + 0.5f);
		rd.top    = (int)(iPosY + fOffsetY1 + 0.5f);
		rd.bottom = (int)(rd.top + fNewHeight + iSubTitleHeight + 0.5f);

		iSubTitlePos = image_height;
		bClearSubtitleRegion= true;

		if (iSubTitlePos  + 2*iSubTitleHeight >= m_iDeviceHeight - fOffsetY2)
		{
			bClearSubtitleRegion= false;
			iSubTitlePos = m_iDeviceHeight - (int)fOffsetY2 - iSubTitleHeight*2;
		}

		return 0;
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
	m_iResolution=0;
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

	Directx_ManageDisplay();
	if ( g_graphicsContext.IsFullScreenVideo() )
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


	bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
	if ( m_bFullScreen != bFullScreen )
	{
		m_bFullScreen= bFullScreen;
		g_graphicsContext.Lock();
		if (m_bFullScreen) 
		{
			choose_best_resolution(m_fFPS);
		}
		else 
		{
			restore_resolution();
		}
		g_graphicsContext.Unlock();
	}
}

void xbox_video_getRect(RECT& SrcRect, RECT& DestRect)
{
	SrcRect=rs;
	DestRect=rd;
}

float xbox_video_getAR()
{
	float fOutputPixelRatio = fScreenModePixelRatio[m_iResolution];
	float fOutputFrameRatio = ((float)(rd.right-rd.left)) / ((float)(rd.bottom-rd.top));
	return fOutputFrameRatio*fOutputPixelRatio;
}

void xbox_video_update()
{
	bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
	if ( m_bFullScreen != bFullScreen )
	{
		m_bFullScreen= bFullScreen;
		g_graphicsContext.Lock();
		if (m_bFullScreen) 
		{
			choose_best_resolution(m_fFPS);
		}
		else 
		{
			restore_resolution();
		}
		g_graphicsContext.Unlock();
	}
	Directx_ManageDisplay();
	if ( g_graphicsContext.IsFullScreenVideo() )
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

	OutputDebugString("video_config\n");
	fs = options & 0x01;
	image_format	 =  format;
	image_width		 = width;
	image_height	 = height;
	d_image_width  = d_width;
	d_image_height = d_height;
	m_fFPS = fps;

	m_bFullScreen=g_graphicsContext.IsFullScreenVideo();

	choose_best_resolution(m_fFPS);

	// We don't need mplayer to try and calculate an output size for us, as
	// mplayer knows nothing about non-square pixels.
	/*	aspect_save_orig(image_width,image_height);
	aspect_save_prescale(d_image_width,d_image_height);
	aspect_save_screenres( m_iDeviceWidth, m_iDeviceHeight );
	aspect(&d_image_width,&d_image_height,A_NOZOOM);*/

	fs=1;//fullscreen

	Directx_ManageDisplay();
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

