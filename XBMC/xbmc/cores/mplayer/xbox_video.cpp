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

#define SUBTITLE_TEXTURE_WIDTH  720
#define SUBTITLE_TEXTURE_HEIGHT 120

static RECT             rd;                             //rect of our stretched image
static RECT             rs;                             //rect of our source image
static unsigned int     image_width, image_height;      //image width and height
static unsigned int     d_image_width, d_image_height;  //image width and height zoomed
static unsigned char*   image=NULL;                     //image data
static unsigned int     image_format=0;                 //image format
static unsigned int     primary_image_format;
static float            fSourceFrameRatio=0;            //frame aspect ratio of video
static unsigned int     fs = 0;                         //display in window or fullscreen
static unsigned int     dstride;                        //surface stride
bool                    m_bPauseDrawing=false;          // whether we have paused drawing or not
int                     iClearSubtitleRegion[2]={0,0};  // amount of subtitle region to clear
LPDIRECT3DTEXTURE8      m_pSubtitleTexture[2]={NULL,NULL};
static unsigned char*   subtitleimage=NULL;             //image data
static unsigned int     subtitlestride;                 //surface stride
LPDIRECT3DVERTEXBUFFER8 m_pSubtitleVB;                  // vertex buffer for subtitles
int                     m_iBackBuffer=0;
LPDIRECT3DTEXTURE8      m_pOverlay[2]={NULL,NULL};      // Overlay textures
LPDIRECT3DSURFACE8      m_pSurface[2]={NULL,NULL};      // Overlay Surfaces
bool                    m_bFlip=false;
bool                    m_bRenderGUI=false;
static RESOLUTION       m_iResolution=PAL_4x3;
bool                    m_bFlipped;
bool					m_bHasDimView;		// Screensaver

typedef struct directx_fourcc_caps
{
    char*             img_format_name;      //human readable name
    unsigned int      img_format;           //as MPlayer image format
    unsigned int      drv_caps;             //what hw supports with this format
} directx_fourcc_caps;

// we just support 1 format which is YUY2
#define DIRECT3D8CAPS VFCAP_CSP_SUPPORTED_BY_HW |VFCAP_CSP_SUPPORTED |VFCAP_OSD |VFCAP_HWSCALE_UP|VFCAP_HWSCALE_DOWN|VFCAP_TIMER
static directx_fourcc_caps g_ddpf[] =
{
  {"YUY2 ",IMGFMT_YUY2 ,DIRECT3D8CAPS},
};

struct VERTEX 
{ 
  D3DXVECTOR4 p;
  D3DCOLOR col; 
  FLOAT tu, tv; 
};
static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;

#define NUM_FORMATS (sizeof(g_ddpf) / sizeof(g_ddpf[0]))

static void video_flip_page(void);

//********************************************************************************************************
void choose_best_resolution(float fps)
{
  DWORD dwStandard = XGetVideoStandard();
  DWORD dwFlags    = XGetVideoFlags();

  bool bUsingPAL        = (dwStandard==XC_VIDEO_STANDARD_PAL_I);    // current video standard:PAL or NTSC 
  bool bCanDoWidescreen = (dwFlags & XC_VIDEO_FLAGS_WIDESCREEN)!=0; // can widescreen be enabled?

  // Work out if the framerate suits PAL50 or PAL60
  bool bPal60=false;
  if (bUsingPAL && g_stSettings.m_bAllowPAL60 && (dwFlags&XC_VIDEO_FLAGS_PAL_60Hz))
  {
#if 0
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
#endif
    if (fps >=29.0) bPal60=true;
  }

  // Work out if framesize suits 4:3 or 16:9
  // Uses the frame aspect ratio of 8/(3*sqrt(3)) which is the optimal point
  // where the percentage of black bars to screen area in 4:3 and 16:9 is equal
  bool bWidescreen = false;
  if (bCanDoWidescreen && ((float)d_image_width/d_image_height > 8.0f/(3.0f*sqrt(3.0f))))
  {
    bWidescreen = true;
  }

  // if video switching is not allowed then use current resolution (with pal 60 if needed)
  // PERHAPS ALSO SUPPORT WIDESCREEN SWITCHING HERE??
  // if we're not in fullscreen mode then use current resolution 
  // if we're calibrating the video  then use current resolution 
  if  ( (!g_stSettings.m_bAllowVideoSwitching) ||
        (! ( g_graphicsContext.IsFullScreenVideo()|| g_graphicsContext.IsCalibrating())  )
      )
  {
    m_iResolution = g_graphicsContext.GetVideoResolution();
    // Check to see if we are using a PAL screen capable of PAL60
    if (bUsingPAL)
    {
      // FIXME - Fix for autochange of widescreen once GUI option is implemented
      bWidescreen = (g_settings.m_ResInfo[m_iResolution].dwFlags&D3DPRESENTFLAG_WIDESCREEN)!=0;
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
    // Change our screen resolution
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
    if ((image_height>540) && (dwFlags&XC_VIDEO_FLAGS_HDTV_720p))
    { //image suits 720p if it's available
      m_iResolution = HDTV_720p;
    }
    else if ((image_height>480 || image_width>720) && (dwFlags&XC_VIDEO_FLAGS_HDTV_1080i))                  //1080i is available
    { // image suits 1080i (540p) if it is available
      m_iResolution = HDTV_1080i;
    }
    else if ((image_height>480 || image_width>720) && (dwFlags&XC_VIDEO_FLAGS_HDTV_720p))
    { // image suits 1080i or 720p and obviously 1080i is unavailable
      m_iResolution = HDTV_720p;
    }
    else  // either picture does not warrant HDTV, or HDTV modes are unavailable
    {
      if (dwFlags&XC_VIDEO_FLAGS_HDTV_480p)
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
  g_graphicsContext.SetVideoResolution(m_iResolution);
}

//**********************************************************************************************
// ClearSubtitleRegion()
//
// Clears our subtitle texture
//**********************************************************************************************
static void ClearSubtitleRegion(int iTexture)
{
//  OutputDebugString("Clearing subs\n");
  for (int y=SUBTITLE_TEXTURE_HEIGHT-iClearSubtitleRegion[iTexture]; y < SUBTITLE_TEXTURE_HEIGHT; y++)
  {
    for (int x=0; x < (int)(subtitlestride); x+=2)
    {
      *(subtitleimage + subtitlestride*y+x   ) = 0x00;  // for black Y=0x10  U=0x80 V=0x80
      *(subtitleimage + subtitlestride*y+x+1 ) = 0x01;
    }
  }
  iClearSubtitleRegion[iTexture] = 0;
}

//********************************************************************************************************
void xbox_video_update_subtitle_position()
{
  if (!m_pSubtitleVB)
    return;
  VERTEX* vertex=NULL;
  m_pSubtitleVB->Lock( 0, 0, (BYTE**)&vertex, 0L );
  float fSubtitleHeight = (float)g_settings.m_ResInfo[m_iResolution].iWidth/SUBTITLE_TEXTURE_WIDTH*SUBTITLE_TEXTURE_HEIGHT;
  float fSubtitlePosition = g_settings.m_ResInfo[m_iResolution].iSubtitles - fSubtitleHeight;
  vertex[0].p = D3DXVECTOR4( 0, fSubtitlePosition,    0, 0 );
  vertex[0].tu = 0;
  vertex[0].tv = 0;

  vertex[1].p = D3DXVECTOR4( (float)g_settings.m_ResInfo[m_iResolution].iWidth, fSubtitlePosition,    0, 0 );
  vertex[1].tu = SUBTITLE_TEXTURE_WIDTH;
  vertex[1].tv = 0;

  vertex[2].p = D3DXVECTOR4( (float)g_settings.m_ResInfo[m_iResolution].iWidth, fSubtitlePosition + fSubtitleHeight,  0, 0 );
  vertex[2].tu = SUBTITLE_TEXTURE_WIDTH;
  vertex[2].tv = SUBTITLE_TEXTURE_HEIGHT;

  vertex[3].p = D3DXVECTOR4( 0, fSubtitlePosition + fSubtitleHeight,  0, 0 );
  vertex[3].tu = 0;
  vertex[3].tv = SUBTITLE_TEXTURE_HEIGHT;
   
  vertex[0].col = 0xFFFFFFFF;
  vertex[1].col = 0xFFFFFFFF;
  vertex[2].col = 0xFFFFFFFF;
  vertex[3].col = 0xFFFFFFFF;
  m_pSubtitleVB->Unlock();  
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

    g_graphicsContext.Get3DDevice()->CreateTexture( image_width,
                                                    image_height,
                                                    1,
                                                    0,
                                                    D3DFMT_YUY2,
                                                    0,
                                                    &m_pOverlay[i] ) ;
    m_pOverlay[i]->GetSurfaceLevel( 0, &m_pSurface[i] );

    // Create subtitle texture
    if (m_pSubtitleTexture[i])  m_pSubtitleTexture[i]->Release();
    g_graphicsContext.Get3DDevice()->CreateTexture( SUBTITLE_TEXTURE_WIDTH,
                                                    SUBTITLE_TEXTURE_HEIGHT,
                                                    1,
                                                    0,
                                                    D3DFMT_LIN_A8R8G8B8,
                                                    0,
                                                    &m_pSubtitleTexture[i] ) ;

    // Clear the subtitle region of this overlay
    D3DLOCKED_RECT rectLocked;
    if ( D3D_OK == m_pSubtitleTexture[i]->LockRect(0,&rectLocked,NULL,0L) )
    {   
      subtitleimage  =(unsigned char*)rectLocked.pBits;
      subtitlestride = rectLocked.Pitch;
      iClearSubtitleRegion[i] = SUBTITLE_TEXTURE_HEIGHT;
      ClearSubtitleRegion(i);
    }
    m_pSubtitleTexture[i]->UnlockRect(0);
  }

  // Create our vertex buffer
  if (m_pSubtitleVB) m_pSubtitleVB->Release();
  g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pSubtitleVB );
  xbox_video_update_subtitle_position();

  g_graphicsContext.Get3DDevice()->EnableOverlay(TRUE);
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

//********************************************************************************************************
static unsigned int Directx_ManageDisplay()
{
  RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
  float fOffsetX1 = (float)g_settings.m_ResInfo[iRes].Overscan.left;
  float fOffsetY1 = (float)g_settings.m_ResInfo[iRes].Overscan.top;
  float iScreenWidth = (float)g_settings.m_ResInfo[iRes].Overscan.width;
  float iScreenHeight = (float)g_settings.m_ResInfo[iRes].Overscan.height;

  if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    const RECT& rv = g_graphicsContext.GetViewWindow();
    iScreenWidth = (float)rv.right-rv.left;
    iScreenHeight= (float)rv.bottom-rv.top;
    fOffsetX1    = (float)rv.left;
    fOffsetY1    = (float)rv.top;
  }

  // Correct for HDTV_1080i -> 540p
  if (iRes == HDTV_1080i)
  {
    fOffsetY1/=2;
    iScreenHeight/=2;
  }

  //if( g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() )
  {
    if (g_stSettings.m_bStretch)
    {
      // stretch the movie so it occupies the entire screen (aspect ratio = gone)
      rs.left   = 0;
      rs.top    = 0;
      rs.right  = image_width;
      rs.bottom = image_height ;
      
      rd.left   = (int)fOffsetX1;
      rd.right  = (int)rd.left+(int)iScreenWidth;
      rd.top    = (int)fOffsetY1;
      rd.bottom = (int)rd.top+(int)iScreenHeight;

      return 0;
    }

    if (g_stSettings.m_bZoom)
    {
        // zoom / panscan the movie so that it fills the entire screen
        // and keeps the Aspect ratio

        // calculate AR compensation (see http://www.iki.fi/znark/video/conversion)
        float fOutputFrameRatio = fSourceFrameRatio / g_settings.m_ResInfo[iRes].fPixelRatio; 
        if (m_iResolution == HDTV_1080i) fOutputFrameRatio *= 2;

        // assume that the movie is widescreen first, so use full height
        float fVertBorder=0;
        float fNewHeight = (float)( iScreenHeight);
        float fNewWidth  =  fNewHeight*fOutputFrameRatio;
        float fHorzBorder= (fNewWidth-(float)iScreenWidth)/2.0f;
        float fFactor = fNewWidth / ((float)image_width);
        fHorzBorder = fHorzBorder/fFactor;

        if ( (int)fNewWidth < iScreenWidth )
        {
          fHorzBorder=0;
          fNewWidth  = (float)( iScreenWidth);
          fNewHeight = fNewWidth/fOutputFrameRatio;
          fVertBorder= (fNewHeight-(float)iScreenHeight)/2.0f;
          fFactor = fNewWidth / ((float)image_width);
          fVertBorder = fVertBorder/fFactor;
        }
        rs.left   = (int)fHorzBorder;
        rs.top    = (int)fVertBorder;
        rs.right  = (int)image_width  - (int)fHorzBorder;
        rs.bottom = (int)image_height - (int)fVertBorder;

        rd.left   = (int)fOffsetX1;
        rd.right  = (int)rd.left + (int)iScreenWidth;
        rd.top    = (int)fOffsetY1;
        rd.bottom = (int)rd.top + (int)iScreenHeight;

        return 0;
    }

    // NORMAL
    // scale up image as much as possible
    // and keep the aspect ratio (introduces with black bars)

    float fOutputFrameRatio = fSourceFrameRatio / g_settings.m_ResInfo[iRes].fPixelRatio; 
    if (m_iResolution == HDTV_1080i) fOutputFrameRatio *= 2;

    // maximize the movie width
    float fNewWidth  = iScreenWidth;
    float fNewHeight = fNewWidth/fOutputFrameRatio;

    if (fNewHeight > iScreenHeight)
    {
      fNewHeight = iScreenHeight;
      fNewWidth = fNewHeight*fOutputFrameRatio;
    }

    // this shouldnt happen, but just make sure that everything still fits onscreen
    if (fNewWidth > iScreenWidth || fNewHeight > iScreenHeight)
    {
      fNewWidth=(float)image_width;
      fNewHeight=(float)image_height;
    }

    // Centre the movie
    float iPosY = (iScreenHeight - fNewHeight)/2;
    float iPosX = (iScreenWidth  - fNewWidth)/2;

    // source rect
    rs.left   = 0;
    rs.top    = 0;
    rs.right  = image_width;
    rs.bottom = image_height;

    rd.left   = (int)(iPosX + fOffsetX1);
    rd.right  = (int)(rd.left + fNewWidth + 0.5f);
    rd.top    = (int)(iPosY + fOffsetY1);
    rd.bottom = (int)(rd.top + fNewHeight + 0.5f);

  }
  return 0;
}

//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
static void vo_draw_alpha_rgb32_shadow(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
{
    register int y;
    for(y=0;y<h;y++)
    {
        register int x;

        for(x=0;x<w;x++)
        {
            if(srca[x])
            {
              dstbase[4*x+0]=((dstbase[4*x+0]*srca[x])>>8)+src[x];
              dstbase[4*x+1]=((dstbase[4*x+1]*srca[x])>>8)+src[x];
              dstbase[4*x+2]=((dstbase[4*x+2]*srca[x])>>8)+src[x];
              dstbase[4*x+3]=0xff;
            }
        }
        src+=srcstride;
        srca+=srcstride;
        dstbase+=dststride;
    }
}

static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,unsigned char *srca, int stride)
{
  // if we draw text on the bottom then it must b the subtitles
  // if we're not in stretch mode try to put the subtitles below the video
  if ( y0 > (int)(image_height/10)  )
  {
    if (w > SUBTITLE_TEXTURE_WIDTH) w = SUBTITLE_TEXTURE_WIDTH;
    if (h > SUBTITLE_TEXTURE_HEIGHT) h = SUBTITLE_TEXTURE_HEIGHT;
    int xpos = (SUBTITLE_TEXTURE_WIDTH-w)/2;
//    OutputDebugString("Rendering Subs\n");
    vo_draw_alpha_rgb32_shadow(w,h,src,srca,stride,((unsigned char *) subtitleimage) + subtitlestride*(SUBTITLE_TEXTURE_HEIGHT-h) + 4*xpos,subtitlestride);
//    vo_draw_alpha_rgb32(w,h,src,srca,stride,((unsigned char *) subtitleimage) + subtitlestride*(SUBTITLE_TEXTURE_HEIGHT-h) + 4*xpos,subtitlestride);
    iClearSubtitleRegion[m_iBackBuffer] = h;
    m_bRenderGUI=true;
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
        0x1  -  supported (with or without conversion)
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
  OutputDebugString("video_uninit\n");  

  g_graphicsContext.Get3DDevice()->EnableOverlay(FALSE);
  for (int i=0; i <=1; i++)
  {
    if ( m_pSurface[i]) m_pSurface[i]->Release();
    if ( m_pOverlay[i]) m_pOverlay[i]->Release();
    m_pSurface[i]=NULL;
    m_pOverlay[i]=NULL;
    // subtitle stuff
    if ( m_pSubtitleTexture[i]) m_pSubtitleTexture[i]->Release();
    m_pSubtitleTexture[i]=NULL;
    if (m_pSubtitleVB) m_pSubtitleVB->Release();
    m_pSubtitleVB=NULL;
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
  m_iResolution=PAL_4x3;
  m_bPauseDrawing=false;
  for (int i=0; i<2; i++) iClearSubtitleRegion[i] = 0;
  m_iBackBuffer=0;
  m_bFlip=false;
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
  OutputDebugString("video_draw_slice?? (should not happen)\n");
  return 0;
}

//********************************************************************************************************
void xbox_video_render_subtitles(bool bUseBackBuffer)
{
  int iTexture = bUseBackBuffer ? m_iBackBuffer : 1-m_iBackBuffer;
  if (!m_pSubtitleTexture[iTexture])
    return ;

  if (!m_pSubtitleVB)
    return;
//  OutputDebugString("Rendering subs to screen\n");
    // Set state to render the image
    g_graphicsContext.Get3DDevice()->SetTexture( 0, m_pSubtitleTexture[iTexture]);
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );

    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, FALSE);
    g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
    // Render the image
    g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pSubtitleVB, sizeof(VERTEX) );
    g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

}
//********************************************************************************************************
static void video_flip_page(void)
{
  if (!m_bFlip) return;
  m_bFlip=false;
  m_pOverlay[m_iBackBuffer]->UnlockRect(0);     
  m_pSubtitleTexture[m_iBackBuffer]->UnlockRect(0);

  Directx_ManageDisplay();
  if (!m_bPauseDrawing)
  {
	m_bHasDimView = false;		// reset for next screensaver event

    if (g_graphicsContext.IsFullScreenVideo() )
    {
      if (m_bRenderGUI||g_application.NeedRenderFullScreen())
      {
        m_bRenderGUI=true;
        g_graphicsContext.Lock();
        g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
        g_application.RenderFullScreen();
        // update our subtitle position
        xbox_video_update_subtitle_position();
        xbox_video_render_subtitles(true);
        g_graphicsContext.Unlock();
      }
	
	  g_graphicsContext.Lock();

      while (!g_graphicsContext.Get3DDevice()->GetOverlayUpdateStatus()) Sleep(10);
      g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_iBackBuffer], &rs, &rd, TRUE, 0x00010001  );
      if (m_bRenderGUI)
      {
        g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
      }
      g_graphicsContext.Unlock();
      m_bRenderGUI=false;
    }
    else
    {
      g_graphicsContext.Lock();
      g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_iBackBuffer], &rs, &rd, TRUE, 0x00010001  );
      g_graphicsContext.Unlock();
    }
  }
  
  m_iBackBuffer=1-m_iBackBuffer;

  D3DLOCKED_RECT rectLocked;
  if ( D3D_OK == m_pOverlay[m_iBackBuffer]->LockRect(0,&rectLocked,NULL,0L  ))
  {
    dstride=rectLocked.Pitch;
    image  =(unsigned char*)rectLocked.pBits;
  }
  D3DLOCKED_RECT rectLocked2;
  if ( D3D_OK == m_pSubtitleTexture[m_iBackBuffer]->LockRect(0,&rectLocked2,NULL,0L  ))
  {
    subtitlestride=rectLocked2.Pitch;
    subtitleimage  =(unsigned char*)rectLocked2.pBits;
  }

  if (iClearSubtitleRegion[m_iBackBuffer])
  {
    ClearSubtitleRegion(m_iBackBuffer);
    m_bRenderGUI=true;
  }

  m_bFlipped=true;
}

//********************************************************************************************************
void xbox_video_wait()
{
  int iTmp=0;
  m_bFlipped=false;
  while (!m_bFlipped && iTmp < 300)
  {
    Sleep(10);
    iTmp++;
  }
}
//********************************************************************************************************
void xbox_video_getRect(RECT& SrcRect, RECT& DestRect)
{
  SrcRect=rs;
  DestRect=rd;
}

void xbox_video_CheckScreenSaver()
{
	// Called from CMPlayer::Process() (mplayer.cpp) when in 'pause' mode
	D3DLOCKED_RECT lr;

	if (g_application.m_bScreenSave && !m_bHasDimView)
	{
		if ( D3D_OK == m_pSurface[m_iBackBuffer]->LockRect( &lr, NULL, 0 ))
		{
			// Drop brightness of current surface to 20%
			DWORD strideScreen=lr.Pitch;
			for (DWORD y=0; y < UINT (rs.top + rs.bottom); y++)
			{
				BYTE *pDest = (BYTE*)lr.pBits + strideScreen*y;
				for (DWORD x=0; x < UINT ((rs.left + rs.right)>>1); x++)
				{
					pDest[0] = BYTE (pDest[0] * 0.20f);	// Y1
					pDest[1] = BYTE ((pDest[1] - 128) * 0.20f) + 128;	// U (with 128 shift!)
					pDest[2] = BYTE (pDest[2] * 0.20f);	// Y2
					pDest[3] = BYTE ((pDest[3] - 128) * 0.20f) + 128;	// V (with 128 shift!)
					pDest += 4;
				}
			}
			m_pSurface[m_iBackBuffer]->UnlockRect();

			// Commit to screen
			g_graphicsContext.Lock();
			while (!g_graphicsContext.Get3DDevice()->GetOverlayUpdateStatus()) Sleep(10);
			g_graphicsContext.Get3DDevice()->UpdateOverlay( m_pSurface[m_iBackBuffer], &rs, &rd, TRUE, 0x00010001  );
			g_graphicsContext.Unlock();
		}

		m_bHasDimView = true;
	}

	return;
}

//********************************************************************************************************
void xbox_video_getAR(float& fAR)
{
  float fOutputPixelRatio = g_settings.m_ResInfo[m_iResolution].fPixelRatio;
  if (m_iResolution == HDTV_1080i)
    fOutputPixelRatio /= 2;
  float fWidth = (float)(rd.right - rd.left);
  float fHeight = (float)(rd.bottom - rd.top);
  fAR = fWidth/fHeight*fOutputPixelRatio;
}

//********************************************************************************************************
void xbox_video_update(bool bPauseDrawing)
{
//  OutputDebugString("Calling xbox_video_update ... ");
  m_bPauseDrawing = bPauseDrawing;
  bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
  g_graphicsContext.Lock();
  g_graphicsContext.SetVideoResolution(bFullScreen ? m_iResolution:g_stSettings.m_ScreenResolution);
  g_graphicsContext.Unlock();
  Directx_ManageDisplay();
//  OutputDebugString("Done \n");
}
/********************************************************************************************************
  draw_frame(): this is the older interface, this displays only complete
  frames, and can do only packed format (YUY2, RGB/BGR).
  Win32 codecs use this (DivX, Indeo, etc).
  If you implement VOCTRL_DRAW_IMAGE, you can left draw_frame.
*/
static unsigned int video_draw_frame(unsigned char *src[])
{

  OutputDebugString("video_draw_frame?? (should not happen)\n");
  return 0;
}

//********************************************************************************************************
static unsigned int get_image(mp_image_t *mpi)
{
  g_graphicsContext.Lock();
  while (!g_graphicsContext.Get3DDevice()->GetOverlayUpdateStatus()) Sleep(10);
  g_graphicsContext.Unlock();

  if((2*mpi->width==dstride) || (mpi->flags&(MP_IMGFLAG_ACCEPT_STRIDE|MP_IMGFLAG_ACCEPT_WIDTH)))
  {
    if(mpi->flags&MP_IMGFLAG_PLANAR)
    {
    }
    mpi->planes[0]=image;
    mpi->stride[0]=dstride;
    mpi->width=image_width;
    mpi->height=image_height;
    mpi->flags|=MP_IMGFLAG_DIRECT;
    return VO_TRUE;
  }
  return VO_FALSE;
}
//********************************************************************************************************
static unsigned int put_image(mp_image_t *mpi)
{
  g_graphicsContext.Lock();
  while (!g_graphicsContext.Get3DDevice()->GetOverlayUpdateStatus()) Sleep(10);
  g_graphicsContext.Unlock();

  if((mpi->flags&MP_IMGFLAG_DIRECT)||(mpi->flags&MP_IMGFLAG_DRAW_CALLBACK)) 
  {
      m_bFlip=true;
      return VO_TRUE;
  }

  if (mpi->flags&MP_IMGFLAG_PLANAR)
  {
    for (int y=0; y < mpi->h; ++y)
    {
      memcpy(image+y*dstride,mpi->planes[0]+mpi->stride[0]*y,2*mpi->width);
    }
  }
  else
  {
    //packed
    memcpy( image, mpi->planes[0], image_height * dstride);
  }

  m_bFlip=true;
  return VO_TRUE;
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
  char strFourCC[12];
  char strVideoCodec[256];
  float fps;
  unsigned int iWidth,iHeight;
  long tooearly, toolate;

  mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fps, &iWidth,&iHeight, &tooearly, &toolate);

  OutputDebugString("video_config\n");
  fs = options & 0x01;
  image_format   =  format;
  image_width    = width;
  image_height   = height;
  d_image_width  = d_width;
  d_image_height = d_height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio();
  choose_best_resolution(fps);

  fs=1;//fullscreen

  Directx_ManageDisplay();
  Directx_CreateOverlay(image_format);
  // get stride
  D3DLOCKED_RECT rectLocked;
  if ( D3D_OK == m_pOverlay[m_iBackBuffer]->LockRect(0,&rectLocked,NULL,0L  ) )
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