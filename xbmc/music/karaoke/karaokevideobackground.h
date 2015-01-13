#ifndef KARAOKEVIDEOFFMPEG_H
#define KARAOKEVIDEOFFMPEG_H

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
class FFmpegVideoDecoder;

// C++ Interface: karaokevideoffmpeg
// Contact: oldnemesis
//
// FFMpeg-based background video decoder for Karaoke background.
// We are not using DVDPlayer for this because:
// 1. DVDPlayer was not designed to run at the same time when music is being played and other things (like lyrics) rendered. 
// While this setup works from time to time, it constantly gets broken. Some modes, like VDPAU, lead to crash right away.
//
// 2. We do not need to decode audio, hence we don't have to use extra CPU.
//
// 3. We do not really care about frame rate. Jerky video is fine for the background. Lyrics sync is much more important.
//
class KaraokeVideoBackground
{
public:
  KaraokeVideoBackground();
  ~KaraokeVideoBackground();
  
  // Start playing the video. It is called each time a new song is being played. Should continue playing existing 
  // video from the position it was paused. If it returns false, the video rendering is disabled and 
  // KaraokeVideoFFMpeg object is deleted. Must write the reason for failure into the log file.
  bool Start( const std::string& filename = "" );

  // Render the current frame into the screen. This function also must handle video loops and 
  // switching to the next video when necessary. Hence it shouldn't take too long.
  void Render();

  // Stops playing the video. It is called once the song is finished and the Render() is not going to be called anymore.
  // The object, however, is kept and should keep its state because it must continue on next Start() call.
  void Stop();
  
private:
  // Initialize the object. This function is called only once when the object is created or after it has been dismissed. 
  // If it returns false, the video rendering is disabled and KaraokeVideoFFMpeg object is deleted
  bool Init();

  // Dismisses the object, freeing all the memory and unloading the libraries. The object must be inited before using again.
  void Dismiss();

  bool openVideoFile( const std::string& filename );
  void closeVideoFile();
  
  // FFMpeg objects
  FFmpegVideoDecoder * m_decoder;

  std::string       m_curVideoFile;
  int              m_videoWidth;   // shown video width, i.e. upscaled or downscaled as necessary
  int              m_videoHeight;  // shown video height, i.e. upscaled or downscaled as necessary
  int              m_displayLeft, m_displayRight, m_displayTop, m_displayBottom;  // Video as shown at the display
  double           m_millisecondsPerFrame;
  double           m_nextFrameTime;
  double           m_timeFromPrevSong;
  
  CBaseTexture    *m_texture;
};

#endif
