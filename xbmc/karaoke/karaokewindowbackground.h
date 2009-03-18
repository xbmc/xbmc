//
// C++ Interface: karaokewindowbackground
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef KARAOKEWINDOWBACKGROUND_H
#define KARAOKEWINDOWBACKGROUND_H

#include "cores/IPlayer.h"

class CGUIWindow;
class CGUImage;
class CGUIVisualisationControl;
class CDVDPlayer;

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
  virtual void StartImage( const CStdString& path );

  // Start with song-specific video background
  virtual void StartVideo( const CStdString& path, __int64 offset = 0 );

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

  // for visualization background
  CGUIVisualisationControl * m_VisControl;
  CGUIImage                * m_ImgControl;

  BackgroundMode             m_currentMode;

  // Parent window pointer
  CGUIWindow               * m_parentWindow;

  // Video player pointer
  CDVDPlayer               * m_videoPlayer;

  // For default visualisation mode
  BackgroundMode             m_defaultMode;
  CStdString                 m_path; // image or video
  __int64                    m_videoLastTime; // video only
  bool                       m_playingDefaultVideo; // whether to store the time
};

#endif
