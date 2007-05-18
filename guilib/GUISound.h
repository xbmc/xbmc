#pragma once

#ifdef HAS_SDL
#ifdef _LINUX
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif
#endif

class CGUISound
{
public:
  CGUISound();
  virtual ~CGUISound();

  bool        Load(const CStdString& strFile);
  void        Play();
  void        Stop();
  bool        IsPlaying();
  void        SetVolume(int level);

private:
#ifndef HAS_SDL
  bool        LoadWav(const CStdString& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize);
  bool        CreateBuffer(LPWAVEFORMATEX wfx, int iLength);
  bool        FillBuffer(LPBYTE pbData, int iLength);
  void        FreeBuffer();

  LPDIRECTSOUNDBUFFER m_soundBuffer;
#else
  Mix_Chunk* m_soundBuffer;  
#endif
};
