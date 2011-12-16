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
#include "settings/AdvancedSettings.h"
#include "karaokevideobackground.h"

KaraokeVideoFFMpeg::KaraokeVideoFFMpeg()
{
  pFormatCtx = 0;
  pCodecCtx = 0;
  pCodec = 0;
  pFrame = 0;
  pFrameRGB = 0;
  pBuf_rgb32 = 0;
  m_timeBase = 0;
  m_texture = 0;

  m_maxFrame = 0;
  m_currentFrameNumber = 0;
}

KaraokeVideoFFMpeg::~KaraokeVideoFFMpeg()
{
  closeVideoFile();
  
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllSwScale.Unload();
  m_dllAvFormat.Unload();
}

bool KaraokeVideoFFMpeg::Init()
{
  if ( !m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load() || !m_dllAvFormat.Load() )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: failed to load FFMpeg libraries" );
    return false;
  }

  m_dllAvCodec.avcodec_register_all();
  m_dllAvFormat.av_register_all();

  return true;
}

bool KaraokeVideoFFMpeg::openVideoFile( const CStdString& filename )
{
  // See http://dranger.com/ffmpeg/tutorial01.html
  closeVideoFile();

  // Open video file
  if ( m_dllAvFormat.av_open_input_file( &pFormatCtx, filename.c_str(), NULL, 0, NULL ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not open video file %s", filename.c_str() );
    return false;
  }

  // Retrieve stream information
  if ( m_dllAvFormat.av_find_stream_info( pFormatCtx ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not find stream information in the video file %s", filename.c_str() );
    return false;
  }

  // Find the first video stream
  videoStream = -1;

  for ( unsigned i = 0; i < pFormatCtx->nb_streams; i++ )
  {
    if ( pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
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
  pCodec = m_dllAvCodec.avcodec_find_decoder( pCodecCtx->codec_id );

  if ( pCodec == NULL )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not find the video decoder for the file %s", filename.c_str() );
    return false;
  }
    
  // Open the codec
  if ( m_dllAvCodec.avcodec_open( pCodecCtx, pCodec ) < 0 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not open the video decoder for the file %s", filename.c_str() );
    return false;
  }
  
  // Allocate video frames
  pFrame = m_dllAvCodec.avcodec_alloc_frame();
  pFrameRGB = m_dllAvCodec.avcodec_alloc_frame();

  if ( !pFrame || !pFrameRGB )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate memory for frame" );
    return false;
  }
  
  // Allocate memory for the picture
  int numBytes = m_dllAvCodec.avpicture_get_size( PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height );
  pBuf_rgb32 = new BYTE [numBytes];
  
  if ( !pBuf_rgb32 )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate memory for picture buf" );
    return false;
  }
  
  // Assign appropriate parts of buffer to image planes in pFrameRGB
  m_dllAvCodec.avpicture_fill( (AVPicture *) pFrameRGB, (uint8_t*) pBuf_rgb32, PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height );
  
  m_width = pCodecCtx->width;
  m_height = pCodecCtx->height;
  m_timeBase = 0;
  m_currentFrameNumber = 0;
  m_curVideoFile = filename;
  
  CLog::Log( LOGDEBUG, "Karaoke Video Background: Video file %s (%dx%d) opened successfully", filename.c_str(), m_width, m_height );
  return true;
}

void KaraokeVideoFFMpeg::closeVideoFile()
{
  // Free the YUV frame
  if ( pFrame )
    m_dllAvUtil.av_free( pFrame );

  // Free the RGB frame
  if ( pFrameRGB )
    m_dllAvUtil.av_free( pFrameRGB );

  // Close the codec
  if ( pCodecCtx )
    m_dllAvCodec.avcodec_close( pCodecCtx );

  // Close the video file
  if ( pFormatCtx )
    m_dllAvFormat.av_close_input_file( pFormatCtx );

  delete[] pBuf_rgb32;
  
  pFormatCtx = 0;
  pCodecCtx = 0;
  pCodec = 0;
  pFrame = 0;
  pFrameRGB = 0;
  pBuf_rgb32 = 0;
}

bool KaraokeVideoFFMpeg::readFrame( int frame )
{
  AVPacket packet;
  int frameFinished;

  while ( m_currentFrameNumber < frame )
  {
    // Read a frame
    if ( m_dllAvFormat.av_read_frame( pFormatCtx, &packet ) < 0 )
      return false;  // Frame read failed (e.g. end of stream)

    if ( packet.stream_index == videoStream )
    {
      // Is this a packet from the video stream -> decode video frame
      m_dllAvCodec.avcodec_decode_video2( pCodecCtx, pFrame, &frameFinished, &packet );

      // Did we get a video frame?
      if ( frameFinished )
      {
        m_currentFrameNumber++;

        if ( m_currentFrameNumber >= frame )
        {
          // convert the picture
          struct SwsContext * context = m_dllSwScale.sws_getContext( m_width, m_height, pCodecCtx->pix_fmt, 
                      m_width, m_height, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL );

          m_dllSwScale.sws_scale( context, pFrame->data, pFrame->linesize, 0, m_height, pFrameRGB->data, pFrameRGB->linesize );
		  m_dllSwScale.sws_freeContext( context );
        }

        m_dllAvCodec.av_free_packet( &packet );
      }

      return true;
    }
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
  unsigned int videoTime = m_timeBase + songTime;

  // Which frame should we show?
  int frame_for_time = ((videoTime * m_fps_num) / m_fps_den) / 1000;

  if ( frame_for_time == 0 )
    frame_for_time = 1;

  // Loop if we know how many frames we have total
  if ( m_maxFrame > 0 )
    frame_for_time %= m_maxFrame;

  // Read a new frame if necessary
  while ( m_currentFrameNumber < frame_for_time )
  {
    if ( readFrame( frame_for_time ) )
    {
      // We have read a new frame.
      m_lastTimeFrame = videoTime;
      break;
    }

    // End of video; restart
    m_maxFrame = m_currentFrameNumber - 1;
    m_currentFrameNumber = 0;
    m_timeBase = 0;

    // Reset the frame
    frame_for_time = 0;

    if ( m_dllAvFormat.av_seek_frame( pFormatCtx, videoStream, 0, 0 ) < 0 )
      return;

    m_dllAvCodec.avcodec_flush_buffers( pCodecCtx );
  }

  // We got a frame. Draw it.
  m_texture->Update( m_width, m_height, m_width * 4, XB_FMT_A8R8G8B8, pFrameRGB->data[0], false );
  
  // Get screen coordinates
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  CRect vertCoords((float)g_settings.m_ResInfo[res].Overscan.left,
                   (float)g_settings.m_ResInfo[res].Overscan.top,
                   (float)g_settings.m_ResInfo[res].Overscan.right,
                   (float)g_settings.m_ResInfo[res].Overscan.bottom);

  CGUITexture::DrawQuad(vertCoords, 0xffffffff, m_texture );
}

bool KaraokeVideoFFMpeg::Start( const CStdString& filename )
{
  if ( !filename.empty() )
  {
     if ( !openVideoFile( filename ) )
       return false;

     m_timeBase = 0;
  }
  
  if ( m_curVideoFile.empty() )
  {
     if ( !openVideoFile( g_advancedSettings.m_karaokeDefaultBackgroundFilePath ) )
       return false;

     m_timeBase = 0;
  }
  
  m_timeBase = m_lastTimeFrame;
  m_lastTimeFrame = 0;
  
  // Allocate the texture
  m_texture = new CTexture( pCodecCtx->width, pCodecCtx->height, XB_FMT_A8R8G8B8 );
  
  if ( !m_texture )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate texture" );
    return false;
  }
  
  return true;
}

void KaraokeVideoFFMpeg::Stop()
{
  delete m_texture;
  m_texture = 0;
}
