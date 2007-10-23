#include "include.h"
#include "GUIAudioManager.h"
#include "Key.h"
#include "AudioContext.h"
#include "GUISound.h"
#include "../xbmc/Settings.h"
#include "../xbmc/ButtonTranslator.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/Util.h"
#ifdef HAS_SDL
#include <SDL/SDL_mixer.h>
#endif


CGUIAudioManager g_audioManager;

CGUIAudioManager::CGUIAudioManager()
{
  m_actionSound=NULL;
  m_bEnabled=true;
  g_audioContext.SetSoundDeviceCallback(this);    
}

CGUIAudioManager::~CGUIAudioManager()
{

}

void CGUIAudioManager::Initialize(int iDevice)
{
  CSingleLock lock(m_cs);

  if (iDevice==CAudioContext::DEFAULT_DEVICE)
  {
#ifndef HAS_SDL_AUDIO
    bool bAudioOnAllSpeakers=false;
    g_audioContext.SetupSpeakerConfig(2, bAudioOnAllSpeakers);
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
#else
    Mix_CloseAudio();
    if (Mix_OpenAudio(44100, AUDIO_S16, 2, 4096))
       CLog::Log(LOGERROR, "Unable to open audio mixer");
#endif
  }
}

void CGUIAudioManager::DeInitialize(int iDevice)
{
  CSingleLock lock(m_cs);

  if (!(iDevice == CAudioContext::DIRECTSOUND_DEVICE || iDevice == CAudioContext::DEFAULT_DEVICE)) return;

  if (m_actionSound)
  {
    //  Wait for finish when an action sound is playing
    while(m_actionSound->IsPlaying());

    delete m_actionSound;
    m_actionSound=NULL;
  }

  windowSoundsMap::iterator it=m_windowSounds.begin();
  while (it!=m_windowSounds.end())
  {
    CGUISound* sound=it->second;
    if (sound->IsPlaying())
      sound->Stop();

    delete sound;
    m_windowSounds.erase(it++);
  }
  m_windowSounds.clear();

  pythonSoundsMap::iterator it1=m_pythonSounds.begin();
  while (it1!=m_pythonSounds.end())
  {
    CGUISound* sound=it1->second;
    if (sound->IsPlaying())
      sound->Stop();

    delete sound;
    m_pythonSounds.erase(it1++);
  }
  m_pythonSounds.clear();
}

// \brief Clear any unused audio buffers
void CGUIAudioManager::FreeUnused()
{
  CSingleLock lock(m_cs);

  //  Free the sound from the last action
  if (m_actionSound && !m_actionSound->IsPlaying())
  {
    delete m_actionSound;
    m_actionSound=NULL;
  }

  //  Free sounds from windows
  windowSoundsMap::iterator it=m_windowSounds.begin();
  while (it!=m_windowSounds.end())
  {
    CGUISound* sound=it->second;
    if (!sound->IsPlaying())
    {
      delete sound;
      m_windowSounds.erase(it++);
    }
    else ++it;
  }

  // Free sounds from python
  pythonSoundsMap::iterator it1=m_pythonSounds.begin();
  while (it1!=m_pythonSounds.end())
  {
    CGUISound* sound=it1->second;
    if (!sound->IsPlaying())
    {
      delete sound;
      m_pythonSounds.erase(it1++);
    }
    else ++it1;
  }
}

// \brief Play a sound associated with a CAction
void CGUIAudioManager::PlayActionSound(const CAction& action)
{
  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled || g_audioContext.IsPassthroughActive())
    return;

  CSingleLock lock(m_cs);

  actionSoundMap::iterator it=m_actionSoundMap.find(action.wID);
  if (it==m_actionSoundMap.end()) 
    return;
  
  if (m_actionSound)
  {
    delete m_actionSound;
    m_actionSound=NULL;
  }

  CStdString strFile=_P(m_strMediaDir+"\\"+it->second);
  m_actionSound=new CGUISound();
  if (!m_actionSound->Load(strFile))
  {
    delete m_actionSound;
    m_actionSound=NULL;
    return;
  }

  m_actionSound->Play();
}

// \brief Play a sound associated with a window and its event
// Events: SOUND_INIT, SOUND_DEINIT
void CGUIAudioManager::PlayWindowSound(DWORD dwID, WINDOW_SOUND event)
{
  // it's not possible to play gui sounds when passthrough is active
  if (!m_bEnabled || g_audioContext.IsPassthroughActive())
    return;

  CSingleLock lock(m_cs);

  windowSoundMap::iterator it=m_windowSoundMap.find((WORD)(dwID & 0xffff));
  if (it==m_windowSoundMap.end())
    return;

  CWindowSounds sounds=it->second;
  CStdString strFile;
  switch (event)
  {
  case SOUND_INIT:
    strFile=sounds.strInitFile;
    break;
  case SOUND_DEINIT:
    strFile=sounds.strDeInitFile;
    break;
  }

  if (strFile.IsEmpty())
    return;

  //  One sound buffer for each window
  windowSoundsMap::iterator itsb=m_windowSounds.find(dwID);
  if (itsb!=m_windowSounds.end())
  {
    CGUISound* sound=itsb->second;
    if (sound->IsPlaying())
      sound->Stop();
    delete sound;
    m_windowSounds.erase(itsb++);
  }

  CGUISound* sound=new CGUISound();
  if (!sound->Load(_P(m_strMediaDir+"\\"+strFile)))
  {
    delete sound;
    return;
  }

  m_windowSounds.insert(pair<DWORD, CGUISound*>(dwID, sound));
  sound->Play();
}

// \brief Play a sound given by filename
void CGUIAudioManager::PlayPythonSound(const CStdString& strFileName)
{
  // it's not possible to play gui sounds when passthrough is active
  if (g_audioContext.IsPassthroughActive())
    return;

  CSingleLock lock(m_cs);

  // If we already loaded the sound, just play it
  pythonSoundsMap::iterator itsb=m_pythonSounds.find(strFileName);
  if (itsb!=m_pythonSounds.end())
  {
    CGUISound* sound=itsb->second;
    if (sound->IsPlaying())
      sound->Stop();

    sound->Play();

    return;
  }

  CGUISound* sound=new CGUISound();
  if (!sound->Load(_P(strFileName)))
  {
    delete sound;
    return;
  }

  m_pythonSounds.insert(pair<CStdString, CGUISound*>(strFileName, sound));
  sound->Play();
}

// \brief Load the config file (sounds.xml) for nav sounds
// Can be located in a folder "sounds" in the skin or from a
// subfolder of the folder "sounds" in the root directory of
// xbmc
bool CGUIAudioManager::Load()
{  
  m_actionSoundMap.clear();
  m_windowSoundMap.clear();

  if (g_guiSettings.GetString("lookandfeel.soundskin")=="OFF")
    return true;

  if (g_guiSettings.GetString("lookandfeel.soundskin")=="SKINDEFAULT")
    m_strMediaDir=_P("Q:\\skin\\"+g_guiSettings.GetString("lookandfeel.skin")+"\\sounds");
  else
    m_strMediaDir=_P("Q:\\sounds\\"+g_guiSettings.GetString("lookandfeel.soundskin"));
    
  CStdString strSoundsXml=_P(m_strMediaDir+"\\sounds.xml");

  //  Load our xml file
  TiXmlDocument xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", strSoundsXml.c_str());
  
  //  Load the config file
  if (!xmlDoc.LoadFile(strSoundsXml))
  {
    CLog::Log(LOGNOTICE, "%s, Line %d\n%s", strSoundsXml.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  CStdString strValue = pRoot->Value();
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
      WORD wID = 0;    // action identity
      if (pIdNode && pIdNode->FirstChild())
      {
        g_buttonTranslator.TranslateActionString(pIdNode->FirstChild()->Value(), wID);
      }

      TiXmlNode* pFileNode = pAction->FirstChild("file");
      CStdString strFile;
      if (pFileNode && pFileNode->FirstChild())
        strFile+=pFileNode->FirstChild()->Value();

      if (wID > 0 && !strFile.IsEmpty())
        m_actionSoundMap.insert(pair<WORD, CStdString>(wID, strFile));

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
      WORD wID = 0;

      TiXmlNode* pIdNode = pWindow->FirstChild("name");
      if (pIdNode)
      {
        if (pIdNode->FirstChild())
          wID = g_buttonTranslator.TranslateWindowString(pIdNode->FirstChild()->Value());
      }

      CWindowSounds sounds;
      LoadWindowSound(pWindow, "activate", sounds.strInitFile);
      LoadWindowSound(pWindow, "deactivate", sounds.strDeInitFile);

      if (wID > 0)
        m_windowSoundMap.insert(pair<WORD, CWindowSounds>(wID, sounds));

      pWindow = pWindow->NextSibling();
    }
  }

  return true;
}

// \brief Load a window node of the config file (sounds.xml)
bool CGUIAudioManager::LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile)
{
  if (!pWindowNode)
    return false;

  TiXmlNode* pFileNode = pWindowNode->FirstChild(strIdentifier);
  if (pFileNode && pFileNode->FirstChild())
  {
    strFile = pFileNode->FirstChild()->Value();
    return true;
  }

  return false;
}

// \brief Enable/Disable nav sounds
void CGUIAudioManager::Enable(bool bEnable)
{
  // Enable/Disable has no effect if nav sounds are turned off
  if (g_guiSettings.GetString("lookandfeel.soundskin")=="OFF")
    return;

  m_bEnabled=bEnable;
}

// \brief Sets the volume of all playing sounds
void CGUIAudioManager::SetVolume(int iLevel)
{
  CSingleLock lock(m_cs);

  if (m_actionSound)
    m_actionSound->SetVolume(iLevel);

  windowSoundsMap::iterator it=m_windowSounds.begin();
  while (it!=m_windowSounds.end())
  {
    if (it->second)
      it->second->SetVolume(iLevel);

    ++it;
  }

  pythonSoundsMap::iterator it1=m_pythonSounds.begin();
  while (it1!=m_pythonSounds.end())
  {
    if (it1->second)
      it1->second->SetVolume(iLevel);

    ++it1;
  }
}
