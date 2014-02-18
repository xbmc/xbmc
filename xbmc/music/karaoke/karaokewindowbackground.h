#ifndef KARAOKEWINDOWBACKGROUND_H
#define KARAOKEWINDOWBACKGROUND_H

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

// C++ Interface: karaokewindowbackground

#include "cores/IPlayerCallback.h"

class CGUIWindow;
class CGUIImage;
class CGUIVisualisationControl;
class KaraokeVideoBackground;

class CKaraokeWindowBackground : public IPlayerCallback
{
public:
  CKaraokeWindowBackground();
  ~CKaraokeWindowBackground();

  virtual void Init(CGUIWindow * wnd);

  // Start with empty background
  virtual void StartEmpty();

  // Start with visualisation background
  virtual void StartVisualisation();

  // Start with song-specific still image background
  virtual void StartImage( const std::string& path );

  // Start with song-specific video background
  virtual void StartVideo( const std::string& path = "" );

  // Start with default (setting-specific) background
  virtual void StartDefault();

  // Pause or continue the background
  virtual void Pause( bool now_paused );

  // Stop any kind of background
  virtual void Stop();

  // Function forwarders
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  // IPlayer callbacks
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();

private:
  enum BackgroundMode
  {
      BACKGROUND_NONE,
      BACKGROUND_VISUALISATION,
      BACKGROUND_IMAGE,
      BACKGROUND_VIDEO
  };

  // This critical section protects all variables except m_videoEnded
  CCriticalSection          m_CritSectionShared;

  // for visualization background
  CGUIVisualisationControl * m_VisControl;
  CGUIImage                * m_ImgControl;

  BackgroundMode             m_currentMode;

  // Parent window pointer
  CGUIWindow               * m_parentWindow;

  // Video player pointer
  KaraokeVideoBackground   * m_videoPlayer;

  // For default visualisation mode
  BackgroundMode             m_defaultMode;
  std::string                 m_path; // image
};

#endif
