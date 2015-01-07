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

#include "karaokevideobackground.h"

#include "guilib/GraphicContext.h"
#include "guilib/Texture.h"
#include "guilib/GUITexture.h"
#include "Application.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "video/FFmpegVideoDecoder.h"
#include "system.h"
#include "utils/log.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

KaraokeVideoBackground::KaraokeVideoBackground()
{
  m_decoder = new FFmpegVideoDecoder();
  m_timeFromPrevSong = 0.0;
  m_texture = 0;
}

KaraokeVideoBackground::~KaraokeVideoBackground()
{
  delete m_decoder;
  delete m_texture;
}

bool KaraokeVideoBackground::openVideoFile( const std::string& filename )
{
  std::string realPath = CSpecialProtocol::TranslatePath( filename );

  if ( !m_decoder->open( realPath ) )
  {
	CLog::Log( LOGERROR, "Karaoke Video Background: %s, video file %s (%s)", m_decoder->getErrorMsg().c_str(), filename.c_str(), realPath.c_str() );
    return false;
  }

  m_videoWidth = m_decoder->getWidth();
  m_videoHeight = m_decoder->getHeight();
  m_curVideoFile = filename;
  
  // Find out the necessary aspect ratio for height (assuming fit by width) and width (assuming fit by height)
  const RESOLUTION_INFO info = g_graphicsContext.GetResInfo();
  m_displayLeft   = info.Overscan.left;
  m_displayRight  = info.Overscan.right;
  m_displayTop    = info.Overscan.top;
  m_displayBottom = info.Overscan.bottom;
  
  int screen_width = m_displayRight - m_displayLeft;
  int screen_height = m_displayBottom - m_displayTop;

  // Do we need to modify the output video size? This could happen in two cases:
  // 1. Either video dimension is larger than the screen - video needs to be downscaled
  // 2. Both video dimensions are smaller than the screen - video needs to be upscaled
  if ( ( m_videoWidth > 0 && m_videoHeight > 0 )
  && ( ( m_videoWidth > screen_width || m_videoHeight > screen_height )
  || ( m_videoWidth < screen_width && m_videoHeight < screen_height ) ) )
  {
    // Calculate the scale coefficients for width/height separately
    double scale_width = (double) screen_width / (double) m_videoWidth;
    double scale_height = (double) screen_height / (double) m_videoHeight;

    // And apply the smallest
    double scale = scale_width < scale_height ? scale_width : scale_height;
    m_videoWidth = (int) (m_videoWidth * scale);
    m_videoHeight = (int) (m_videoHeight * scale);
  }

  // Calculate the desktop dimensions to show the video
  if ( m_videoWidth < screen_width || m_videoHeight < screen_height )
  {
    m_displayLeft = (screen_width - m_videoWidth) / 2;
    m_displayRight -= m_displayLeft;

    m_displayTop = (screen_height - m_videoHeight) / 2;
    m_displayBottom -= m_displayTop;
  }
  
  m_millisecondsPerFrame = 1.0 / m_decoder->getFramesPerSecond();

  CLog::Log( LOGDEBUG, "Karaoke Video Background: Video file %s (%dx%d) length %g seconds opened successfully, will be shown as %dx%d at (%d, %d - %d, %d) rectangle",
             filename.c_str(), 
			 m_decoder->getWidth(), m_decoder->getHeight(),
			 m_decoder->getDuration(),
			 m_videoWidth, m_videoHeight,
             m_displayLeft, m_displayTop, m_displayRight, m_displayBottom );
  
  return true;
}

void KaraokeVideoBackground::closeVideoFile()
{
  m_decoder->close();
}

void KaraokeVideoBackground::Render()
{
  // Just in case
  if ( !m_texture )
    return;
  
  // Get the current song timing in ms.
  // This will only fit songs up to 71,000 hours, so if you got a larger one, change to int64
  double current = g_application.GetTime();

  // We're supposed to show m_decoder->getFramesPerSecond() frames in one second.
  if ( current >= m_nextFrameTime )
  {
	// We don't care to adjust for the exact timing as we don't worry about the exact frame rate
	m_nextFrameTime = current + m_millisecondsPerFrame - (current - m_nextFrameTime);

	while ( true )
	{
	  if ( !m_decoder->nextFrame( m_texture ) )
	  {
		// End of video; restart
		m_decoder->seek( 0.0 );
		m_nextFrameTime = 0.0;
		continue;
	  }

	  break;
	}
  }

  // We got a frame. Draw it.
  CRect vertCoords((float) m_displayLeft, (float) m_displayTop, (float) m_displayRight, (float) m_displayBottom );
  CGUITexture::DrawQuad(vertCoords, 0xffffffff, m_texture );
}

bool KaraokeVideoBackground::Start( const std::string& filename )
{
  if ( !filename.empty() )
  {
     if ( !openVideoFile( filename ) )
       return false;

	 m_timeFromPrevSong = 0;
  }
  else
  {
     if ( !openVideoFile( g_advancedSettings.m_karaokeDefaultBackgroundFilePath ) )
       return false;

	 if ( m_timeFromPrevSong != 0.0 && !m_decoder->seek( m_timeFromPrevSong ) )
	   m_timeFromPrevSong = 0;
  }
  
  // Allocate the texture
  m_texture = new CTexture( m_videoWidth, m_videoHeight, XB_FMT_A8R8G8B8 );
  
  if ( !m_texture )
  {
    CLog::Log( LOGERROR, "Karaoke Video Background: Could not allocate texture" );
    return false;
  }
  
  m_nextFrameTime = 0.0;
  return true;
}

void KaraokeVideoBackground::Stop()
{
  delete m_texture;
  m_texture = 0;

  m_timeFromPrevSong = m_decoder->getLastFrameTime();
}
