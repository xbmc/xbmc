#pragma once
#include "IPlayer.h"
#include "../utils/Thread.h"

class CDummyVideoPlayer : public IPlayer, public CThread
{
public:
  CDummyVideoPlayer(IPlayerCallback& callback);
  virtual ~CDummyVideoPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {}
  virtual void UnRegisterAudioCallback()                        {}
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo();
  virtual bool HasAudio();
  virtual void ToggleOSD() { }; // empty
  virtual void SwitchToNextLanguage();
  virtual void ToggleSubtitles();
  virtual void ToggleFrameDrop();
  virtual bool CanSeek();
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual void SetVolume(long nVolume) {}
  virtual void SetDynamicRangeCompression(long drc) {}
  virtual void SetContrast(bool bPlus) {}
  virtual void SetBrightness(bool bPlus) {}
  virtual void SetHue(bool bPlus) {}
  virtual void SetSaturation(bool bPlus) {}
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing)                       {}
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect)      {}
  virtual void GetVideoAspectRatio(float& fAR)                  {}
  virtual void SwitchToNextAudioLanguage();
  virtual bool CanRecord() { return false; }
  virtual bool IsRecording() { return false; }
  virtual bool Record(bool bOnOff) { return false; }
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();
  void Render();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();

  virtual void SeekTime(__int64 iTime);
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual void ShowOSD(bool bOnoff);
  virtual void DoAudioWork()                                    {}
  
  virtual CStdString GetPlayerState();
  virtual bool SetPlayerState(CStdString state);
  
private:
  virtual void Process();

  bool m_paused;
  __int64 m_clock;
  DWORD m_lastTime;
  int m_speed;
};
