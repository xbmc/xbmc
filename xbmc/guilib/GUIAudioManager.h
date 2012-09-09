#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "threads/CriticalSection.h"
#include "utils/log.h"
#include "utils/StdString.h"
#include "cores/AudioEngine/Interfaces/AESound.h"

#include <map>

// forward definitions
class CAction;
class TiXmlNode;
class IAESound;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager
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
  ~CGUIAudioManager();

  void Initialize();
  void DeInitialize();

  bool Load();
  void UnLoad();


  void PlayActionSound(const CAction& action);
  void PlayWindowSound(int id, WINDOW_SOUND event);
  void PlayPythonSound(const CStdString& strFileName);

  void Enable(bool bEnable);
  void SetVolume(float level);
  void Stop();
private:
  typedef std::map<const CStdString, CSoundInfo> soundCache;
  typedef std::map<int, IAESound*              > actionSoundMap;
  typedef std::map<int, CWindowSounds          > windowSoundMap;
  typedef std::map<const CStdString, IAESound* > pythonSoundsMap;

  soundCache          m_soundCache;
  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;
  pythonSoundsMap     m_pythonSounds;

  CStdString          m_strMediaDir;
  bool                m_bEnabled;

  CCriticalSection    m_cs;

  IAESound* LoadSound(const CStdString &filename);
  void      FreeSound(IAESound *sound);
  IAESound* LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier);
};

extern CGUIAudioManager g_audioManager;
