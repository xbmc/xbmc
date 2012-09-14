//
//  BackgroundMusicPlayer.h
//  Plex
//
//  Created by Jamie Kirkpatrick on 29/01/2011.
//  Copyright 2011 Kirk Consulting Limited. All rights reserved.
//

#pragma once

#include <boost/shared_ptr.hpp>

#include "IPlayer.h"
#include "StdString.h"

#define GUI_MSG_BG_MUSIC_THEME_UPDATED GUI_MSG_USER + 46

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
  void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength){};
  
  // Member variables.
  int m_globalVolume;
  CStdString m_theme;
  PlayerPtr m_player;
};
