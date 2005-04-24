#pragma once
#include "..\iplayer.h"
#include "..\..\utils\thread.h"

#include "IDVDPlayer.h"

#include "DVDVideo.h"
#include "DVDDemuxers\DVDPacketQueue.h"
#include "DVDDemuxSPU.h"
#include "DVDClock.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayerVideo.h"
#include "DVDPlayerMessenger.h"

class DllLoader;
class CDVDTimerThread;
class CDVDInputStream;
class CDVDVideoCodec;

class CDVDDemux;
class CDemuxStreamVideo;
class CDemuxStreamAudio;

#define DVDSTATE_NORMAL 0x00000001 // normal dvd state
#define DVDSTATE_STILL  0x00000002 // currently displaying a still frame
#define DVDSTATE_RESYNC 0x00000004 // will be set in a packet to signal that we have an discontinuity

typedef struct DVDInfo
{
  int iCurrentCell;
  int state; // current dvdstate
  int iDVDStillTime; // total time in seconds we should display the still before continuing
  __int64 iDVDStillStartTime; // time in ticks when we started the still
  bool bDisplayedStill;
  int iSelectedSPUStream; // mpeg stream id, or -1 if disabled
  int iSelectedAudioStream; // mpeg stream id, or -1 if disabled
  // DVDVideoPicture* pStillPicture;
  unsigned __int64 iNAVPackStart;
  unsigned __int64 iNAVPackFinish;
  int iFlagSentStart;
}
DVDInfo;

class CDVDPlayer : public IPlayer, public CThread, public IDVDPlayer
{
public:
  CDVDPlayer(IPlayerCallback& callback);
  virtual ~CDVDPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();
  virtual bool OpenFile(const CFileItem& file, __int64 iStartTime);
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
  virtual void SubtitleOffset(bool bPlus = true);
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual void SeekPercentage(int iPercent);
  virtual int GetPercentage();
  virtual void SetVolume(long nVolume);
  virtual int GetVolume();
  virtual void SetContrast(bool bPlus);
  virtual void SetBrightness(bool bPlus);
  virtual void SetHue(bool bPlus);
  virtual void SetSaturation(bool bPlus);
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing);
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect);
  virtual void GetVideoAspectRatio(float& fAR);
  virtual void AudioOffset(bool bPlus);
  virtual void SwitchToNextAudioLanguage();
  virtual void UpdateSubtitlePosition();
  virtual void RenderSubtitles();
  virtual bool CanRecord() { return false; }
  virtual bool IsRecording() { return false; }
  virtual bool Record(bool bOnOff) { return false; }
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();
  virtual int GetSubtitleCount();
  virtual int GetSubtitle();
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void SetSubtitle(int iStream);
  virtual bool GetSubtitleVisible();
  virtual void SetSubtitleVisible(bool bVisible);
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension);

  virtual int GetAudioStreamCount();
  virtual int GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void SetAudioStream(int iStream);

  virtual void SeekTime(int iTime);
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual void ShowOSD(bool bOnoff);

  virtual void DoAudioWork();
  virtual bool OnAction(const CAction &action);

  virtual bool IsInMenu() const;

  int OutputPicture(DVDVideoPicture* pPicture, double pts1);

  bool OpenDefaultAudioStream();
  bool OpenAudioStream(int iStream);
  bool OpenVideoStream(int iStream);
  bool CloseAudioStream(bool bWaitForBuffers);
  bool CloseVideoStream(bool bWaitForBuffers);

  void UnlockStreams();
  void LockStreams();

  virtual int OnDVDNavResult(void* pData, int iMessage);

  CDVDDemuxSPU m_dvdspus; // dvd subtitle demuxer
  DVDInfo m_dvd;

  CDVDInputStream* m_pInputStream;
  int m_iSpeed; // 1 is normal speed, 0 is paused
  int m_bAbortRequest;

  CDVDDemux* m_pDemuxer;

  bool m_bFirstHandleMessages; // tricky, if set to true, the main loop will start over again
  bool m_bDrawedFrame;
  CDemuxStreamVideo* m_pCurrentDemuxStreamVideo;
  CDemuxStreamAudio* m_pCurrentDemuxStreamAudio;
  int m_iCurrentVideoStream;
  int m_iCurrentAudioStream;
  int m_iCurrentPhysicalAudioStream; //The x:th audio stream will be opened by default

  CDVDPlayerAudio m_dvdPlayerAudio;
  CDVDPlayerVideo m_dvdPlayerVideo;
  CDVDPlayerMessenger m_messenger;
  CDVDClock m_clock;

protected:
  void Unload();
  bool Load();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  void FlushBuffers();

  void HandleMessages();

  DllLoader* m_pDLLavformat;
  DllLoader* m_pDLLavcodec;

  HANDLE m_hReadyEvent;

  char m_filename[1024];

  bool m_bReadData;
  bool m_bRenderSubtitle;
  CRITICAL_SECTION m_critStreamSection; // need to have this lock when switching streams (audio / video)
};
