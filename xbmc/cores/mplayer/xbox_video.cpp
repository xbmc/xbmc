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

#include "PixelShaderRenderer.h"
#include "OverlayRenderer.h"
#include "ComboRenderer.h"
#include "RGBRenderer.h"

static void video_flip_page(void);
void video_uninit(void);

CXBoxRenderManager g_renderManager;

CXBoxRenderManager::CXBoxRenderManager()
{
	m_bChanging = false;
	m_pRenderer = NULL;
	m_bPauseDrawing = false;
}

CXBoxRenderManager::~CXBoxRenderManager()
{
	m_bChanging = true;
	if (m_pRenderer)
		delete m_pRenderer;
	m_pRenderer = NULL;
}

unsigned int CXBoxRenderManager::QueryFormat(unsigned int format)
{
	if (!m_bChanging && m_pRenderer)
		return m_pRenderer->QueryFormat(format);
	return 0;
}

// Functions called from mplayer
inline void CXBoxRenderManager::WaitForFlip()
{
	if (!m_bChanging && m_pRenderer)
		m_pRenderer->WaitForFlip();
}

unsigned int CXBoxRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, unsigned int options, char *title, unsigned int format)
{
	m_iSourceWidth = width;
	m_iSourceHeight = height;
	if (!m_bChanging && m_pRenderer)
		return m_pRenderer->Configure(width, height, d_width, d_height, options, title, format);
	return 0;
}
inline unsigned int CXBoxRenderManager::GetImage(mp_image_t *mpi)
{
	if (!m_bChanging && m_pRenderer)
		return m_pRenderer->GetImage(mpi);
	return VO_FALSE;
}
inline unsigned int CXBoxRenderManager::PutImage(mp_image_t *mpi)
{
	if (!m_bChanging && m_pRenderer)
		return m_pRenderer->PutImage(mpi);
	return VO_FALSE;
}

inline unsigned int CXBoxRenderManager::DrawFrame(unsigned char *src[])
{
	if (!m_bChanging && m_pRenderer)
		return m_pRenderer->DrawFrame(src);
	return 0;
}

inline unsigned int CXBoxRenderManager::DrawSlice(unsigned char *src[], int stride[], int w,int h,int x,int y)
{
	if (!m_bChanging && m_pRenderer)
		return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
	return 0;
}

inline void CXBoxRenderManager::FlipPage()
{
	if (m_bPauseDrawing) return;
	if (!m_bChanging && m_pRenderer)
		m_pRenderer->FlipPage();
}

unsigned int CXBoxRenderManager::PreInit(const char *arg)
{
  m_bPauseDrawing=false;
	if (!m_bChanging)
	{
		if (!m_pRenderer)
		{	// no renderer
			if (g_guiSettings.GetInt("Filters.RenderMethod") == RENDER_OVERLAYS)
			{
				m_pRenderer = new COverlayRenderer(g_graphicsContext.Get3DDevice());
			}
			else if (g_guiSettings.GetInt("Filters.RenderMethod") == RENDER_COMBO)
			{
				m_pRenderer = new CComboRenderer(g_graphicsContext.Get3DDevice());
			}
			else if (g_guiSettings.GetInt("Filters.RenderMethod") == RENDER_RGB_SHADER)
			{
				m_pRenderer = new CRGBRenderer(g_graphicsContext.Get3DDevice());
			}
			else
				m_pRenderer = new CPixelShaderRenderer(g_graphicsContext.Get3DDevice());
		}
		if (m_pRenderer)
			return m_pRenderer->PreInit(arg);
	}
	return 0;
}

void CXBoxRenderManager::UnInit()
{
	if (!m_bChanging && m_pRenderer)
	{
		m_pRenderer->UnInit();
		delete m_pRenderer;
		m_pRenderer = NULL;
	}
}

inline void CXBoxRenderManager::DrawAlpha(int x0, int y0, int w, int h, unsigned char *src,unsigned char *srca, int stride)
{
	if (m_bPauseDrawing) return;
	if (!m_bChanging && m_pRenderer)
		m_pRenderer->DrawAlpha(x0, y0, w, h, src, srca, stride);
}

void CXBoxRenderManager::SetupScreenshot()
{
	if (m_pRenderer)
		m_pRenderer->SetupScreenshot();
}

//********************************************************************************************************
// Functions below here are called from our app, rather than from mplayer directly
//********************************************************************************************************
void xbox_video_CheckScreenSaver()
{
	g_renderManager.CheckScreenSaver();
}

void xbox_video_getAR(float& fAR)
{
	fAR = g_renderManager.GetAR();
}

void xbox_video_getRect(RECT& SrcRect, RECT& DestRect)
{
	g_renderManager.GetRects(SrcRect, DestRect);
}

void xbox_video_wait()
{
	g_renderManager.WaitForFlip();
}

void xbox_video_render_update()
{
	g_renderManager.RenderUpdate();
}

void xbox_video_update(bool bPauseDrawing)
{
	g_renderManager.Update(bPauseDrawing);
}

/********************************************************************************************************
  mplayer calls below here
********************************************************************************************************/

//********************************************************************************************************
static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,unsigned char *srca, int stride)
{
	g_renderManager.DrawAlpha(x0, y0, w, h, src, srca, stride);
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
	if (g_renderManager.Paused()) return;
  vo_draw_text(g_renderManager.GetOSDWidth(), g_renderManager.GetOSDHeight(), draw_alpha);
}


//********************************************************************************************************
//      Uninit the whole system, this is on the same "level" as preinit.
static void video_uninit(void)
{
  OutputDebugString("video_uninit\n");  
	g_renderManager.UnInit();
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
	return g_renderManager.PreInit(arg);
}

/********************************************************************************************************
  draw_slice(): this displays YV12 pictures (3 planes, one full sized that
  contains brightness (Y), and 2 quarter-sized which the colour-info
  (U,V). MPEG codecs (libmpeg2, opendivx) use this. This doesn't have
  to display the whole frame, only update small parts of it.
*/
static unsigned int video_draw_slice(unsigned char *src[], int stride[], int w,int h,int x,int y )
{
	return g_renderManager.DrawSlice(src, stride, w, h, x, y);
}

//********************************************************************************************************
static void video_flip_page(void)
{
	g_renderManager.FlipPage();
}
/********************************************************************************************************
  draw_frame(): this is the older interface, this displays only complete
  frames, and can do only packed format (YUY2, RGB/BGR).
  Win32 codecs use this (DivX, Indeo, etc).
  If you implement VOCTRL_DRAW_IMAGE, you can left draw_frame.
*/
static unsigned int video_draw_frame(unsigned char *src[])
{
	return g_renderManager.DrawFrame(src);
}

/********************************************************************************************************
  Set up the video system. You get the dimensions and flags.
    width, height: size of the source image
    d_width, d_height: wanted scaled/display size (it's a hint)
    Flags:
      0x01  - force fullscreen (-fs)
      0x02  - allow mode switching (-vm)
      0x04  - allow software scaling (-zoom)
      0x08  - flipping (-flip)
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
  OutputDebugString("video_config()\n");
	return g_renderManager.Configure(width, height, d_width, d_height, options, title, format);
  OutputDebugString("video_config() done\n");
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
    return g_renderManager.GetImage((mp_image_t *)data);

/********************************************************************************************************
   VOCTRL_QUERY_FORMAT  -  queries if a given pixelformat is supported.
    It also returns various flags decsirbing the capabilities
    of the driver with teh given mode. for the flags, see file vfcaps.h !
    the most important flags, every driver must properly report
    these:
        0x1  -  supported (with or without conversion)
        0x2  -  supported without conversion (define 0x1 too!)
        0x100  -  driver/hardware handles timing (blocking)
    also SET sw/hw scaling and osd support flags, and flip,
    and accept_stride if you implement VOCTRL_DRAW_IMAGE (see bellow)
    NOTE: VOCTRL_QUERY_FORMAT may be called _before_ first config()
    but is always called between preinit() and uninit()
*/
    case VOCTRL_QUERY_FORMAT:
      return g_renderManager.QueryFormat(*((unsigned int*)data));

    case VOCTRL_DRAW_IMAGE:
    /*  replacement for the current draw_slice/draw_frame way of
        passing video frames. by implementing SET_IMAGE, you'll get
        image in mp_image struct instead of by calling draw_*.
        unless you return VO_TRUE for VOCTRL_DRAW_IMAGE call, the
        old-style draw_* functils will be called!
        Note: draw_slice is still mandatory, for per-slice rendering!
    */
      
      return g_renderManager.PutImage((mp_image_t *)data);
//      return VO_NOTIMPL;

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
    "XBOX Direct3D8 YUY2 renderer",
    "directx",
    "Frodo/JCMarshall",
    ""
};

//********************************************************************************************************
vo_functions_t video_functions =
{
    &video_info,
    video_preinit,			// done
    video_config,				// done
    video_control,
    video_draw_frame,		// done
    video_draw_slice,		// done
    video_draw_osd,
    video_flip_page,		// done
    video_check_events,	// unused
    video_uninit				// done
};