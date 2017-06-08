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

#include "system.h"
#include "GUIAudioManager.h"
#include "ServiceBroker.h"
#include "input/ActionIDs.h"
#include "input/ActionTranslator.h"
#include "input/ButtonTranslator.h"
#include "input/Key.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "filesystem/Directory.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "utils/log.h"

CGUIAudioManager g_audioManager;

CGUIAudioManager::CGUIAudioManager()
{
  m_bEnabled = false;
}

CGUIAudioManager::~CGUIAudioManager()
{
}

void CGUIAudioManager::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN)
  {
    Enable(true);
    Load();
  }
}

bool CGUIAudioManager::OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

  if (setting->GetId() == CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN)
  {
    //Migrate the old settings
    if (std::static_pointer_cast<CSettingString>(setting)->GetValue() == "SKINDEFAULT")
      std::static_pointer_cast<CSettingString>(setting)->Reset();
    else if (std::static_pointer_cast<CSettingString>(setting)->GetValue() == "OFF")
      std::static_pointer_cast<CSettingString>(setting)->SetValue("");
  }
  return true;
}


void CGUIAudioManager::Initialize()
{
}

void CGUIAudioManager::DeInitialize()
{
  CSingleLock lock(m_cs);
  UnLoad();
}

void CGUIAudioManager::Stop()
{
  CSingleLock lock(m_cs);
  for (windowSoundMap::iterator it = m_windowSoundMap.begin(); it != m_windowSoundMap.end(); ++it)
  {
    if (it->second.initSound  ) it->second.initSound  ->Stop();
    if (it->second.deInitSound) it->second.deInitSound->Stop();
  }

  for (pythonSoundsMap::iterator it = m_pythonSounds.begin(); it != m_pythonSounds.end(); ++it)
  {
    IAESound* sound = it->second;
    sound->Stop();
  }
}

// \brief Play a sound associated with a CAction
void CGUIAudioManager::PlayActionSound(const CAction& action)
{
  CSingleLock lock(m_cs);

  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled)
    return;

  actionSoundMap::iterator it = m_actionSoundMap.find(action.GetID());
  if (it == m_actionSoundMap.end())
    return;

  if (it->second)
    it->second->Play();
}

// \brief Play a sound associated with a window and its event
// Events: SOUND_INIT, SOUND_DEINIT
void CGUIAudioManager::PlayWindowSound(int id, WINDOW_SOUND event)
{
  CSingleLock lock(m_cs);

  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled)
    return;

  windowSoundMap::iterator it=m_windowSoundMap.find(id);
  if (it==m_windowSoundMap.end())
    return;

  CWindowSounds sounds=it->second;
  IAESound *sound = NULL;
  switch (event)
  {
  case SOUND_INIT:
    sound = sounds.initSound;
    break;
  case SOUND_DEINIT:
    sound = sounds.deInitSound;
    break;
  }

  if (!sound)
    return;

  sound->Play();
}

// \brief Play a sound given by filename
void CGUIAudioManager::PlayPythonSound(const std::string& strFileName, bool useCached /*= true*/)
{
  CSingleLock lock(m_cs);

  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled)
    return;

  // If we already loaded the sound, just play it
  pythonSoundsMap::iterator itsb=m_pythonSounds.find(strFileName);
  if (itsb != m_pythonSounds.end())
  {
    IAESound* sound = itsb->second;
    if (useCached)
    {
      sound->Play();
      return;
    }
    else
    {
      FreeSoundAllUsage(sound);
      m_pythonSounds.erase(itsb);
    }
  }

  IAESound *sound = LoadSound(strFileName);
  if (!sound)
    return;

  m_pythonSounds.insert(std::pair<const std::string, IAESound*>(strFileName, sound));
  sound->Play();
}

void CGUIAudioManager::UnLoad()
{
  //  Free sounds from windows
  {
    windowSoundMap::iterator it = m_windowSoundMap.begin();
    while (it != m_windowSoundMap.end())
    {
      if (it->second.initSound  ) FreeSound(it->second.initSound  );
      if (it->second.deInitSound) FreeSound(it->second.deInitSound);
      m_windowSoundMap.erase(it++);
    }
  }

  // Free sounds from python
  {
    pythonSoundsMap::iterator it = m_pythonSounds.begin();
    while (it != m_pythonSounds.end())
    {
      IAESound* sound = it->second;
      FreeSound(sound);
      m_pythonSounds.erase(it++);
    }
  }

  // free action sounds
  {
    actionSoundMap::iterator it = m_actionSoundMap.begin();
    while (it != m_actionSoundMap.end())
    {
      IAESound* sound = it->second;
      FreeSound(sound);
      m_actionSoundMap.erase(it++);
    }
  }
}


std::string GetSoundSkinPath()
{
  auto setting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettings().GetSetting(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN));
  auto value = setting->GetValue();
  if (value.empty())
    return "";

  ADDON::AddonPtr addon;
  if (!ADDON::CAddonMgr::GetInstance().GetAddon(value, addon, ADDON::ADDON_RESOURCE_UISOUNDS))
  {
    CLog::Log(LOGNOTICE, "Unknown sounds addon '%s'. Setting default sounds.", value.c_str());
    setting->Reset();
  }
  return URIUtils::AddFileToFolder("resource://", setting->GetValue());
}


// \brief Load the config file (sounds.xml) for nav sounds
bool CGUIAudioManager::Load()
{
  CSingleLock lock(m_cs);
  UnLoad();

  m_strMediaDir = GetSoundSkinPath();
  if (m_strMediaDir.empty())
    return true;

  Enable(true);
  std::string strSoundsXml = URIUtils::AddFileToFolder(m_strMediaDir, "sounds.xml");

  //  Load our xml file
  CXBMCTinyXML xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", strSoundsXml.c_str());

  //  Load the config file
  if (!xmlDoc.LoadFile(strSoundsXml))
  {
    CLog::Log(LOGNOTICE, "%s, Line %d\n%s", strSoundsXml.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  std::string strValue = pRoot->Value();
  if ( strValue != "sounds")
  {
    CLog::Log(LOGNOTICE, "%s Doesn't contain <sounds>", strSoundsXml.c_str());
    return false;
  }

  //  Load sounds for actions
  TiXmlElement* pActions = pRoot->FirstChildElement("actions");
  if (pActions)
  {
    TiXmlNode* pAction = pActions->FirstChild("action");

    while (pAction)
    {
      TiXmlNode* pIdNode = pAction->FirstChild("name");
      unsigned int id = ACTION_NONE;    // action identity
      if (pIdNode && pIdNode->FirstChild())
      {
        CActionTranslator::TranslateActionString(pIdNode->FirstChild()->Value(), id);
      }

      TiXmlNode* pFileNode = pAction->FirstChild("file");
      std::string strFile;
      if (pFileNode && pFileNode->FirstChild())
        strFile += pFileNode->FirstChild()->Value();

      if (id != ACTION_NONE && !strFile.empty())
      {
        std::string filename = URIUtils::AddFileToFolder(m_strMediaDir, strFile);
        IAESound *sound = LoadSound(filename);
        if (sound)
          m_actionSoundMap.insert(std::pair<int, IAESound *>(id, sound));
      }

      pAction = pAction->NextSibling();
    }
  }

  //  Load window specific sounds
  TiXmlElement* pWindows = pRoot->FirstChildElement("windows");
  if (pWindows)
  {
    TiXmlNode* pWindow = pWindows->FirstChild("window");

    while (pWindow)
    {
      int id = 0;

      TiXmlNode* pIdNode = pWindow->FirstChild("name");
      if (pIdNode)
      {
        if (pIdNode->FirstChild())
          id = CButtonTranslator::TranslateWindow(pIdNode->FirstChild()->Value());
      }

      CWindowSounds sounds;
      sounds.initSound   = LoadWindowSound(pWindow, "activate"  );
      sounds.deInitSound = LoadWindowSound(pWindow, "deactivate");

      if (id > 0)
        m_windowSoundMap.insert(std::pair<int, CWindowSounds>(id, sounds));

      pWindow = pWindow->NextSibling();
    }
  }

  return true;
}

IAESound* CGUIAudioManager::LoadSound(const std::string &filename)
{
  CSingleLock lock(m_cs);
  soundCache::iterator it = m_soundCache.find(filename);
  if (it != m_soundCache.end())
  {
    ++it->second.usage;
    return it->second.sound;
  }

  IAESound *sound = CServiceBroker::GetActiveAE().MakeSound(filename);
  if (!sound)
    return NULL;

  CSoundInfo info;
  info.usage = 1;
  info.sound = sound;
  m_soundCache[filename] = info;

  return info.sound;
}

void CGUIAudioManager::FreeSound(IAESound *sound)
{
  CSingleLock lock(m_cs);
  for(soundCache::iterator it = m_soundCache.begin(); it != m_soundCache.end(); ++it) {
    if (it->second.sound == sound) {
      if (--it->second.usage == 0) {     
        CServiceBroker::GetActiveAE().FreeSound(sound);
        m_soundCache.erase(it);
      }
      return;
    }
  }
}

void CGUIAudioManager::FreeSoundAllUsage(IAESound *sound)
{
  CSingleLock lock(m_cs);
  for(soundCache::iterator it = m_soundCache.begin(); it != m_soundCache.end(); ++it) {
    if (it->second.sound == sound) {   
      CServiceBroker::GetActiveAE().FreeSound(sound);
      m_soundCache.erase(it);
      return;
    }
  }
}

// \brief Load a window node of the config file (sounds.xml)
IAESound* CGUIAudioManager::LoadWindowSound(TiXmlNode* pWindowNode, const std::string& strIdentifier)
{
  if (!pWindowNode)
    return NULL;

  TiXmlNode* pFileNode = pWindowNode->FirstChild(strIdentifier);
  if (pFileNode && pFileNode->FirstChild())
    return LoadSound(URIUtils::AddFileToFolder(m_strMediaDir, pFileNode->FirstChild()->Value()));

  return NULL;
}

// \brief Enable/Disable nav sounds
void CGUIAudioManager::Enable(bool bEnable)
{
  // always deinit audio when we don't want gui sounds
  if (CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN).empty())
    bEnable = false;

  CSingleLock lock(m_cs);
  m_bEnabled = bEnable;
}

// \brief Sets the volume of all playing sounds
void CGUIAudioManager::SetVolume(float level)
{
  CSingleLock lock(m_cs);

  {
    actionSoundMap::iterator it = m_actionSoundMap.begin();
    while (it!=m_actionSoundMap.end())
    {
      if (it->second)
        it->second->SetVolume(level);
      ++it;
    }
  }

  for(windowSoundMap::iterator it = m_windowSoundMap.begin(); it != m_windowSoundMap.end(); ++it)
  {
    if (it->second.initSound  ) it->second.initSound  ->SetVolume(level);
    if (it->second.deInitSound) it->second.deInitSound->SetVolume(level);
  }

  {
    pythonSoundsMap::iterator it = m_pythonSounds.begin();
    while (it != m_pythonSounds.end())
    {
      if (it->second)
        it->second->SetVolume(level);

      ++it;
    }
  }
}
