//
//  BackgroundMusicPlayer.h
//  Plex
//
//  Created by Jamie Kirkpatrick on 29/01/2011.
//  Copyright 2011 Kirk Consulting Limited. All rights reserved.
//

#pragma once

#include <boost/shared_ptr.hpp>

#include "cores/IPlayer.h"
#include "StdString.h"
#include "PlexTypes.h"

class BackgroundMusicPlayer;
typedef boost::shared_ptr<BackgroundMusicPlayer> BackgroundMusicPlayerPtr;
typedef boost::shared_ptr<IPlayer> PlayerPtr;

//
// Utility class for playing background theme music.
//
class BackgroundMusicPlayer : public IPlayerCallback, IAudioCallback
{
public:
  // Factory method.
  static BackgroundMusicPlayerPtr Create();
  
  // Convenience method to send a background theme change message.
  static void SendThemeChangeMessage(const CStdString& theme = CStdString());
  
  // Destructor
  virtual ~BackgroundMusicPlayer();
  
  // Set the global volume as a percentage of total possible volume.
  void SetGlobalVolumeAsPercent(int volume);
  
  // Set the currently active theme id.
  void SetTheme(const CStdString& theme);
  
  // Play the currently selected theme music if there is any.
  void PlayCurrentTheme();
  
private:
  // Constructor.
  BackgroundMusicPlayer();
  
  // Player callbacks.
  void OnPlayBackEnded(){};
  void OnPlayBackStarted(){};
  void OnPlayBackStopped(){};
  void OnQueueNextItem(){};
  
  // Audio callbacks
  void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
  void OnAudioData(const float* pAudioData, int iAudioDataLength) {};
  
  // Member variables.
  int m_globalVolume;
  CStdString m_theme;
  PlayerPtr m_player;
};
