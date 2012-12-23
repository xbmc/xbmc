/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "DllAvFormat.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"
#include "DllSwScale.h"
#include "guilib/Texture.h"

#include "FFmpegVideoDecoder.h"


FFmpegVideoDecoder::FFmpegVideoDecoder()
{
  m_pFormatCtx = 0;
  m_pCodecCtx = 0;
  m_pCodec = 0;
  m_pFrame = 0;
  m_pFrameRGB = 0;
  
  m_dllAvFormat = new DllAvFormat();
  m_dllAvCodec = new DllAvCodec();
  m_dllAvUtil = new DllAvUtil();
  m_dllSwScale = new DllSwScale();
}

FFmpegVideoDecoder::~FFmpegVideoDecoder()
{
  close();
  
  delete m_dllAvFormat;
  delete m_dllAvCodec;
  delete m_dllAvUtil;
  delete m_dllSwScale;
}

void FFmpegVideoDecoder::close()
{
  // Free the YUV frame
  if ( m_pFrame )
	m_dllAvUtil->av_free( m_pFrame );

  // Free the RGB frame
  if ( m_pFrameRGB )
  {
	m_dllAvCodec->avpicture_free( m_pFrameRGB );
	m_dllAvUtil->av_free( m_pFrameRGB );
  }

  // Close the codec
  if ( m_pCodecCtx )
	m_dllAvCodec->avcodec_close( m_pCodecCtx );

  // Close the video file
  if ( m_pFormatCtx )
	m_dllAvFormat->avformat_close_input( &m_pFormatCtx );

  m_pFormatCtx = 0;
  m_pCodecCtx = 0;
  m_pCodec = 0;
  m_pFrame = 0;
  m_pFrameRGB = 0;
  
  if ( m_dllAvCodec->IsLoaded() )
    m_dllAvCodec->Unload();
  
  if ( m_dllAvUtil->IsLoaded() )
    m_dllAvUtil->Unload();
  
  if ( m_dllSwScale->IsLoaded() )
    m_dllSwScale->Unload();
  
  if ( m_dllAvFormat->IsLoaded() )
    m_dllAvFormat->Unload();
}

bool FFmpegVideoDecoder::isOpened() const
{
  return m_pFrame ? true : false;
}

double FFmpegVideoDecoder::getDuration() const
{
  if ( m_pFormatCtx && m_pFormatCtx->duration / AV_TIME_BASE > 0.0 )
	return m_pFormatCtx->duration / AV_TIME_BASE;

  return 0.0;
}
  
double FFmpegVideoDecoder::getFramesPerSecond() const
{
  return m_pFormatCtx ? av_q2d( m_pFormatCtx->streams[ m_videoStream ]->r_frame_rate ) : 0.0;
}
  
unsigned int FFmpegVideoDecoder::getWidth() const
{
  if ( !m_pCodecCtx )
    return 0;

  return m_pCodecCtx->width;
}

unsigned int FFmpegVideoDecoder::getHeight() const
{
  if ( !m_pCodecCtx )
    return 0;

  return m_pCodecCtx->height;
}

const AVFormatContext * FFmpegVideoDecoder::getAVFormatContext() const
{
  return m_pFormatCtx;
}

const AVCodecContext * FFmpegVideoDecoder::getAVCodecContext() const
{
  return m_pCodecCtx;
}

const AVCodec * FFmpegVideoDecoder::getAVCodec() const
{
  return m_pCodec;
}

CStdString FFmpegVideoDecoder::getErrorMsg() const
{
  return m_errorMsg;
}

double FFmpegVideoDecoder::getLastFrameTime() const
{
  return m_lastFrameTime;
}


bool FFmpegVideoDecoder::open( const CStdString& filename )
{
  // See http://dranger.com/ffmpeg/tutorial01.html
  close();
  
  if ( !m_dllAvUtil->Load() || !m_dllAvCodec->Load() || !m_dllSwScale->Load() || !m_dllAvFormat->Load() )
  {
    m_errorMsg = "Failed to load FFMpeg libraries";
    return false;
  }

  m_dllAvCodec->avcodec_register_all();
  m_dllAvFormat->av_register_all();

  // Open the video file
  if ( m_dllAvFormat->avformat_open_input( &m_pFormatCtx, filename.c_str(), NULL, NULL ) < 0 )
  {
    m_errorMsg = "Could not open the video file";
   close();
    return false;
  }

  // Retrieve the stream information
  if ( m_dllAvFormat->avformat_find_stream_info( m_pFormatCtx, 0 ) < 0 )
  {
    m_errorMsg = "Could not find the stream information";
    close();
    return false;
  }

  // Find the first video stream
  m_videoStream = -1;

  for ( unsigned i = 0; i < m_pFormatCtx->nb_streams; i++ )
  {
    if ( m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
    {
      m_videoStream = i;
      break;
    }
  }

  if ( m_videoStream == -1 )
  {
    m_errorMsg = "Could not find a playable video stream";
    close();
    return false;
  }

  // Get a pointer to the codec context for the video stream
  m_pCodecCtx = m_pFormatCtx->streams[ m_videoStream ]->codec;

  // Find the decoder for the video stream
  m_pCodec = m_dllAvCodec->avcodec_find_decoder( m_pCodecCtx->codec_id );

  if ( m_pCodec == NULL )
  {
    m_errorMsg = "Could not find a video decoder";
    close();
    return false;
  }
    
  // Open the codec
  if ( m_dllAvCodec->avcodec_open2( m_pCodecCtx, m_pCodec, 0 ) < 0 )
  {
    m_errorMsg = "Could not open the video decoder";
    close();
    return false;
  }
  
  // Allocate video frames
  m_pFrame = m_dllAvCodec->avcodec_alloc_frame();

  if ( !m_pFrame )
  {
    m_errorMsg = "Could not allocate memory for a frame";
    close();
    return false;
  }
  
  return true;
}


bool FFmpegVideoDecoder::seek( double time )
{
  // Convert the frame number into time stamp
  int64_t timestamp = (int64_t) (time * AV_TIME_BASE * av_q2d( m_pFormatCtx->streams[ m_videoStream ]->time_base ));

  if ( m_dllAvFormat->av_seek_frame( m_pFormatCtx, m_videoStream, timestamp, AVSEEK_FLAG_ANY ) < 0 )
	return false;

  m_dllAvCodec->avcodec_flush_buffers( m_pCodecCtx );
  return true;
}

bool FFmpegVideoDecoder::nextFrame( CBaseTexture * texture )
{
  // Just in case
  if ( !m_pCodecCtx )
	return false;

  // If we did not preallocate the picture or the texture size changed, (re)allocate it
  if ( !m_pFrameRGB || texture->GetWidth() != m_frameRGBwidth || texture->GetHeight() != m_frameRGBheight )
  {
    if ( m_pFrameRGB )
    {
      m_dllAvCodec->avpicture_free( m_pFrameRGB );
      m_dllAvUtil->av_free( m_pFrameRGB );
    }

    m_frameRGBwidth = texture->GetWidth();
    m_frameRGBheight = texture->GetHeight();

    // Allocate the conversion frame and relevant picture
    m_pFrameRGB = (AVPicture*)m_dllAvUtil->av_mallocz(sizeof(AVPicture));

    if ( !m_pFrameRGB )
      return false;

    // Due to a bug in swsscale we need to allocate one extra line of data
    if ( m_dllAvCodec->avpicture_alloc( m_pFrameRGB, PIX_FMT_RGB32, m_frameRGBwidth, m_frameRGBheight + 1 ) < 0 )
      return false;
  }

  AVPacket packet;
  int frameFinished;

  while ( true )
  {
    // Read a frame
    if ( m_dllAvFormat->av_read_frame( m_pFormatCtx, &packet ) < 0 )
      return false;  // Frame read failed (e.g. end of stream)

    if ( packet.stream_index == m_videoStream )
    {
      // Is this a packet from the video stream -> decode video frame
      m_dllAvCodec->avcodec_decode_video2( m_pCodecCtx, m_pFrame, &frameFinished, &packet );

      // Did we get a video frame?
      if ( frameFinished )
      {
        if ( packet.dts != AV_NOPTS_VALUE )
	  m_lastFrameTime = packet.dts * av_q2d( m_pFormatCtx->streams[ m_videoStream ]->time_base );
        else
	   m_lastFrameTime = 0.0;

	break;
      }
    }

    m_dllAvCodec->av_free_packet( &packet );
  }

  // We got the video frame, render it into the picture buffer
  struct SwsContext * context = m_dllSwScale->sws_getContext( m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
                           m_frameRGBwidth, m_frameRGBheight, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL );

  m_dllSwScale->sws_scale( context, m_pFrame->data, m_pFrame->linesize, 0, m_pCodecCtx->height, 
                                                                     m_pFrameRGB->data, m_pFrameRGB->linesize );
  m_dllSwScale->sws_freeContext( context );
  m_dllAvCodec->av_free_packet( &packet );

  // And into the texture
  texture->Update( m_frameRGBwidth, m_frameRGBheight, m_frameRGBwidth * 4, XB_FMT_A8R8G8B8, m_pFrameRGB->data[0], false );

  return true;
}

