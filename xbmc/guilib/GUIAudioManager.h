#pragma once

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

#include <map>

#ifndef GUILIB_CORES_AUDIOENGINE_INTERFACES_AESOUND_H_INCLUDED
#define GUILIB_CORES_AUDIOENGINE_INTERFACES_AESOUND_H_INCLUDED
#include "cores/AudioEngine/Interfaces/AESound.h"
#endif

#ifndef GUILIB_SETTINGS_LIB_ISETTINGCALLBACK_H_INCLUDED
#define GUILIB_SETTINGS_LIB_ISETTINGCALLBACK_H_INCLUDED
#include "settings/lib/ISettingCallback.h"
#endif

#ifndef GUILIB_THREADS_CRITICALSECTION_H_INCLUDED
#define GUILIB_THREADS_CRITICALSECTION_H_INCLUDED
#include "threads/CriticalSection.h"
#endif

#ifndef GUILIB_UTILS_LOG_H_INCLUDED
#define GUILIB_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef GUILIB_UTILS_STDSTRING_H_INCLUDED
#define GUILIB_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif


// forward definitions
class CAction;
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
  ~CGUIAudioManager();

  virtual void OnSettingChanged(const CSetting *setting);

  void Initialize();
  void DeInitialize();

  bool Load();
  void UnLoad();


  void PlayActionSound(const CAction& action);
  void PlayWindowSound(int id, WINDOW_SOUND event);
  void PlayPythonSound(const CStdString& strFileName, bool useCached = true);

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
  void      FreeSoundAllUsage(IAESound *sound);
  IAESound* LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier);
};

extern CGUIAudioManager g_audioManager;
