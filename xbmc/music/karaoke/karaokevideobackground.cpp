/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "guilib/Texture.h"
#include "guilib/GUITexture.h"
#include "settings/Settings.h"
#include "Application.h"
#include "utils/MathUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "DllAvFormat.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"
#include "DllSwScale.h"

#include "karaokevideobackground.h"

static bool PERSISTENT_OBJECT = 0;


KaraokeVideoFFMpeg::KaraokeVideoFFMpeg()
{
  pFormatCtx = 0;
  pCodecCtx = 0;
  pCodec = 0;
  pFrame = 0;
  pFrameRGB = 0;
  m_texture = 0;

  m_maxFrame = 0;
  m_currentFrameNumber = 0;
  m_frameBase = 0;
  
  m_dllAvFormat = new DllAvFormat();
  m_dllAvCodec = new DllAvCodec();
  m_dllAvUtil = new DllAvUtil();
  m_dllSwScale = new DllSwScale();
}

KaraokeVideoFFMpeg::~KaraokeVideoFFMpeg()
{
  Dismiss();
  
  delete m_dllAvFormat;
  delete m_dllAvCodec;
  delete m_dllAvUtil;
  delete m_dllSwScale;

  delete m_texture;
}

bool KaraokeVideoFFMpeg::Init()
{
  if ( !m_dllAvUtil->Load() || !m_dllAvCodec->Load() || !m_dllSwScale->Load() || !m_dllAvFormat->Load() )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: failed to load FFMpeg libraries" );
    return false;
  }

  m_dllAvCodec->avcodec_register_all();
  m_dllAvFormat->av_register_all();

  CLog::Log( LOGDEBUG, "Karaoke Video Background: init succeed" );
  return true;
}

void KaraokeVideoFFMpeg::Dismiss()
{
  closeVideoFile();
  
  m_dllAvCodec->Unload();
  m_dllAvUtil->Unload();
  m_dllSwScale->Unload();
  m_dllAvFormat->Unload();
  
  CLog::Log( LOGDEBUG, "Karaoke Video Background: dismiss succeed" );
}

bool KaraokeVideoFFMpeg::openVideoFile( const CStdString& filename )
{
  // See http://dranger.com/ffmpeg/tutorial01.html
  closeVideoFile();

  CStdString realPath = CSpecialProtocol::TranslatePath( filename );
  
  // Open video file
  if ( m_dllAvFormat->avformat_open_input( &pFormatCtx, realPath.c_str(), NULL, NULL ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not open video file %s (%s)", filename.c_str(), realPath.c_str() );
    return false;
  }

  // Retrieve stream information
  if ( m_dllAvFormat->avformat_find_stream_info( pFormatCtx, 0 ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not find stream information in the video file %s", filename.c_str() );
    return false;
  }

  // Find the first video stream
  videoStream = -1;

  for ( unsigned i = 0; i < pFormatCtx->nb_streams; i++ )
  {
    if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
    {
      videoStream = i;
      break;
    }
  }

  if ( videoStream == -1 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not find playable video stream in the file %s", filename.c_str() );
    return false;
  }

  m_fps_den = pFormatCtx->streams[videoStream]->r_frame_rate.den;
  m_fps_num = pFormatCtx->streams[videoStream]->r_frame_rate.num;

  if ( m_fps_den == 60000 )
    m_fps_den = 30000;

  // Get a pointer to the codec context for the video stream
  pCodecCtx = pFormatCtx->streams[ videoStream ]->codec;

  // Find the decoder for the video stream
  pCodec = m_dllAvCodec->avcodec_find_decoder( pCodecCtx->codec_id );

  if ( pCodec == NULL )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not find the video decoder for the file %s", filename.c_str() );
    return false;
  }
    
  // Open the codec
  if ( m_dllAvCodec->avcodec_open2( pCodecCtx, pCodec, 0 ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not open the video decoder for the file %s", filename.c_str() );
    return false;
  }
  
  // Allocate video frames
  pFrame = m_dllAvCodec->avcodec_alloc_frame();

  if ( !pFrame )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate memory for frame" );
    return false;
  }
  
  // Init the rest of params
  m_videoWidth = pCodecCtx->width;
  m_videoHeight = pCodecCtx->height;
  m_currentFrameNumber = 0;
  m_maxFrame = 0;
  m_curVideoFile = filename;
  
  // Find out the necessary aspect ratio for height (assuming fit by width) and width (assuming fit by height)
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  m_displayLeft = g_settings.m_ResInfo[res].Overscan.left;
  m_displayRight = g_settings.m_ResInfo[res].Overscan.right;
  m_displayTop = g_settings.m_ResInfo[res].Overscan.top;
  m_displayBottom = g_settings.m_ResInfo[res].Overscan.bottom;
  
  int screen_width = m_displayRight - m_displayLeft;
  int screen_height = m_displayBottom - m_displayTop;

  // Do we need to modify the output video size? This could happen in two cases:
  // 1. Either video dimension is larger than the screen - video needs to be downscaled
  // 2. Both video dimensions are smaller than the screen - video needs to be upscaled
  if ( (m_videoWidth > screen_width || m_videoHeight > screen_height )
  || (  m_videoWidth < screen_width && m_videoHeight < screen_height ) )
  {
    // Calculate the scale coefficients for width/height separately
    double scale_width = (double) screen_width / (double) m_videoWidth;
    double scale_height = (double) screen_height / (double) m_videoHeight;

    // And apply the smallest
    double scale = scale_width < scale_height ? scale_width : scale_height;
    m_videoWidth = (int) ((double) m_videoWidth * scale);
    m_videoHeight = (int) ((double) m_videoHeight * scale);
  }

  // Allocate the conversion frame and relevant picture
  pFrameRGB = (AVPicture*)m_dllAvUtil->av_mallocz(sizeof(AVPicture));

  if ( !pFrameRGB )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate memory for frame" );
    return false;
  }
  
  // Due to a bug in swsscale we need to allocate one extra line of data
  if ( m_dllAvCodec->avpicture_alloc( pFrameRGB, PIX_FMT_RGB32, m_videoWidth, m_videoHeight + 1 ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate memory for picture buf" );
    return false;
  }

  // Calculate the desktop dimensions to show the video
  if ( m_videoWidth < screen_width || m_videoHeight < screen_height )
  {
    m_displayLeft = (screen_width - m_videoWidth) / 2;
    m_displayRight -= m_displayLeft;

    m_displayTop = (screen_height - m_videoHeight) / 2;
    m_displayBottom -= m_displayTop;
  }
  
  CLog::Log( LOGDEBUG, "Karaoke Video Background: Video file %s (%dx%d) opened successfully, will be shown as %dx%d at (%d, %d - %d, %d) rectangle", 
             filename.c_str(), 
             pCodecCtx->width, pCodecCtx->height, 
             m_videoWidth, m_videoHeight, 
             m_displayLeft, m_displayTop, m_displayRight, m_displayBottom );
  
  return true;
}

void KaraokeVideoFFMpeg::closeVideoFile()
{
  // Free the YUV frame
  if ( pFrame )
    m_dllAvUtil->av_free( pFrame );

  // Free the RGB frame
  if ( pFrameRGB )
  {
    m_dllAvCodec->avpicture_free( pFrameRGB );
    m_dllAvUtil->av_free( pFrameRGB );
  }

  // Close the codec
  if ( pCodecCtx )
    m_dllAvCodec->avcodec_close( pCodecCtx );

  // Close the video file
  if ( pFormatCtx )
    m_dllAvFormat->avformat_close_input( &pFormatCtx );

  pFormatCtx = 0;
  pCodecCtx = 0;
  pCodec = 0;
  pFrame = 0;
  pFrameRGB = 0;
  m_curVideoFile.clear();
}

bool KaraokeVideoFFMpeg::readFrame( int frame )
{
  AVPacket packet;
  int frameFinished;

  while ( m_currentFrameNumber < frame )
  {
    // Read a frame
    if ( m_dllAvFormat->av_read_frame( pFormatCtx, &packet ) < 0 )
      return false;  // Frame read failed (e.g. end of stream)

    if ( packet.stream_index == videoStream )
    {
      // Is this a packet from the video stream -> decode video frame
      m_dllAvCodec->avcodec_decode_video2( pCodecCtx, pFrame, &frameFinished, &packet );

      // Did we get a video frame?
      if ( frameFinished )
      {
        m_currentFrameNumber++;

        if ( m_currentFrameNumber >= frame )
        {
          // convert the picture
          struct SwsContext * context = m_dllSwScale->sws_getContext( pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
                      m_videoWidth, m_videoHeight, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL );

          m_dllSwScale->sws_scale( context, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize );
          m_dllSwScale->sws_freeContext( context );
          m_dllAvCodec->av_free_packet( &packet );
          return true;
        }
      }
    }
    m_dllAvCodec->av_free_packet( &packet );
  }
  
  return false;
}

void KaraokeVideoFFMpeg::Render()
{
  // Just in case
  if ( !m_texture )
    return;
  
  // Get the current song timing in ms.
  // This will only fit songs up to 71,000 hours, so if you got a larger one, change to int64
  unsigned int songTime = (unsigned int) MathUtils::round_int( g_application.GetTime() * 1000 );

  // Which frame should we show?
  int frame_for_time = m_frameBase + ((songTime * m_fps_num) / m_fps_den) / 1000;

  if ( frame_for_time == 0 )
    frame_for_time = 1;

  // Loop if we know how many frames we have total
  if ( m_maxFrame > 0 )
    frame_for_time %= m_maxFrame;

  // Read a new frame if necessary
  while ( m_currentFrameNumber < frame_for_time )
  {
    if ( readFrame( frame_for_time ) )
      break;

    // End of video; restart
    m_maxFrame = m_currentFrameNumber - 1;
    m_currentFrameNumber = 0;
    m_frameBase = 0;

    // Reset the frame
    frame_for_time = 0;

    if ( m_dllAvFormat->av_seek_frame( pFormatCtx, videoStream, 0, 0 ) < 0 )
      return;

    m_dllAvCodec->avcodec_flush_buffers( pCodecCtx );
  }

  // We got a frame. Draw it.
  m_texture->Update( m_videoWidth, m_videoHeight, m_videoWidth * 4, XB_FMT_A8R8G8B8, pFrameRGB->data[0], false );
  CRect vertCoords((float) m_displayLeft, (float) m_displayTop, (float) m_displayRight, (float) m_displayBottom );
  CGUITexture::DrawQuad(vertCoords, 0xffffffff, m_texture );
}

bool KaraokeVideoFFMpeg::Start( const CStdString& filename )
{
  if ( !m_dllAvFormat->IsLoaded() )
  {
    if ( !Init() )
      return false;
  }
  
  if ( !filename.empty() )
  {
     if ( !openVideoFile( filename ) )
       return false;

     m_frameBase = 0;
  }
  else
  {
     if ( !openVideoFile( g_advancedSettings.m_karaokeDefaultBackgroundFilePath ) )
       return false;

     int64_t timestamp = (m_frameBase * 1000 * m_fps_den) / m_fps_num;

     if ( m_frameBase > 0 && m_dllAvFormat->av_seek_frame( pFormatCtx, videoStream, timestamp, 0 ) >= 0 )
     {
       m_dllAvCodec->avcodec_flush_buffers( pCodecCtx );
       m_currentFrameNumber = m_frameBase - 1;
     }
     else
       m_frameBase = 0;
  }
  
  // Allocate the texture
  m_texture = new CTexture( m_videoWidth, m_videoHeight, XB_FMT_A8R8G8B8 );
  
  if ( !m_texture )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate texture" );
    return false;
  }
  
  return true;
}

void KaraokeVideoFFMpeg::Stop()
{
  if ( !m_dllAvFormat->IsLoaded() )
  {
    CLog::Log( LOGERROR, "KaraokeVideoFFMpeg::Start: internal error, called on unitinialized object" );
    return;
  }

  delete m_texture;
  m_texture = 0;
  
  m_frameBase = m_currentFrameNumber;
  
  if ( !PERSISTENT_OBJECT )
    Dismiss();
}
