/*
* XBoxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
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
#include "stdafx.h"
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
#include "../../utils/log.h"
#include "../../XBVideoConfig.h"

#define OSD_TEXTURE_HEIGHT 150

#define NUM_BUFFERS (2)

struct DRAWRECT
{
	float left;
	float top;
	float right;
	float bottom;
};

static RECT                   rd;                             //rect of our stretched image
static RECT                   rs;                             //rect of our source image

static unsigned int           image_width, image_height;      //image width and height
static unsigned int           d_image_width, d_image_height;  //image width and height zoomed
static unsigned int           image_format=0;                 //image format
static unsigned int           primary_image_format;
static float                  fSourceFrameRatio=0;            //frame aspect ratio of video
static unsigned int           fs = 0;                         //display in window or fullscreen
static bool                   m_bPauseDrawing=false;          // whether we have paused drawing or not
static int                    m_bFlip=0;
static RESOLUTION             m_iResolution=PAL_4x3;
static bool                   m_bFlipped;
static float                  m_fps;
static __int64                m_lFrameCounter;

static LPDIRECT3DTEXTURE8     m_pOSDYTexture[2];
static LPDIRECT3DTEXTURE8     m_pOSDATexture[2];
static float									m_OSDWidth;
static float									m_OSDHeight;
static DRAWRECT								m_OSDRect;
static int										m_iOSDBuffer;
static bool                   m_OSDRendered;
//static bool										m_SubsOnOSD;
static int                    m_iOSDTextureWidth;
static int                    m_iOSDTextureHeight;

// YV12 decoder textures
static LPDIRECT3DTEXTURE8     m_YTexture[NUM_BUFFERS];
static LPDIRECT3DTEXTURE8     m_UTexture[NUM_BUFFERS];
static LPDIRECT3DTEXTURE8     m_VTexture[NUM_BUFFERS];
static DWORD                  m_hPixelShader = 0;
static int                    m_iYUVDecodeBuffer;
static int                    m_iYUVRenderBuffer;
static int                    m_bConfigured=false;
typedef struct directx_fourcc_caps
{
	char*             img_format_name;      //human readable name
	unsigned int      img_format;           //as MPlayer image format
	unsigned int      drv_caps;             //what hw supports with this format
} directx_fourcc_caps;

// we just support 1 format which is YV12
// TODO: add ARGB support

// 20-2-2004. 
// Removed the VFCAP_TIMER. (VFCAP_TIMER means that the video output driver handles all timing which we dont)
// this solves problems playing movies with no audio. With VFCAP_TIMER enabled they played much 2 fast
#define DIRECT3D8CAPS VFCAP_CSP_SUPPORTED_BY_HW |VFCAP_CSP_SUPPORTED |VFCAP_OSD |VFCAP_HWSCALE_UP|VFCAP_HWSCALE_DOWN|VFCAP_ACCEPT_STRIDE
static directx_fourcc_caps g_ddpf[] =
{
	{ "YV12",  IMGFMT_YV12,  DIRECT3D8CAPS},
//	{ "RGB32", IMGFMT_RGB32, DIRECT3D8CAPS},
//	{ "BGR32", IMGFMT_BGR32, DIRECT3D8CAPS},
};

static const DWORD FVF_YV12VERTEX = D3DFVF_XYZRHW|D3DFVF_TEX3;
static const DWORD FVF_RGBVERTEX  = D3DFVF_XYZRHW|D3DFVF_TEX1;
static const DWORD FVF_Y8A8VERTEX = D3DFVF_XYZRHW|D3DFVF_TEX2;

#define NUM_FORMATS (sizeof(g_ddpf) / sizeof(g_ddpf[0]))

static void video_flip_page(void);
void video_uninit(void);
//********************************************************************************************************
void choose_best_resolution(float fps)
{
	bool bUsingPAL        = g_videoConfig.HasPAL();    // current video standard:PAL or NTSC 
	bool bCanDoWidescreen = g_videoConfig.HasWidescreen(); // can widescreen be enabled?

	// Work out if the framerate suits PAL50 or PAL60
	bool bPal60=false;
	if (bUsingPAL && g_stSettings.m_bAllowPAL60 && g_videoConfig.HasPAL60())
	{
		// yes we're in PAL
		// yes PAL60 is allowed
		// yes dashboard PAL60 settings is enabled
		// Calculate the framerate difference from a divisor of 120fps and 100fps
		// (twice 60fps and 50fps to allow for 2:3 IVTC pulldown)
		float fFrameDifference60 = abs(120.0f/fps-floor(120.0f/fps+0.5f));
		float fFrameDifference50 = abs(100.0f/fps-floor(100.0f/fps+0.5f));
		// Make a decision based on the framerate difference
		if (fFrameDifference60 < fFrameDifference50)
			bPal60=true;
	}

	// Work out if framesize suits 4:3 or 16:9
	// Uses the frame aspect ratio of 8/(3*sqrt(3)) (=1.53960) which is the optimal point
	// where the percentage of black bars to screen area in 4:3 and 16:9 is equal
	bool bWidescreen = false;
	if (g_stSettings.m_bAutoWidescreenSwitching)
	{	// allowed to switch
		if (bCanDoWidescreen && (float)d_image_width/d_image_height > 8.0f/(3.0f*sqrt(3.0f)))
			bWidescreen = true;
		else
			bWidescreen = false;
	}
	else
	{	// user doesn't want us to switch - use the GUI setting
		bWidescreen = (g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].dwFlags&D3DPRESENTFLAG_WIDESCREEN)!=0;
	}

	// if we always upsample video to the GUI resolution then use it (with pal 60 if needed)
	// if we're not in fullscreen mode then use current resolution 
	// if we're calibrating the video  then use current resolution 
	if  ( g_stSettings.m_bUpsampleVideo ||
		(! ( g_graphicsContext.IsFullScreenVideo()|| g_graphicsContext.IsCalibrating())  )
		)
	{
		m_iResolution = g_graphicsContext.GetVideoResolution();
		// Check to see if we are using a PAL screen capable of PAL60
		if (bUsingPAL)
		{
			if (bPal60)
			{
				if (bWidescreen)
					m_iResolution = PAL60_16x9;
				else
					m_iResolution = PAL60_4x3;
			}
			else
			{
				if (bWidescreen)
					m_iResolution = PAL_16x9;
				else
					m_iResolution = PAL_4x3;
			}
		}
		else if (m_iResolution == NTSC_4x3 || m_iResolution == NTSC_16x9)
		{
			if (bWidescreen)
				m_iResolution = NTSC_16x9;
			else
				m_iResolution = NTSC_4x3;
		}
		else if (m_iResolution == HDTV_480p_4x3 || m_iResolution == HDTV_480p_16x9)
		{
			if (bWidescreen)
				m_iResolution = HDTV_480p_16x9;
			else
				m_iResolution = HDTV_480p_4x3;
		}
		// Change our screen resolution
		Sleep(1000);
		g_graphicsContext.SetVideoResolution(m_iResolution);
		return;
	}

	// We are allowed to switch video resolutions, so we must
	// now decide which is the best resolution for the video we have
	if (bUsingPAL)  // PAL resolutions
	{
		// Currently does not allow HDTV solutions, as it is my beleif
		// that the XBox hardware only allows HDTV resolutions for NTSC systems.
		// this may need revising as more knowledge is obtained.
		if (bPal60)
		{
			if (bWidescreen)
				m_iResolution = PAL60_16x9;
			else
				m_iResolution = PAL60_4x3;
		}
		else    // PAL50
		{
			if (bWidescreen)
				m_iResolution = PAL_16x9;
			else
				m_iResolution = PAL_4x3;
		}
	}
	else      // NTSC resolutions
	{
		// Check if the picture warrants HDTV mode
		// And if HDTV modes (1080i and 720p) are available
		if ((image_height>720 || image_width>1280) && g_videoConfig.Has1080i())
		{ //image suits 1080i if it's available
			m_iResolution = HDTV_1080i;
		}
		else if ((image_height>480 || image_width>720) && g_videoConfig.Has720p())                  //1080i is available
		{ // image suits 720p if it is available
			m_iResolution = HDTV_720p;
		}
		else if ((image_height>480 || image_width>720) && g_videoConfig.Has1080i())
		{ // image suits 720p and obviously 720p is unavailable
			m_iResolution = HDTV_1080i;
		}
		else  // either picture does not warrant HDTV, or HDTV modes are unavailable
		{
			if (g_videoConfig.Has480p())
			{
				if (bWidescreen)
					m_iResolution = HDTV_480p_16x9;
				else
					m_iResolution = HDTV_480p_4x3;
			}
			else
			{
				if (bWidescreen)
					m_iResolution = NTSC_16x9;
				else
					m_iResolution = NTSC_4x3;
			}
		}
	}

	// Finished - update our video resolution
    Sleep(1000);
	g_graphicsContext.SetVideoResolution(m_iResolution);
}

//********************************************************************************************************
static void Directx_CreateOverlay(unsigned int uiFormat)
{
	g_graphicsContext.Lock();

	for (int i = 0; i < NUM_BUFFERS; ++i)
	{
		// setup textures as linear luminance only textures, the pixel shader handles interleaving 
		if (m_YTexture[i])
			m_YTexture[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture(image_width, image_height, 1, 0, D3DFMT_LIN_L8, 0, &m_YTexture[i]);
		if (m_UTexture[i])
			m_UTexture[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture(image_width/2, image_height/2, 1, 0, D3DFMT_LIN_L8, 0, &m_UTexture[i]);
		if (m_VTexture[i])
			m_VTexture[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture(image_width/2, image_height/2, 1, 0, D3DFMT_LIN_L8, 0, &m_VTexture[i]);

		// RGB conversion buffer
//		if (m_RGBTexture[i])
//			m_RGBTexture[i]->Release();
//		g_graphicsContext.Get3DDevice()->CreateTexture(image_width, image_height, 1, 0, D3DFMT_LIN_X8R8G8B8, 0, &m_RGBTexture[i]);

		// blank the textures (just in case)
		D3DLOCKED_RECT lr;
		m_YTexture[i]->LockRect(0, &lr, NULL, 0);
		fast_memset(lr.pBits, 0, lr.Pitch*image_height);
		m_YTexture[i]->UnlockRect(0);

		m_UTexture[i]->LockRect(0, &lr, NULL, 0);
		fast_memset(lr.pBits, 128, lr.Pitch*(image_height/2));
		m_UTexture[i]->UnlockRect(0);

		m_VTexture[i]->LockRect(0, &lr, NULL, 0);
		fast_memset(lr.pBits, 128, lr.Pitch*(image_height/2));
		m_VTexture[i]->UnlockRect(0);

//		m_RGBTexture[i]->LockRect(0, &lr, NULL, 0);
//		fast_memset(lr.pBits, 0, lr.Pitch*image_height);
//		m_RGBTexture[i]->UnlockRect(0);
	}

	RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
	m_iOSDTextureWidth = g_settings.m_ResInfo[iRes].Overscan.right-g_settings.m_ResInfo[iRes].Overscan.left;
  m_iOSDTextureHeight = g_settings.m_ResInfo[iRes].Overscan.bottom-g_settings.m_ResInfo[iRes].Overscan.top;
	// Create osd textures
	for (int i = 0; i < 2; ++i)
	{
		if (m_pOSDYTexture[i])
			m_pOSDYTexture[i]->Release();
    g_graphicsContext.Get3DDevice()->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight, 1, 0, D3DFMT_LIN_L8, 0, &m_pOSDYTexture[i]);
		if (m_pOSDATexture[i])
			m_pOSDATexture[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight, 1, 0, D3DFMT_LIN_A8, 0, &m_pOSDATexture[i]);

		m_OSDWidth = m_OSDHeight = 0;
	}

	//xbox_video_update_subtitle_position();

	g_graphicsContext.Unlock();
}

//***************************************************************************************
// CalculateSourceFrameRatio()
//
// Considers the source frame size and output frame size (as suggested by mplayer)
// to determine if the pixels in the source are not square.  It calculates the aspect
// ratio of the output frame.  We consider the cases of VCD, SVCD and DVD separately,
// as these are intended to be viewed on a non-square pixel TV set, so the pixels are
// defined to be the same ratio as the intended display pixels.
// These formats are determined by frame size.
//***************************************************************************************
static void CalculateFrameAspectRatio()
{
	fSourceFrameRatio = (float)d_image_width / d_image_height;

	// Check whether mplayer has decided that the size of the video file should be changed
	// This indicates either a scaling has taken place (which we didn't ask for) or it has
	// found an aspect ratio parameter from the file, and is changing the frame size based
	// on that.
	if (image_width == d_image_width && image_height == d_image_height)
		return;

	// mplayer is scaling in one or both directions.  We must alter our Source Pixel Ratio
	float fImageFrameRatio = (float)image_width / image_height;

	// OK, most sources will be correct now, except those that are intended
	// to be displayed on non-square pixel based output devices (ie PAL or NTSC TVs)
	// This includes VCD, SVCD, and DVD (and possibly others that we are not doing yet)
	// For this, we can base the pixel ratio on the pixel ratios of PAL and NTSC,
	// though we will need to adjust for anamorphic sources (ie those whose
	// output frame ratio is not 4:3) and for SVCDs which have 2/3rds the
	// horizontal resolution of the default NTSC or PAL frame sizes

	// The following are the defined standard ratios for PAL and NTSC pixels
	float fPALPixelRatio = 128.0f/117.0f;
	float fNTSCPixelRatio = 72.0f/79.0f;

	// Calculate the correction needed for anamorphic sources
	float fNon4by3Correction = (float)fSourceFrameRatio/(4.0f/3.0f);

	// Now use the helper functions to check for a VCD, SVCD or DVD frame size
	if (CUtil::IsNTSC_VCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fNTSCPixelRatio;
	if (CUtil::IsNTSC_SVCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*3.0f/2.0f*fNTSCPixelRatio*fNon4by3Correction;
	if (CUtil::IsNTSC_DVD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fNTSCPixelRatio*fNon4by3Correction;
	if (CUtil::IsPAL_VCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fPALPixelRatio;
	if (CUtil::IsPAL_SVCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*3.0f/2.0f*fPALPixelRatio*fNon4by3Correction;
	if (CUtil::IsPAL_DVD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fPALPixelRatio*fNon4by3Correction;
}

void CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fUserPixelRatio, float fZoomAmount, RECT* displayRect)
{
  	// scale up image as much as possible
	// and keep the aspect ratio (introduces with black bars)
	// calculate the correct output frame ratio (using the users pixel ratio setting
	// and the output pixel ratio setting)

	float fOutputFrameRatio = fSourceFrameRatio*fUserPixelRatio / g_settings.m_ResInfo[m_iResolution].fPixelRatio; 

	// maximize the movie width
	float fNewWidth  = fScreenWidth;
	float fNewHeight = fNewWidth/fOutputFrameRatio;

	if (fNewHeight > fScreenHeight)
	{
		fNewHeight = fScreenHeight;
		fNewWidth = fNewHeight*fOutputFrameRatio;
	}

	// this shouldnt happen, but just make sure that everything still fits onscreen
	if (fNewWidth > fScreenWidth || fNewHeight > fScreenHeight)
	{
		fNewWidth=(float)image_width;
		fNewHeight=(float)image_height;
	}

	// Scale the movie up by set zoom amount
	fNewWidth *= fZoomAmount;
	fNewHeight *= fZoomAmount;

	// Centre the movie
	float fPosY = (fScreenHeight - fNewHeight)/2;
	float fPosX = (fScreenWidth  - fNewWidth)/2;

	displayRect->left   = (int)(fPosX + fOffsetX1);
	displayRect->right  = (int)(displayRect->left + fNewWidth + 0.5f);
	displayRect->top    = (int)(fPosY + fOffsetY1);
	displayRect->bottom = (int)(displayRect->top + fNewHeight + 0.5f);
}

//********************************************************************************************************
unsigned int Directx_ManageDisplay()
{
	RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
	float fOffsetX1 = (float)g_settings.m_ResInfo[iRes].Overscan.left;
	float fOffsetY1 = (float)g_settings.m_ResInfo[iRes].Overscan.top;
	float iScreenWidth = (float)(g_settings.m_ResInfo[iRes].Overscan.right-g_settings.m_ResInfo[iRes].Overscan.left);
	float iScreenHeight = (float)(g_settings.m_ResInfo[iRes].Overscan.bottom-g_settings.m_ResInfo[iRes].Overscan.top);

	if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
	{
		const RECT& rv = g_graphicsContext.GetViewWindow();
		iScreenWidth = (float)rv.right-rv.left;
		iScreenHeight= (float)rv.bottom-rv.top;
		fOffsetX1    = (float)rv.left;
		fOffsetY1    = (float)rv.top;
	}

	// source rect
	rs.left   = 0;
	rs.top    = 0;
	rs.right  = image_width;
	rs.bottom = image_height;

	CalcNormalDisplayRect(fOffsetX1, fOffsetY1, iScreenWidth, iScreenHeight, g_stSettings.m_fPixelRatio, g_stSettings.m_fZoomAmount, &rd);

	return 0;
}

//***********************************************************************************************************
static void vo_draw_alpha_xbox(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta,int dststride)
{
  if (m_bPauseDrawing)
		return;

	for(int y = 0; y < h; ++y)
	{
		fast_memcpy(dst, src, w);
		fast_memcpy(dsta, srca, w);
		src+=srcstride;
		srca+=srcstride;
		dst+=dststride;
		dsta+=dststride;
	}
}

//********************************************************************************************************
static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,unsigned char *srca, int stride)
{
  if (m_bPauseDrawing)
		return;
  
  // OSD is drawn after draw_slice / put_image
  // this means that the buffer has already been handed off to the RGB converter
	// solution: have separate OSD textures

	// if it's down the bottom, use sub alpha blending
//  m_SubsOnOSD = (y0 > (int)(rs.bottom - rs.top) * 4 / 5);

  //use temporary rect for calculation to avoid messing with module-rect while other functions might be using it.
  DRAWRECT osdRect;
	// scale to fit screen
  float EnlargeFactor = 1.0f + (g_stSettings.m_iEnlargeSubtitlePercent / 100.0f);

	if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
	{
		const RECT& rv = g_graphicsContext.GetViewWindow();
		float xscale = EnlargeFactor*(float)(rv.right - rv.left)/m_iOSDTextureWidth;
		float yscale = xscale * g_settings.m_ResInfo[m_iResolution].fPixelRatio;
		osdRect.left = (float)rv.left + (float)(rv.right - rv.left - (float)w*xscale)/2.0f;
		osdRect.right  = m_OSDRect.left + (float)w * xscale;
		float relbottom = ((float)(g_settings.m_ResInfo[m_iResolution].iSubtitles-g_settings.m_ResInfo[m_iResolution].Overscan.top))/(g_settings.m_ResInfo[m_iResolution].Overscan.bottom - g_settings.m_ResInfo[m_iResolution].Overscan.top);
		osdRect.bottom = (float)rv.top + (float)(rv.bottom-rv.top)*relbottom;
		osdRect.top    = osdRect.bottom - (float)h * yscale;
	}
	else
	{
		float xscale = EnlargeFactor;
		float yscale = xscale * g_settings.m_ResInfo[m_iResolution].fPixelRatio;

		osdRect.left   = (float)g_settings.m_ResInfo[m_iResolution].Overscan.left + (float)(g_settings.m_ResInfo[m_iResolution].Overscan.right-g_settings.m_ResInfo[m_iResolution].Overscan.left - (float)w*xscale)/2.0f;
		osdRect.right  = osdRect.left + (float)w * xscale;
		osdRect.bottom = (float)g_settings.m_ResInfo[m_iResolution].iSubtitles;
		osdRect.top    = osdRect.bottom - (float)h * yscale;
	}

	// clip to buffer
	if (w > m_iOSDTextureWidth) w = m_iOSDTextureWidth;
	if (h > m_iOSDTextureHeight) h = m_iOSDTextureHeight;

	RECT rc = { 0, 0, w, h };

	// flip buffers and wait for gpu
	int iOSDBuffer = 1-m_iOSDBuffer;
	while (m_pOSDYTexture[iOSDBuffer]->IsBusy()) Sleep(1);
	while (m_pOSDATexture[iOSDBuffer]->IsBusy()) Sleep(1);

  // draw textures
  D3DLOCKED_RECT lr, lra;
  if (
    (D3D_OK == m_pOSDYTexture[iOSDBuffer]->LockRect(0, &lr, &rc, 0)) &&
    (D3D_OK == m_pOSDATexture[iOSDBuffer]->LockRect(0, &lra, &rc, 0))
  ) {
    //clear the textures
    fast_memset(lr.pBits, 0, lr.Pitch*m_iOSDTextureHeight);
    fast_memset(lra.pBits, 0, lra.Pitch*m_iOSDTextureHeight);
    //draw the osd/subs
    vo_draw_alpha_xbox(w,h,src,srca,stride, (BYTE*)lr.pBits, (BYTE*)lra.pBits, lr.Pitch);
  }
  m_pOSDYTexture[iOSDBuffer]->UnlockRect(0);
  m_pOSDATexture[iOSDBuffer]->UnlockRect(0);

  //set module variables to calculated values
  m_iOSDBuffer = iOSDBuffer;
  m_OSDRect    = osdRect;
  m_OSDWidth   = (float)w;
  m_OSDHeight  = (float)h;

  m_OSDRendered = true;
}

//********************************************************************************************************
static void video_draw_osd(void)
{
  if (m_bPauseDrawing) return;
	
	vo_draw_text((int)m_iOSDTextureWidth, (int)((float)m_iOSDTextureWidth/fSourceFrameRatio), draw_alpha);
}

//********************************************************************************************************
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
void video_uninit(void)
{
	OutputDebugString("video_uninit\n");  

	for (int i=0; i < NUM_BUFFERS; i++)
	{
		if (m_YTexture[i])
			m_YTexture[i]->Release();
		m_YTexture[i] = NULL;
 		if (m_UTexture[i])
			m_UTexture[i]->Release();
		m_UTexture[i] = NULL;
		if (m_VTexture[i])
			m_VTexture[i]->Release();
		m_VTexture[i] = NULL;

//		if (m_RGBTexture[i])
//			m_RGBTexture[i]->Release();
//		m_RGBTexture[i] = NULL;
	}
	// subtitle and osd stuff
	for (int i = 0; i < 2; ++i)
	{
		if (m_pOSDYTexture[i]) m_pOSDYTexture[i]->Release();
		m_pOSDYTexture[i]=NULL;
		if (m_pOSDATexture[i]) m_pOSDATexture[i]->Release();
		m_pOSDATexture[i]=NULL;
	}
	OutputDebugString("video_uninit done\n");
}

//***********************************************************************************************************
static void video_check_events(void)
{
}

//***********************************************************************************************************
static unsigned int video_preinit(const char *arg)
{
  video_uninit();
	m_iResolution=PAL_4x3;
	m_bPauseDrawing=false;
//	for (int i=0; i<NUM_BUFFERS; i++) iClearSubtitleRegion[i] = 0;
	m_iOSDBuffer = 0;
//	m_iRGBRenderBuffer = -1;
//	m_iRGBDecodeBuffer =
	m_iYUVDecodeBuffer = m_iYUVRenderBuffer = 0;
	m_bFlip=0;
	fs=1;
	m_bConfigured=false;
	// Create the pixel shader
	if (!m_hPixelShader)
	{
		//D3DPIXELSHADERDEF* ShaderBuffer = (D3DPIXELSHADERDEF*)((char*)XLoadSection("PXLSHADER") + 4);

		// shader to interleave separate Y U and V planes into a single YUV output
		const char* shader =
			"xps.1.1\n"
			"def c0,1,0,0,0\n"
			"def c1,0,1,0,0\n"
			"def c2,0,0,1,0\n"
			"tex t0\n"
			"tex t1\n"
			"tex t2\n"
			"xmma discard,discard,r0, t0,c0, t1,c1\n"
			"mad r0, t2,c2,r0\n";

		XGBuffer* pShader;
		XGAssembleShader("XBMCShader", shader, strlen(shader), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
		g_graphicsContext.Get3DDevice()->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hPixelShader);
		pShader->Release();
	}

	return 0;
}

//********************************************************************************************************
/*
static void YV12ToRGB()
{
	g_graphicsContext.Lock();

	while (m_RGBTexture[m_iRGBDecodeBuffer]->IsBusy())
	{
		if (m_RGBTexture[m_iRGBDecodeBuffer]->Lock)
			g_graphicsContext.Get3DDevice()->BlockOnFence(m_RGBTexture[m_iRGBDecodeBuffer]->Lock);
		else
			Sleep(1);
	}

	g_graphicsContext.Get3DDevice()->SetTexture( 0, m_YTexture[m_iYUVDecodeBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 1, m_UTexture[m_iYUVDecodeBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 2, m_VTexture[m_iYUVDecodeBuffer]);
	for (int i = 0; i < 3; ++i)
	{
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MAGFILTER,  D3DTEXF_POINT );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MINFILTER,  D3DTEXF_POINT );
	}

	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );

	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_YV12VERTEX );
	g_graphicsContext.Get3DDevice()->SetPixelShader(m_hPixelShader);

	LPDIRECT3DSURFACE8 pOldRT, pNewRT;
	m_RGBTexture[m_iRGBDecodeBuffer]->GetSurfaceLevel(0, &pNewRT);
	g_graphicsContext.Get3DDevice()->GetRenderTarget(&pOldRT);
	g_graphicsContext.Get3DDevice()->SetRenderTarget(pNewRT, NULL);

	// Render the image
	float w = float(image_width / 2);
	float h = float(image_height / 2);

	g_graphicsContext.Get3DDevice()->SetScreenSpaceOffset(-0.5f, -0.5f); // fix texel align

	g_graphicsContext.Get3DDevice()->Begin(D3DPT_QUADLIST);
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, 0, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, 0, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, 0, 0, 0, 1.0f );

	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)image_width, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, w, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, w, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, (float)image_width, 0, 0, 1.0f );

	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)image_width, (float)image_height );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, w, h );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, w, h );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, (float)image_width, (float)image_height, 0, 1.0f );

	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, (float)image_height );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, 0, h );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, 0, h );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, 0, (float)image_height, 0, 1.0f );
	g_graphicsContext.Get3DDevice()->End();

	g_graphicsContext.Get3DDevice()->SetScreenSpaceOffset(0, 0);

	g_graphicsContext.Get3DDevice()->SetPixelShader(NULL);
	g_graphicsContext.Get3DDevice()->SetVertexShader(0);
	g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture(1, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture(2, NULL);
	g_graphicsContext.Get3DDevice()->SetRenderTarget(pOldRT, NULL);

	g_graphicsContext.Get3DDevice()->KickPushBuffer();

	pOldRT->Release();
	pNewRT->Release();

	++m_iRGBDecodeBuffer %= NUM_BUFFERS;

	for (int i = 0; i < 3; ++i)
	{
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MAGFILTER, g_stSettings.m_maxFilter );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MINFILTER, g_stSettings.m_minFilter );
	}

	g_graphicsContext.Unlock();
}
*/
static unsigned int video_draw_slice(unsigned char *src[], int stride[], int w,int h,int x,int y )
{
  BYTE *s;
  BYTE *d;
  int i=0;
  int iBottom=y+h;
  int iOrgY=y;
  int iOrgH=h;

	g_graphicsContext.Lock();

	while (m_YTexture[m_iYUVDecodeBuffer]->IsBusy())
	{
		if (m_YTexture[m_iYUVDecodeBuffer]->Lock)
			g_graphicsContext.Get3DDevice()->BlockOnFence(m_YTexture[m_iYUVDecodeBuffer]->Lock);
		else
			Sleep(1);
	}
	while (m_UTexture[m_iYUVDecodeBuffer]->IsBusy())
	{
		if (m_UTexture[m_iYUVDecodeBuffer]->Lock)
			g_graphicsContext.Get3DDevice()->BlockOnFence(m_UTexture[m_iYUVDecodeBuffer]->Lock);
		else
			Sleep(1);
	}
	while (m_VTexture[m_iYUVDecodeBuffer]->IsBusy())
	{
		if (m_VTexture[m_iYUVDecodeBuffer]->Lock)
			g_graphicsContext.Get3DDevice()->BlockOnFence(m_VTexture[m_iYUVDecodeBuffer]->Lock);
		else
			Sleep(1);
	}

	D3DLOCKED_RECT lr;

  // copy Y
	m_YTexture[m_iYUVDecodeBuffer]->LockRect(0, &lr, NULL, 0);
  d=(BYTE*)lr.pBits + lr.Pitch*y + x;
  s=src[0];
  for(i=0;i<h;i++){
    fast_memcpy(d,s,w);
    s+=stride[0];
    d+=lr.Pitch;
  }
	m_YTexture[m_iYUVDecodeBuffer]->UnlockRect(0);
    
	w/=2;h/=2;x/=2;y/=2;
	
	// copy U
	m_UTexture[m_iYUVDecodeBuffer]->LockRect(0, &lr, NULL, 0);
  d=(BYTE*)lr.pBits + lr.Pitch*y + x;
  s=src[1];
  for(i=0;i<h;i++){
    fast_memcpy(d,s,w);
    s+=stride[1];
    d+=lr.Pitch;
  }
	m_UTexture[m_iYUVDecodeBuffer]->UnlockRect(0);

	// copy V
	m_VTexture[m_iYUVDecodeBuffer]->LockRect(0, &lr, NULL, 0);
  d=(BYTE*)lr.pBits + lr.Pitch*y + x;
  s=src[2];
  for(i=0;i<h;i++){
    fast_memcpy(d,s,w);
    s+=stride[2];
    d+=lr.Pitch;
  }
	m_VTexture[m_iYUVDecodeBuffer]->UnlockRect(0);

  if (iBottom+1>=(int)image_height)
  {
		// frame finished, output buffer to screen
		//YV12ToRGB();
    ++m_iYUVDecodeBuffer %= NUM_BUFFERS;
    m_bFlip++;
  }

	g_graphicsContext.Unlock();

  return 0;
}

//********************************************************************************************************

void Setup_Y8A8Render()
{
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAKILL, D3DTALPHAKILL_ENABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );

	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER,  g_stSettings.m_maxFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER,  g_stSettings.m_minFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_MAGFILTER,  g_stSettings.m_maxFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_MINFILTER,  g_stSettings.m_minFilter );


	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_Y8A8VERTEX );
}

//void xbox_video_render_subtitles()
//{
//	int iTexture = 1-m_iBackBuffer;
//  
//	if (!m_pSubtitleYTexture[iTexture] || !m_pSubtitleATexture[iTexture])
//		return;
//
//	// Set state to render the image
//	g_graphicsContext.Get3DDevice()->SetTexture(0, m_pSubtitleYTexture[iTexture]);
//	g_graphicsContext.Get3DDevice()->SetTexture(1, m_pSubtitleATexture[iTexture]);
//	Setup_Y8A8Render();
//	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
//	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_INVSRCALPHA );
//	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
//
//	// Render the image
//	g_graphicsContext.Get3DDevice()->Begin(D3DPT_QUADLIST);
//  g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, 0.0f );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 0.0f );
//	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, m_SubRect.left, m_SubRect.top, 0, 0 );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, SUBTITLE_TEXTURE_WIDTH, 0.0f );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, SUBTITLE_TEXTURE_WIDTH, 0.0f );
//	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, m_SubRect.right, m_SubRect.top, 0, 0 );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, SUBTITLE_TEXTURE_WIDTH, SUBTITLE_TEXTURE_HEIGHT );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, SUBTITLE_TEXTURE_WIDTH, SUBTITLE_TEXTURE_HEIGHT );
//	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, m_SubRect.right, m_SubRect.bottom, 0, 0 );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.0f, SUBTITLE_TEXTURE_HEIGHT );
//	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, SUBTITLE_TEXTURE_HEIGHT );
//	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, m_SubRect.left, m_SubRect.bottom, 0, 0 );
//	g_graphicsContext.Get3DDevice()->End();
//
//	g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
//	g_graphicsContext.Get3DDevice()->SetTexture(1, NULL);
//
//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE );
//}
//********************************************************************************************************

void xbox_video_render_osd()
{
	if (!m_pOSDYTexture[m_iOSDBuffer] || !m_pOSDATexture[m_iOSDBuffer])
		return;
	if (!m_OSDWidth || !m_OSDHeight)
		return;

  //copy alle static vars to local vars because they might change during this function by mplayer callbacks
  int      buffer    = m_iOSDBuffer;
  float    osdWidth  = m_OSDWidth;
  float    osdHeight = m_OSDHeight;
  DRAWRECT osdRect   = m_OSDRect;
//  if (!viewportRect.bottom && !viewportRect.right) 
//    return;

	// Set state to render the image
	g_graphicsContext.Get3DDevice()->SetTexture(0, m_pOSDYTexture[buffer]);
	g_graphicsContext.Get3DDevice()->SetTexture(1, m_pOSDATexture[buffer]);
	Setup_Y8A8Render();

	/* In mplayer's alpha planes, 0 is transparent, then 1 is nearly
	opaque upto 255 which is transparent */
	// means do alphakill + inverse alphablend
  /*
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	if (m_SubsOnOSD)
	{
		// subs use mplayer style alpha
		//g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_INVSRCALPHA );
		//g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );

			// Note the mplayer code actually does this
		//g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
		//g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
	}
	else
	{
		// OSD looks better with src+(1-a)*dst
		g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
		g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}
  */
  

	// Set texture filters
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER,  g_stSettings.m_maxFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER,  g_stSettings.m_minFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_MAGFILTER,  g_stSettings.m_maxFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_MINFILTER,  g_stSettings.m_minFilter );

	// clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
	if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
	{
		RECT rv = g_graphicsContext.GetViewWindow();
		D3DRECT clip = { rv.left, rv.top, rv.right, rv.bottom };
		g_graphicsContext.Get3DDevice()->SetScissors(1, FALSE, &clip);
	}

	// Render the image
	g_graphicsContext.Get3DDevice()->Begin(D3DPT_QUADLIST);
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, 0, 0 );
  g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, osdRect.left, osdRect.top, 0, 1.0f );
  g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, osdWidth, 0 );
  g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, osdWidth, 0 );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, osdRect.right, osdRect.top, 0, 1.0f );
  g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, osdWidth, osdHeight );
  g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, osdWidth, osdHeight );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, osdRect.right, osdRect.bottom, 0, 1.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, osdHeight );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, 0, osdHeight );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, osdRect.left, osdRect.bottom, 0, 1.0f );
	g_graphicsContext.Get3DDevice()->End();

	g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture(1, NULL);
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE );

	g_graphicsContext.Get3DDevice()->SetScissors(0, FALSE, NULL);
}

void xbox_video_render()
{
  //if (m_iRGBRenderBuffer<0 || m_iRGBRenderBuffer >= NUM_BUFFERS) return;
	if (m_iYUVRenderBuffer<0 || m_iYUVRenderBuffer >= NUM_BUFFERS) return;

  //always render the buffer that is not currently being decoded to.
  m_iYUVRenderBuffer = ((m_iYUVDecodeBuffer + 1) % NUM_BUFFERS);
  //g_graphicsContext.Get3DDevice()->SetTexture( 0, m_RGBTexture[m_iRGBRenderBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 0, m_YTexture[m_iYUVRenderBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 1, m_UTexture[m_iYUVRenderBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 2, m_VTexture[m_iYUVRenderBuffer]);

//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
//	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	for (int i = 0; i < 3; ++i)
	{
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MAGFILTER,  g_stSettings.m_maxFilter );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MINFILTER,  g_stSettings.m_minFilter );
	}

	// set scissors if we are not in fullscreen video
	if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
	{
		RECT rv = g_graphicsContext.GetViewWindow();
		D3DRECT clip = { rv.left, rv.top, rv.right, rv.bottom };
		g_graphicsContext.Get3DDevice()->SetScissors(1, FALSE, &clip);
	}

	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, TRUE );
	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_YV12VERTEX );
	g_graphicsContext.Get3DDevice()->SetPixelShader( m_hPixelShader );
	// Render the image
	g_graphicsContext.Get3DDevice()->Begin(D3DPT_QUADLIST);
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.top );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.left/2.0f, (float)rs.top/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.left/2.0f, (float)rs.top/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.top, 0, 1.0f );

	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.top );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.right/2.0f, (float)rs.top/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.right/2.0f, (float)rs.top/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.top, 0, 1.0f );

	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.bottom );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.right/2.0f, (float)rs.bottom/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.right/2.0f, (float)rs.bottom/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.bottom, 0, 1.0f );

	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.bottom );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.left/2.0f, (float)rs.bottom/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.left/2.0f, (float)rs.bottom/2.0f );
	g_graphicsContext.Get3DDevice()->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.bottom, 0, 1.0f );
	g_graphicsContext.Get3DDevice()->End();

	g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture(1, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture(2, NULL);

	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, FALSE );
	g_graphicsContext.Get3DDevice()->SetPixelShader( NULL );
	g_graphicsContext.Get3DDevice()->SetScissors(0, FALSE, NULL );

	xbox_video_render_osd();
}
//************************************************************************************
static void video_flip_page(void)
{
  if (m_bPauseDrawing) return;

	if (m_bFlip<=0)
  {
    return;
  }
	m_bFlip--;
  //++m_iRGBRenderBuffer %= NUM_BUFFERS;
	//++m_iYUVRenderBuffer %= NUM_BUFFERS;
//	m_pSubtitleYTexture[m_iBackBuffer]->UnlockRect(0);
//	m_pSubtitleATexture[m_iBackBuffer]->UnlockRect(0);
//	m_iBackBuffer = 1-m_iBackBuffer;

	if (g_graphicsContext.IsFullScreenVideo())
	{
	  Directx_ManageDisplay();

	  g_graphicsContext.Lock();
		g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

		// render first so the subtitle overlay works
		xbox_video_render();

		if (g_application.NeedRenderFullScreen())
			g_application.RenderFullScreen();

//	  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();      
		g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
  	g_graphicsContext.Unlock();
	}
  //if no osd was rendered before this call to flip_page make sure it doesn't get 
  //rendered the next call to render not initialized by mplayer (like previewwindow + calibrationwindow);
  if (!m_OSDRendered)
  {
	  m_OSDWidth = m_OSDHeight = 0;
  }
  m_OSDRendered = false;
	m_bFlipped    = true;
/*
	// when movie is running,
	// check the FPS again after 50 frames
	// after 50 frames mplayer has determined the REAL fps instead of just the one mentioned in the header
	// if its different we might need pal60
	m_lFrameCounter++;
	if (m_lFrameCounter==50)
	{
		char strFourCC[12];
		char strVideoCodec[256];
		unsigned int iWidth,iHeight;
		long tooearly, toolate;
		float fps;
		mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fps, &iWidth,&iHeight, &tooearly, &toolate);
		if (fps != m_fps)
		{
			choose_best_resolution(fps);
		}
	}
*/
}

//********************************************************************************************************
void xbox_video_wait()
{
	int iTmp=0;
	m_bFlipped=false;
	while (!m_bFlipped && iTmp < 3000)
	{
		Sleep(1);
		iTmp++;
	}
}
//********************************************************************************************************
void xbox_video_getRect(RECT& SrcRect, RECT& DestRect)
{
	SrcRect=rs;
	DestRect=rd;
}

//********************************************************************************************************
void xbox_video_CheckScreenSaver()
{
	// Called from CMPlayer::Process() (mplayer.cpp) when in 'pause' mode

	return;
}

//********************************************************************************************************
void xbox_video_getAR(float& fAR)
{
	fAR = fSourceFrameRatio;
}

//********************************************************************************************************
void xbox_video_render_update()
{
  //if (m_iRGBRenderBuffer < 0 || m_iRGBRenderBuffer >=NUM_BUFFERS) return;
	if (m_iYUVRenderBuffer < 0 || m_iYUVRenderBuffer >=NUM_BUFFERS) return;
  if (!m_YTexture[0])return;

  Directx_ManageDisplay();
	g_graphicsContext.Lock();
	xbox_video_render();
//	if ( bFullScreen )
//	{
//		xbox_video_update_subtitle_position();
//		xbox_video_render_subtitles();
//	}
	g_graphicsContext.Unlock();
}


//********************************************************************************************************
void xbox_video_update(bool bPauseDrawing)
{
  m_bPauseDrawing = bPauseDrawing;
	if(m_bConfigured) //Only do this if we have been inited.
	{
		bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
		g_graphicsContext.Lock();
		g_graphicsContext.SetVideoResolution(bFullScreen ? m_iResolution:g_stSettings.m_GUIResolution);
		g_graphicsContext.Unlock();
		Directx_ManageDisplay();
		xbox_video_render_update();
	}
}


//********************************************************************************************************
static unsigned int video_draw_frame(unsigned char *src[])
{
	// no support
	return 1;
}

//********************************************************************************************************
static unsigned int get_image(mp_image_t *mpi)
{
	return VO_FALSE;
}
//********************************************************************************************************
static unsigned int put_image(mp_image_t *mpi)
{
	return VO_FALSE;
	if((mpi->flags&MP_IMGFLAG_DIRECT) || (mpi->flags&MP_IMGFLAG_DRAW_CALLBACK))
	{
		return VO_FALSE;
	}
  int  i = 0;
  BYTE   *d;
  BYTE   *s;
  int x = mpi->x;
  int y = mpi->y;
  int w = mpi->w;
  int h = mpi->h;

	if (mpi->flags&MP_IMGFLAG_PLANAR)
	{
		g_graphicsContext.Lock();

		while (m_YTexture[m_iYUVDecodeBuffer]->IsBusy())
		{
			if (m_YTexture[m_iYUVDecodeBuffer]->Lock)
				g_graphicsContext.Get3DDevice()->BlockOnFence(m_YTexture[m_iYUVDecodeBuffer]->Lock);
			else
				Sleep(1);
		}
		while (m_UTexture[m_iYUVDecodeBuffer]->IsBusy())
		{
			if (m_UTexture[m_iYUVDecodeBuffer]->Lock)
				g_graphicsContext.Get3DDevice()->BlockOnFence(m_UTexture[m_iYUVDecodeBuffer]->Lock);
			else
				Sleep(1);
		}
		while (m_VTexture[m_iYUVDecodeBuffer]->IsBusy())
		{
			if (m_VTexture[m_iYUVDecodeBuffer]->Lock)
				g_graphicsContext.Get3DDevice()->BlockOnFence(m_VTexture[m_iYUVDecodeBuffer]->Lock);
			else
				Sleep(1);
		}

		D3DLOCKED_RECT lr;

		m_YTexture[m_iYUVDecodeBuffer]->LockRect(0, &lr, NULL, 0);
		d=(BYTE*)lr.pBits + lr.Pitch*y + x;
    s=mpi->planes[0];
    for(i=0;i<h;i++)
    {
      fast_memcpy(d,s,w);
      s+=mpi->stride[0];
      d+=lr.Pitch;
    }
		m_YTexture[m_iYUVDecodeBuffer]->UnlockRect(0);

    w/=2;h/=2;x/=2;y/=2;
    
		m_UTexture[m_iYUVDecodeBuffer]->LockRect(0, &lr, NULL, 0);
		d=(BYTE*)lr.pBits + lr.Pitch*y + x;
    s=mpi->planes[1];
    for(i=0;i<h;i++){
      fast_memcpy(d,s,w);
      s+=mpi->stride[1];
      d+=lr.Pitch;
    }
		m_UTexture[m_iYUVDecodeBuffer]->UnlockRect(0);

		m_VTexture[m_iYUVDecodeBuffer]->LockRect(0, &lr, NULL, 0);
		d=(BYTE*)lr.pBits + lr.Pitch*y + x;
    s=mpi->planes[2];
    for(i=0;i<h;i++){
      fast_memcpy(d,s,w);
      s+=mpi->stride[2];
      d+=lr.Pitch;
    }
		m_VTexture[m_iYUVDecodeBuffer]->UnlockRect(0);
	}
	else
	{
		return VO_FALSE; // no packed support
	}

	// frame finished, send buffer to screen
	//YV12ToRGB();
	++m_iYUVDecodeBuffer %= NUM_BUFFERS;
	m_bFlip++;

	g_graphicsContext.Unlock();

	return VO_TRUE;
}

//********************************************************************************************************
static unsigned int video_config(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, unsigned int options, char *title, unsigned int format)
{
	char strFourCC[12];
	char strVideoCodec[256];

	unsigned int iWidth,iHeight;
	long tooearly, toolate;

	m_lFrameCounter=0;
	mplayer_GetVideoInfo(strFourCC,strVideoCodec, &m_fps, &iWidth,&iHeight, &tooearly, &toolate);
	g_graphicsContext.SetFullScreenVideo(true);
	OutputDebugString("video_config\n");
	fs = options & 0x01;
	image_format   =  format;
	image_width    = width;
	image_height   = height;
	d_image_width  = d_width;
	d_image_height = d_height;

	// calculate the input frame aspect ratio
	CalculateFrameAspectRatio();
	choose_best_resolution(m_fps);

	fs=1;//fullscreen

	Directx_ManageDisplay();
	Directx_CreateOverlay(image_format);
	m_bConfigured=true;
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
		return get_image((mp_image_t *)data);

	case VOCTRL_QUERY_FORMAT:
		return query_format(*((unsigned int*)data));

	case VOCTRL_DRAW_IMAGE:
		/*  replacement for the current draw_slice/draw_frame way of
		passing video frames. by implementing SET_IMAGE, you'll get
		image in mp_image struct instead of by calling draw_*.
		unless you return VO_TRUE for VOCTRL_DRAW_IMAGE call, the
		old-style draw_* functils will be called!
		Note: draw_slice is still mandatory, for per-slice rendering!
		*/

		return put_image( (mp_image_t*)data );
		//return VO_FALSE;//NOTIMPL;

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
	"XBOX Direct3D8 YV12 renderer",
	"directx",
	"Frodo/JCMarshall/Butcher",
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
