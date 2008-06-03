/*
* XBMC
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
#include "cores/VideoRenderers/RenderManager.h"
#include "Application.h"

#include "video.h"
#include "mplayer.h"


//Uncomment line here to allow direct rendering
//Disabling for now.. the code below works, however it doesn't seem
//#define MP_DIRECTRENDERING  // should be defined in XboxRenderer.h instead

#ifdef MP_DIRECTRENDERING
static bool m_bAllowDR = true;
static unsigned int m_iBuffer = -1;
#endif

static int m_iLastFieldFlags = 0;
static EFIELDSYNC m_iFieldSync = FS_NONE;

void video_uninit(void);

/********************************************************************************************************
  mplayer calls below here
********************************************************************************************************/

//********************************************************************************************************
static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
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
  if (g_renderManager.Paused()) return ;
  //RECT rs, rd;
  //g_renderManager.GetVideoRect(rs, rd);
  //vo_draw_text(rd.right-rd.left, rd.bottom-rd.top, draw_alpha);
  // for now use fixed vobsub size, so scaling workes ok for 
  // mplayer text subs size we force the scaling based on fixes size
  vo_draw_text(720, 480, draw_alpha);
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
{}

//***********************************************************************************************************
/*init the video system (to support querying for supported formats)*/
static unsigned int video_preinit(const char *arg)
{
  return g_renderManager.PreInit();
}

/********************************************************************************************************
  draw_slice(): this displays YV12 pictures (3 planes, one full sized that
  contains brightness (Y), and 2 quarter-sized which the colour-info
  (U,V). MPEG codecs (libmpeg2, opendivx) use this. This doesn't have
  to display the whole frame, only update small parts of it.
*/
static unsigned int video_draw_slice(unsigned char *src[], int stride[], int w, int h, int x, int y )
{
  return g_renderManager.DrawSlice(src, stride, w, h, x, y);
}

//********************************************************************************************************
static void video_flip_page(void)
{
  //Do this async to let mplayer keep better sync
#ifdef MP_DIRECTRENDERING
  g_renderManager.FlipPage(1, m_iBuffer, m_iFieldSync);
#else
  g_renderManager.FlipPage(1, -1, m_iFieldSync);
#endif
}
/********************************************************************************************************
  draw_frame(): this is the older interface, this displays only complete
  frames, and can do only packed format (YUY2, RGB/BGR).
  Win32 codecs use this (DivX, Indeo, etc).
  If you implement VOCTRL_DRAW_IMAGE, you can left draw_frame.
*/
static unsigned int video_draw_frame(unsigned char *src[])
{
  return VO_FALSE;
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
  CLog::Log(LOGDEBUG, "mplayer::video_config(%d, %d, %d, %d, %d, %d)", width, height, d_width, d_height, options, format);

#ifdef MP_DIRECTRENDERING
  m_bAllowDR = true;
#endif

  m_iLastFieldFlags = 0;

  float fps = 25.00f;
  if (g_application.m_pPlayer)
    fps = g_application.m_pPlayer->GetActualFPS();

  unsigned flags = 0;
  if(options & VOFLAG_FULLSCREEN)
    flags |= CONF_FLAGS_FULLSCREEN;

  //TODO, the format parameter, should be able to give a fullrange yuv format
  if(g_renderManager.Configure(width, height, d_width, d_height, fps, flags))
    return 0;

  return VO_ERROR;
}

//********************************************************************************************************
static unsigned int video_control(unsigned int request, void *data, ...)
{
  switch (request)
  {
  case VOCTRL_GET_IMAGE:
    {
    //libmpcodecs Direct Rendering interface
    //You need to update mpi (mp_image.h) structure, for example,
    //look at vo_x11, vo_sdl, vo_xv or mga_common.

#ifdef MP_DIRECTRENDERING
      mp_image_t *mpi = (mp_image_t*)data;

      if( !m_bAllowDR )
      {
        mpi->flags &= ~MP_IMGFLAG_DIRECT;
        return VO_FALSE;
      }

      /* only planar yuv data */
      if( !(mpi->flags & MP_IMGFLAG_PLANAR) )
        return VO_FALSE;

      /* check what format we support */
      if( mpi->imgfmt != IMGFMT_YV12 )
        return VO_FALSE;

      /* we always need codec to accept stride, due to our very special stride restrictions */
      if( !(mpi->flags&MP_IMGFLAG_ACCEPT_STRIDE) )
        return VO_FALSE;

      /* this could be handled with some changes in renderer           *
       * common stride could be a trouble thou as it would require     *
       * double the alignment on the first stride to keep support      *
       * for interlaced scaling. i suppose we could make that optional */
      if( mpi->flags & MP_IMGFLAG_COMMON_STRIDE)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Direct rendering disabled due to need of common stride");
        return VO_FALSE;
      }

      YV12Image image;
      unsigned index = AUTOSOURCE;

      /* if image should be presereved, we must get same image again */
      if(mpi->priv && mpi->flags & MP_IMGFLAG_PRESERVE)
        if( mpi->priv )
          index = (int)mpi->priv - 1;

      /* wish i could know when ffmpeg is doing a reget instead *
       * of a get, since it only needs read access then         */
      index = g_renderManager.GetImage(&image, index, false);
      if( index >= 0 )
      {
        if( mpi->flags & MP_IMGFLAG_SWAPPED )
        {
			    mpi->planes[2] = image.plane[0];
	        mpi->planes[1] = image.plane[1];
          mpi->planes[0] = image.plane[2];

          mpi->stride[2] = image.stride[0];
          mpi->stride[1] = image.stride[1];
          mpi->stride[0] = image.stride[2];
        }
        else
        {
			    mpi->planes[0] = image.plane[0];
	        mpi->planes[1] = image.plane[1];
          mpi->planes[2] = image.plane[2];

          mpi->stride[0] = image.stride[0];
          mpi->stride[1] = image.stride[1];
          mpi->stride[2] = image.stride[2];
        }

        /* everything seem fine for direct rendering */
        mpi->flags |= MP_IMGFLAG_DIRECT;
        mpi->priv = (void*)(index+1);
        return VO_TRUE;

      }
#endif

      return VO_FALSE;
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
  case VOCTRL_QUERY_FORMAT:
    {
      unsigned int format = *((unsigned int*)data);
      if (format == IMGFMT_YV12)
        return VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_CSP_SUPPORTED | VFCAP_OSD | VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN | VFCAP_ACCEPT_STRIDE;

      return 0;
    }
  case VOCTRL_DRAW_IMAGE:
    /*  replacement for the current draw_slice/draw_frame way of
        passing video frames. by implementing SET_IMAGE, you'll get
        image in mp_image struct instead of by calling draw_*.
        unless you return VO_TRUE for VOCTRL_DRAW_IMAGE call, the
        old-style draw_* functils will be called!
        Note: draw_slice is still mandatory, for per-slice rendering!
    */
    {
      mp_image_t *mpi = (mp_image_t*)data;

      if( mpi->fields & MP_IMGFIELD_INTERLACED &&
         m_iLastFieldFlags & MP_IMGFIELD_REPEAT_FIRST && !(m_iLastFieldFlags & MP_IMGFIELD_INTERLACED))
      {
          //This is a workaround for some stupid encoders that doesn't set the progressive frame flag properly
          m_iFieldSync = FS_NONE;
      }
      else if( mpi->fields & MP_IMGFIELD_INTERLACED )
      {
        if( mpi->fields & MP_IMGFIELD_ORDERED )
        {
          if( mpi->fields & MP_IMGFIELD_TOP_FIRST )
            m_iFieldSync = FS_ODD;
          else
            m_iFieldSync = FS_EVEN;
        }
        else
          m_iFieldSync = FS_ODD;
      }
      else
        m_iFieldSync = FS_NONE;

      m_iLastFieldFlags = mpi->fields;

#ifdef MP_DIRECTRENDERING
      if( mpi->priv )
      {
        //We where direct rendering last frame, release image here
        m_iBuffer = (int)mpi->priv - 1;
        if(mpi->flags & MP_IMGFLAG_PRESERVE)
          g_renderManager.ReleaseImage(m_iBuffer, true);
        else
          g_renderManager.ReleaseImage(m_iBuffer, false);

        if (mpi->flags & MP_IMGFLAG_DIRECT)
        {
          // direct rendering.. everything should already be fine
          return VO_TRUE;
        }
        else
        {
          //Crap, we enabled directrendering in last call, but apperently something went wrong
          CLog::Log(LOGWARNING, "Direct Rendering Failed, Disabling for future frames");
          m_bAllowDR = false;
          mpi->priv = NULL;
        }
      }
#endif

      if (mpi->flags & MP_IMGFLAG_DRAW_CALLBACK)
      {
        return VO_TRUE;         // done
      }
      if (mpi->flags & MP_IMGFLAG_PLANAR)
      {
        g_renderManager.DrawSlice(mpi->planes, (int*)(mpi->stride), mpi->w, mpi->h, 0, 0);
        return VO_TRUE;
      }
      return VO_FALSE;
    }
  case VOCTRL_RESET:
    {
      g_renderManager.Reset();
      return VO_TRUE;
    }
    break;
  case VOCTRL_FULLSCREEN:
    {
      //TODO
      return VO_NOTIMPL;
    }
  case VOCTRL_PAUSE:
  case VOCTRL_RESUME:
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
    video_preinit,    // done
    video_config,     // done
    video_control,
    video_draw_frame,   // done
    video_draw_slice,   // done
    video_draw_osd,
    video_flip_page,   // done
    video_check_events,  // unused
    video_uninit    // done
  };
