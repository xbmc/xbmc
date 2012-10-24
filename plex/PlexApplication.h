/*
 *  PlexApplication.h
 *  XBMC
 *
 *  Created by Jamie Kirkpatrick on 20/01/2011.
 *  Copyright 2011 Plex Inc. All rights reserved.
 *
 */

#pragma once

#include "guilib/IMsgTargetCallback.h"
#include "AutoUpdate/PlexAutoUpdate.h"

class PlexApplication;
class PlexServiceListener;
class BackgroundMusicPlayer;

typedef boost::shared_ptr<PlexApplication> PlexApplicationPtr;
typedef boost::shared_ptr<PlexServiceListener> PlexServiceListenerPtr;
typedef boost::shared_ptr<BackgroundMusicPlayer > BackgroundMusicPlayerPtr;

///
/// The hub of all Plex goodness.
///
class PlexApplication : public IMsgTargetCallback
{
public:
  /// Create an instance.
  static PlexApplicationPtr Create();

  /// Destructor
  virtual ~PlexApplication();
  
  /// Handle internal messages.
  virtual bool OnMessage(CGUIMessage& message);
  
  /// Handle global volume changes.
  void SetGlobalVolume(int volume);
    
protected:
  /// Default constructor.
  PlexApplication();
  
private:
  /// Members
  PlexServiceListenerPtr m_serviceListener;
  BackgroundMusicPlayerPtr m_bgMusicPlayer;
  PlexAutoUpdate m_autoUpdater;
};
