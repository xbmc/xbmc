#pragma once
#include "IAudioDeviceChangedCallback.h"
#include "../xbmc/utils/CriticalSection.h"

// forward definitions
struct CAction;
class CGUISound;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager : public IAudioDeviceChangedCallback
{
  class CWindowSounds
  {
  public:
    CStdString strInitFile;
    CStdString strDeInitFile;
  };

public:
  CGUIAudioManager();
  virtual ~CGUIAudioManager();

  virtual void        Initialize(int iDevice);
  virtual void        DeInitialize(int iDevice);

          bool        Load();

          void        PlayActionSound(const CAction& action);
          void        PlayWindowSound(DWORD dwID, WINDOW_SOUND event);
          void        PlayPythonSound(const CStdString& strFileName);

          void        FreeUnused();

          void        Enable(bool bEnable);
          void        SetVolume(int iLevel);
private:
          bool        LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile);

  typedef std::map<WORD, CStdString> actionSoundMap;
  typedef std::map<WORD, CWindowSounds> windowSoundMap;

  typedef std::map<CStdString, CGUISound*> pythonSoundsMap;
  typedef std::map<DWORD, CGUISound*> windowSoundsMap;

  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;

  CGUISound*          m_actionSound;
  windowSoundsMap     m_windowSounds;
  pythonSoundsMap     m_pythonSounds;

  CStdString          m_strMediaDir;
  bool                m_bEnabled;

  CCriticalSection    m_cs;
};

extern CGUIAudioManager g_audioManager;
