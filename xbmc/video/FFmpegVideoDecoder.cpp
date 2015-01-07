/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "guilib/Texture.h"

#include "FFmpegVideoDecoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

FFmpegVideoDecoder::FFmpegVideoDecoder()
{
  m_pFormatCtx = 0;
  m_pCodecCtx = 0;
  m_pCodec = 0;
  m_pFrame = 0;
  m_pFrameRGB = 0;
}

FFmpegVideoDecoder::~FFmpegVideoDecoder()
{
  close();
}

void FFmpegVideoDecoder::close()
{
  // Free the YUV frame
  if ( m_pFrame )
	av_free( m_pFrame );

  // Free the RGB frame
  if ( m_pFrameRGB )
  {
	avpicture_free( m_pFrameRGB );
	av_free( m_pFrameRGB );
  }

  // Close the codec
  if ( m_pCodecCtx )
	avcodec_close( m_pCodecCtx );

  // Close the video file
  if ( m_pFormatCtx )
	avformat_close_input( &m_pFormatCtx );

  m_pFormatCtx = 0;
  m_pCodecCtx = 0;
  m_pCodec = 0;
  m_pFrame = 0;
  m_pFrameRGB = 0;
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
#if defined(AVFORMAT_HAS_STREAM_GET_R_FRAME_RATE)
  return m_pFormatCtx ? av_q2d( av_stream_get_r_frame_rate( m_pFormatCtx->streams[ m_videoStream ] ) ) : 0.0;
#else
  return m_pFormatCtx ? av_q2d( m_pFormatCtx->streams[ m_videoStream ]->r_frame_rate ) : 0.0;
#endif
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

std::string FFmpegVideoDecoder::getErrorMsg() const
{
  return m_errorMsg;
}

double FFmpegVideoDecoder::getLastFrameTime() const
{
  return m_lastFrameTime;
}


bool FFmpegVideoDecoder::open( const std::string& filename )
{
  // See http://dranger.com/ffmpeg/tutorial01.html
  close();
  
  // Open the video file
  if ( avformat_open_input( &m_pFormatCtx, filename.c_str(), NULL, NULL ) < 0 )
  {
    m_errorMsg = "Could not open the video file";
   close();
    return false;
  }

  // Retrieve the stream information
  if ( avformat_find_stream_info( m_pFormatCtx, 0 ) < 0 )
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
  m_pCodec = avcodec_find_decoder( m_pCodecCtx->codec_id );

  if ( m_pCodec == NULL )
  {
    m_errorMsg = "Could not find a video decoder";
    close();
    return false;
  }
    
  // Open the codec
  if ( avcodec_open2( m_pCodecCtx, m_pCodec, 0 ) < 0 )
  {
    m_errorMsg = "Could not open the video decoder";
    close();
    return false;
  }
  
  // Allocate video frames
  m_pFrame = av_frame_alloc();

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

  if ( av_seek_frame( m_pFormatCtx, m_videoStream, timestamp, AVSEEK_FLAG_ANY ) < 0 )
	return false;

  avcodec_flush_buffers( m_pCodecCtx );
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
      avpicture_free( m_pFrameRGB );
      av_free( m_pFrameRGB );
    }

    m_frameRGBwidth = texture->GetWidth();
    m_frameRGBheight = texture->GetHeight();

    // Allocate the conversion frame and relevant picture
    m_pFrameRGB = (AVPicture*)av_mallocz(sizeof(AVPicture));

    if ( !m_pFrameRGB )
      return false;

    // Due to a bug in swsscale we need to allocate one extra line of data
    if ( avpicture_alloc( m_pFrameRGB, PIX_FMT_RGB32, m_frameRGBwidth, m_frameRGBheight + 1 ) < 0 )
      return false;
  }

  AVPacket packet;
  int frameFinished;

  while ( true )
  {
    // Read a frame
    if ( av_read_frame( m_pFormatCtx, &packet ) < 0 )
      return false;  // Frame read failed (e.g. end of stream)

    if ( packet.stream_index == m_videoStream )
    {
      // Is this a packet from the video stream -> decode video frame
      avcodec_decode_video2( m_pCodecCtx, m_pFrame, &frameFinished, &packet );

      // Did we get a video frame?
      if ( frameFinished )
      {
        if ( packet.dts != (int64_t)AV_NOPTS_VALUE )
	  m_lastFrameTime = packet.dts * av_q2d( m_pFormatCtx->streams[ m_videoStream ]->time_base );
        else
	   m_lastFrameTime = 0.0;

	break;
      }
    }

    av_free_packet( &packet );
  }

  // We got the video frame, render it into the picture buffer
  struct SwsContext * context = sws_getContext( m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
                           m_frameRGBwidth, m_frameRGBheight, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL );

  sws_scale( context, m_pFrame->data, m_pFrame->linesize, 0, m_pCodecCtx->height,
                                                                     m_pFrameRGB->data, m_pFrameRGB->linesize );
  sws_freeContext( context );
  av_free_packet( &packet );

  // And into the texture
  texture->Update( m_frameRGBwidth, m_frameRGBheight, m_frameRGBwidth * 4, XB_FMT_A8R8G8B8, m_pFrameRGB->data[0], false );

  return true;
}

