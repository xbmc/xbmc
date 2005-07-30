#pragma once
#include "key.h"
#include "IAudioDeviceChangedCallback.h"

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

  virtual void        Initialize();
  virtual void        DeInitialize();

          bool        Load();

          void        PlayActionSound(const CAction& action);
          void        PlayWindowSound(DWORD dwID, WINDOW_SOUND event);
          void        PlayStartSound();

          void        FreeUnused();

          void        Enable(bool bEnable);
          void        SetVolume(int iLevel);
private:
          bool        CreateBufferFromFile(const CStdString& strFile, LPDIRECTSOUNDBUFFER* ppSoundBuffer);
          bool        CreateBuffer(LPWAVEFORMATEX wfx, int iLength, LPDIRECTSOUNDBUFFER* ppSoundBuffer);
          bool        FillBuffer(LPBYTE pbData, int iLength, LPDIRECTSOUNDBUFFER pSoundBuffer);
          void        FreeBuffer(LPDIRECTSOUNDBUFFER* pSoundBuffer);

          void        Play(LPDIRECTSOUNDBUFFER pSoundBuffer);
          void        StopPlaying(LPDIRECTSOUNDBUFFER pSoundBuffer);
          bool        IsPlaying(LPDIRECTSOUNDBUFFER pSoundBuffer);

          bool        LoadWav(const CStdString& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize);

          bool        LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile);

  typedef map<DWORD, LPDIRECTSOUNDBUFFER> soundBufferMap;
  typedef map<WORD, CStdString> actionSoundMap;
  typedef map<WORD, CWindowSounds> windowSoundMap;

  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;

  LPDIRECTSOUND8      m_lpDirectSound;

  LPDIRECTSOUNDBUFFER m_lpActionSoundBuffer;
  LPDIRECTSOUNDBUFFER m_lpStartSoundBuffer;
  soundBufferMap      m_windowSoundBuffers;

  CStdString          m_strMediaDir;
  bool                m_bEnabled;
};

extern CGUIAudioManager g_audioManager;
