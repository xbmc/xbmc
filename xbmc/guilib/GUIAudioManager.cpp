/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAudioManager.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "filesystem/Directory.h"
#include "input/WindowTranslator.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;

CGUIAudioManager::CGUIAudioManager()
  : m_settings(CServiceBroker::GetSettingsComponent()->GetSettings())
{
  m_bEnabled = false;

  m_settings->RegisterCallback(this, {CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN,
                                      CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME});
}

CGUIAudioManager::~CGUIAudioManager()
{
  m_settings->UnregisterCallback(this);
}

void CGUIAudioManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
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

bool CGUIAudioManager::OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                                       const char* oldSettingId,
                                       const TiXmlNode* oldSettingNode)
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
  if (setting->GetId() == CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME)
  {
    int vol = m_settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME);
    SetVolume(0.01f * vol);
  }
  return true;
}


void CGUIAudioManager::Initialize()
{
}

void CGUIAudioManager::DeInitialize()
{
  std::unique_lock<CCriticalSection> lock(m_cs);
  UnLoad();
}

void CGUIAudioManager::Stop()
{
  std::unique_lock<CCriticalSection> lock(m_cs);
  for (const auto& windowSound : m_windowSoundMap)
  {
    if (windowSound.second.initSound)
      windowSound.second.initSound->Stop();
    if (windowSound.second.deInitSound)
      windowSound.second.deInitSound->Stop();
  }

  for (const auto& pythonSound : m_pythonSounds)
  {
    pythonSound.second->Stop();
  }
}

// \brief Play a sound associated with a CAction
void CGUIAudioManager::PlayActionSound(const CAction& action)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled)
    return;

  const auto it = m_actionSoundMap.find(action.GetID());
  if (it == m_actionSoundMap.end())
    return;

  if (it->second)
  {
    int vol = m_settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME);
    it->second->SetVolume(0.01f * vol);
    it->second->Play();
  }
}

// \brief Play a sound associated with a window and its event
// Events: SOUND_INIT, SOUND_DEINIT
void CGUIAudioManager::PlayWindowSound(int id, WINDOW_SOUND event)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled)
    return;

  const auto it = m_windowSoundMap.find(id);
  if (it==m_windowSoundMap.end())
    return;

  std::shared_ptr<IAESound> sound;
  switch (event)
  {
    case SOUND_INIT:
      sound = it->second.initSound;
      break;
    case SOUND_DEINIT:
      sound = it->second.deInitSound;
      break;
  }

  if (!sound)
    return;

  int vol = m_settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME);
  sound->SetVolume(0.01f * vol);
  sound->Play();
}

// \brief Play a sound given by filename
void CGUIAudioManager::PlayPythonSound(const std::string& strFileName, bool useCached /*= true*/)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled)
    return;

  // If we already loaded the sound, just play it
  const auto itsb = m_pythonSounds.find(strFileName);
  if (itsb != m_pythonSounds.end())
  {
    const auto& sound = itsb->second;
    if (useCached)
    {
      sound->Play();
      return;
    }
    else
    {
      m_pythonSounds.erase(itsb);
    }
  }

  auto sound = LoadSound(strFileName);
  if (!sound)
    return;

  int vol = m_settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME);
  sound->SetVolume(0.01f * vol);
  sound->Play();
  m_pythonSounds.emplace(strFileName, std::move(sound));
}

void CGUIAudioManager::UnLoad()
{
  m_windowSoundMap.clear();
  m_pythonSounds.clear();
  m_actionSoundMap.clear();
  m_soundCache.clear();
}


std::string GetSoundSkinPath()
{
  auto setting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN));
  auto value = setting->GetValue();
  if (value.empty())
    return "";

  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(value, addon, ADDON::AddonType::RESOURCE_UISOUNDS,
                                              ADDON::OnlyEnabled::CHOICE_YES))
  {
    CLog::Log(LOGINFO, "Unknown sounds addon '{}'. Setting default sounds.", value);
    setting->Reset();
  }
  return URIUtils::AddFileToFolder("resource://", setting->GetValue());
}


// \brief Load the config file (sounds.xml) for nav sounds
bool CGUIAudioManager::Load()
{
  std::unique_lock<CCriticalSection> lock(m_cs);
  UnLoad();

  m_strMediaDir = GetSoundSkinPath();
  if (m_strMediaDir.empty())
    return true;

  Enable(true);
  std::string strSoundsXml = URIUtils::AddFileToFolder(m_strMediaDir, "sounds.xml");

  //  Load our xml file
  CXBMCTinyXML xmlDoc;

  CLog::Log(LOGINFO, "Loading {}", strSoundsXml);

  //  Load the config file
  if (!xmlDoc.LoadFile(strSoundsXml))
  {
    CLog::Log(LOGINFO, "{}, Line {}\n{}", strSoundsXml, xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  std::string strValue = pRoot->Value();
  if ( strValue != "sounds")
  {
    CLog::Log(LOGINFO, "{} Doesn't contain <sounds>", strSoundsXml);
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
        ACTION::CActionTranslator::TranslateString(pIdNode->FirstChild()->Value(), id);
      }

      TiXmlNode* pFileNode = pAction->FirstChild("file");
      std::string strFile;
      if (pFileNode && pFileNode->FirstChild())
        strFile += pFileNode->FirstChild()->Value();

      if (id != ACTION_NONE && !strFile.empty())
      {
        std::string filename = URIUtils::AddFileToFolder(m_strMediaDir, strFile);
        auto sound = LoadSound(filename);
        if (sound)
          m_actionSoundMap.emplace(id, std::move(sound));
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
          id = CWindowTranslator::TranslateWindow(pIdNode->FirstChild()->Value());
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

std::shared_ptr<IAESound> CGUIAudioManager::LoadSound(const std::string& filename)
{
  std::unique_lock<CCriticalSection> lock(m_cs);
  const auto it = m_soundCache.find(filename);
  if (it != m_soundCache.end())
  {
    auto sound = it->second.lock();
    if (sound)
      return sound;
    else
      m_soundCache.erase(it); // cleanup orphaned cache entry
  }

  IAE *ae = CServiceBroker::GetActiveAE();
  if (!ae)
    return nullptr;

  std::shared_ptr<IAESound> sound(ae->MakeSound(filename));
  if (!sound)
    return nullptr;

  m_soundCache[filename] = sound;

  return sound;
}

// \brief Load a window node of the config file (sounds.xml)
std::shared_ptr<IAESound> CGUIAudioManager::LoadWindowSound(TiXmlNode* pWindowNode,
                                                            const std::string& strIdentifier)
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
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN).empty())
    bEnable = false;

  std::unique_lock<CCriticalSection> lock(m_cs);
  m_bEnabled = bEnable;
}

// \brief Sets the volume of all playing sounds
void CGUIAudioManager::SetVolume(float level)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  {
    for (const auto& actionSound : m_actionSoundMap)
    {
      if (actionSound.second)
        actionSound.second->SetVolume(level);
    }
  }

  for (const auto& windowSound : m_windowSoundMap)
  {
    if (windowSound.second.initSound)
      windowSound.second.initSound->SetVolume(level);
    if (windowSound.second.deInitSound)
      windowSound.second.deInitSound->SetVolume(level);
  }

  {
    for (const auto& pythonSound : m_pythonSounds)
    {
      if (pythonSound.second)
        pythonSound.second->SetVolume(level);
    }
  }
}
