#pragma once
#include "../../Application.h"
#include "../iplayer.h"
#include "../mplayer/ASyncDirectSound.h"
#include "../../utils/thread.h"
#include "ICodec.h"

class PAPlayer : public IPlayer, public CThread
{
public:
  PAPlayer(IPlayerCallback& callback);
  virtual ~PAPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual bool OpenFile(const CFileItem& file, __int64 iStartTime);
  virtual bool CloseFile();
  virtual bool IsPlaying() const { return m_bIsPlaying; }
  virtual void Pause();
  virtual bool IsPaused() const { return m_bPaused; }
  virtual bool HasVideo() { return false; }
  virtual bool HasAudio() { return true; }
  virtual void ToggleOSD() {}
  virtual void SwitchToNextLanguage() {}
  virtual void ToggleSubtitles() {}
  virtual void ToggleFrameDrop() {}
  virtual void SubtitleOffset(bool bPlus = true) {}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false) {}
  virtual void SetVolume(long nVolume);
  virtual void SetContrast(bool bPlus = true) {}
  virtual void SetBrightness(bool bPlus = true) {}
  virtual void SetHue(bool bPlus = true) {}
  virtual void SetSaturation(bool bPlus = true) {}
  virtual void GetAudioInfo( CStdString& strAudioInfo) {}
  virtual void GetVideoInfo( CStdString& strVideoInfo) {}
  virtual void GetGeneralInfo( CStdString& strVideoInfo) {}
  virtual void Update(bool bPauseDrawing = false) {}
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect){}
  virtual void GetVideoAspectRatio(float& fAR) {}
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
  virtual void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength);
  virtual void ToFFRW(int iSpeed = 0);
  virtual int GetTotalTime();
  virtual __int64 GetTime();
  virtual void SeekTime(__int64 iTime = 0);

protected:
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit();

  bool m_bPaused;
  bool m_bIsPlaying;
  bool m_bStopPlaying;

  CEvent m_startEvent;

  int m_iSpeed;   // current playing speed

private:
  
  CASyncDirectSound* m_pAudioDevice;  // our output device
  DWORD m_dwAudioBufferSize; // size of the buffer in use
  DWORD m_dwAudioBufferMin;  // minimum size of our buffer before we need more
  DWORD m_dwAudioMaxSize;
  
  BYTE* m_pPcm;           // our temporary pcm buffer so we can learn the format of the data

  int CreateAudioDevice();     // initializes our audio device
  void KillAudioDevice();      // kills the above

  bool ProcessPAP();    // does the actual reading and decode from our PAP dll
  void ApplyReplayGain(void *pData, int size);

  bool    m_BufferingPcm;
  bool    m_eof;

  __int64 m_SeekTime;
  int     m_IsFFwdRewding;

  unsigned int     m_PcmSize;  
  unsigned int     m_PcmPos;
  unsigned int     m_BytesPerSecond;

  __int64 m_startOffset;
  __int64 m_dwBytesSentOut;

  ICodec*          m_codec;
};
