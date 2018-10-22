/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

#include "GUIComponent.h"
#include "cores/AudioEngine/Interfaces/AESound.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"

// forward definitions
class CAction;
class CSettings;
class TiXmlNode;
class IAESound;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager : public ISettingCallback
{
  class CWindowSounds
  {
  public:
    IAESound *initSound;
    IAESound *deInitSound;
  };

  class CSoundInfo
  {
  public:
    int usage;
    IAESound *sound;
  };

public:
  CGUIAudioManager();
  ~CGUIAudioManager() override;

  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  bool OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode) override;

  void Initialize();
  void DeInitialize();

  bool Load();
  void UnLoad();


  void PlayActionSound(const CAction& action);
  void PlayWindowSound(int id, WINDOW_SOUND event);
  void PlayPythonSound(const std::string& strFileName, bool useCached = true);

  void Enable(bool bEnable);
  void SetVolume(float level);
  void Stop();

private:
  // Construction parameters
  std::shared_ptr<CSettings> m_settings;

  typedef std::map<const std::string, CSoundInfo> soundCache;
  typedef std::map<int, IAESound*              > actionSoundMap;
  typedef std::map<int, CWindowSounds          > windowSoundMap;
  typedef std::map<const std::string, IAESound* > pythonSoundsMap;

  soundCache          m_soundCache;
  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;
  pythonSoundsMap     m_pythonSounds;

  std::string          m_strMediaDir;
  bool                m_bEnabled;

  CCriticalSection    m_cs;

  IAESound* LoadSound(const std::string &filename);
  void      FreeSound(IAESound *sound);
  void      FreeSoundAllUsage(IAESound *sound);
  IAESound* LoadWindowSound(TiXmlNode* pWindowNode, const std::string& strIdentifier);
};

