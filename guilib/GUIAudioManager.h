#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/CriticalSection.h"
#include "utils/log.h"
#include "StdString.h"

#include <map>

// forward definitions
class CAction;
class CGUISound;
class TiXmlNode;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager
{
  class CWindowSounds
  {
  public:
    CStdString strInitFile;
    CStdString strDeInitFile;
  };

public:
  CGUIAudioManager();
          ~CGUIAudioManager();

          void        Initialize(int iDevice);
          void        DeInitialize(int iDevice);

          bool        Load();

          void        PlayActionSound(const CAction& action);
          void        PlayWindowSound(int id, WINDOW_SOUND event);
          void        PlayPythonSound(const CStdString& strFileName);

          void        FreeUnused();

          void        Enable(bool bEnable);
          void        SetVolume(int iLevel);
          void        Stop();
private:
          bool        LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile);

  typedef std::map<int, CStdString> actionSoundMap;
  typedef std::map<int, CWindowSounds> windowSoundMap;

  typedef std::map<CStdString, CGUISound*> pythonSoundsMap;
  typedef std::map<int, CGUISound*> windowSoundsMap;

  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;

  CGUISound*          m_actionSound;
  windowSoundsMap     m_windowSounds;
  pythonSoundsMap     m_pythonSounds;

  CStdString          m_strMediaDir;
  bool                m_bInitialized;
  bool                m_bEnabled;

  CCriticalSection    m_cs;
};

extern CGUIAudioManager g_audioManager;
