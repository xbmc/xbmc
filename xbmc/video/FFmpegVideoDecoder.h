#ifndef FFMPEGVIDEODECODER_H
#define FFMPEGVIDEODECODER_H

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

#include <string>

class CBaseTexture;

extern "C" {
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPicture;
}

/**
 * A simple FFMpeg-based background video decoder.
 * 
 * This class only decodes the video using the standard FFmpeg calls, so likely no hardware acceleration.
 * No audio decoding, and no rendering.
 * 
 */
class FFmpegVideoDecoder
{
public:
  FFmpegVideoDecoder();
  ~FFmpegVideoDecoder();

  /**
   * Opens the video file for decoding. Supports all the formats supported by the used FFmpeg.
   * The file must have at least one video track. If it has more than one, the first video track
   * would be decoded.
   * 
   * If an existing stream was opened, it is automatically closed and the new stream is opened.
   * 
   * Returns true if the file was opened successfully, and false otherwise, in which case
   * the getErrorMsg() function could be used to retrieve the reason.
   * 
   * @param filename The video file name, which must be translated through CSpecialProtocol::TranslatePath()
   */
  bool open( const std::string& filename );

  /**
   * Returns true if the decoder has the video file opened.
   */
  bool isOpened() const;

  /**
   * Returns the movie duration in seconds or 0.0 if the duration is not available. For some formats
   * is calculated through the heuristics, and the video might not really be that long (for example if it is incomplete).
   * The total number of frames is calculated by multiplying the duration by getFramesPerSecond()
   */
  double getDuration() const;
  
  /**
   * Returns the frames per second for this video
   */
  double getFramesPerSecond() const;
  
  /**
   * Returns the original video width or 0 if the video wasn't opened.
   */  
  unsigned int getWidth() const;

  /**
   * Returns the original video height or 0 if the video wasn't opened.
   */  
  unsigned int getHeight() const;

  /**
   * Returns the last rendered frame number.
   */
  double getLastFrameTime() const;

  /**
   * Returns the AVFormatContext structure associated with this video format
   */  
  const AVFormatContext * getAVFormatContext() const;

  /**
   * Returns the AVCodecContext structure associated with this video codec
   */  
  const AVCodecContext * getAVCodecContext() const;
  
  /**
   * Returns the AVCodec structure associated with this video codec
   */  
  const AVCodec * getAVCodec() const;

  /**
   * Returns the error message text if opening the video failed
   */  
  std::string getErrorMsg() const;

  /**
   * Decodes and renders the next video frame into the provided texture which
   * must be in a XB_FMT_A8R8G8B8 format.
   * The frame will be rescaled to fit the whole texture (i.e. texture width/height)
   * so make sure the texture aspect ratio is the same as in the original movie.
   * 
   * @param texture The texture to render the frame into. Must be preallocated, 
   * have the appropriate width/height and in XB_FMT_A8R8G8B8 format.
   * 
   * @return true if the frame rendered, false if there are no more frames
   */  
  bool nextFrame( CBaseTexture * texture );

  /**
	* Seeks to a specific time position in the video file. Note that the seek is limited to the keyframes only.
	* @param time The time to seek to, in seconds
	* @return true if the seek succeed, false if failed
	*/
  bool seek( double time );

  /**
   * Closes the video stream.
   */  
  void close();

private:
  bool readFrame( int frame );
  
  AVFormatContext *m_pFormatCtx;
  AVCodecContext  *m_pCodecCtx;
  AVCodec         *m_pCodec;
  AVFrame         *m_pFrame;
  AVPicture       *m_pFrameRGB;
  int              m_videoStream;
  double           m_lastFrameTime;
  
  // The dimensions of the allocated pFrameRGB
  unsigned int     m_frameRGBwidth;
  unsigned int     m_frameRGBheight;
  
  std::string       m_errorMsg;
};

#endif
