#pragma once
#include "..\iplayer.h"
#include "..\..\utils\thread.h"

#include "IDVDPlayer.h"

#include "DVDDemuxers\DVDPacketQueue.h"
#include "DVDDemuxSPU.h"
#include "DVDClock.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayerVideo.h"
#include "DVDPlayerSubtitle.h"

#include "DVDPlayerMessenger.h"
//#include "DVDChapterReader.h"
#include "DVDSubtitles\DVDFactorySubtitle.h"

class CDVDInputStream;

class CDVDDemux;
class CDemuxStreamVideo;
class CDemuxStreamAudio;

#define DVDSTATE_NORMAL 0x00000001 // normal dvd state
#define DVDSTATE_STILL  0x00000002 // currently displaying a still frame

#define DVDPACKET_MESSAGE_RESYNC 0x00000001 // will be set in a packet to signal that we have an discontinuity
#define DVDPACKET_MESSAGE_STILL  0x00000002
#define DVDPACKET_MESSAGE_NOSKIP 0x00000004
#define DVDPACKET_MESSAGE_FLUSH  0x00000008

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
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdPlayerAudio.RegisterAudioCallback(pCallback); }
  virtual void UnRegisterAudioCallback()                        { m_dvdPlayerAudio.UnRegisterAudioCallback(); }
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
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual void SetVolume(long nVolume)                          { m_dvdPlayerAudio.SetVolume(nVolume); }
  virtual int GetVolume()                                       { return m_dvdPlayerAudio.GetVolume(); }
  virtual void SetContrast(bool bPlus) {}
  virtual void SetBrightness(bool bPlus) {}
  virtual void SetHue(bool bPlus) {}
  virtual void SetSaturation(bool bPlus) {}
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing)                       { m_dvdPlayerVideo.Update(bPauseDrawing); }
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect)      { m_dvdPlayerVideo.GetVideoRect(SrcRect, DestRect); }
  virtual void GetVideoAspectRatio(float& fAR)                  { fAR = m_dvdPlayerVideo.GetAspectRatio(); }
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

  virtual void SeekTime(__int64 iTime);
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual void ShowOSD(bool bOnoff);
  virtual void DoAudioWork()                                    { m_dvdPlayerAudio.DoWork(); }
  virtual bool OnAction(const CAction &action);
  virtual bool HasMenu();
  
  virtual bool GetCurrentSubtitle(CStdStringW& strSubtitle);
  
  // virtual IChapterProvider* GetChapterProvider();

  virtual int OnDVDNavResult(void* pData, int iMessage);
  
private:
  void LockStreams()   { EnterCriticalSection(&m_critStreamSection); }
  void UnlockStreams() { LeaveCriticalSection(&m_critStreamSection); }
  
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool OpenDefaultAudioStream();
  bool OpenAudioStream(int iStream);
  bool OpenVideoStream(int iStream);
  bool CloseAudioStream(bool bWaitForBuffers);
  bool CloseVideoStream(bool bWaitForBuffers);
  
  void ProcessAudioData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket);
  void ProcessSubData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket);
  
  __int64 GetTotalTimeInMsec();
  void FlushBuffers();

  void HandleMessages();
  bool IsInMenu() const;
  void UpdateOverlayInfo(int iAction);

  bool m_bRenderSubtitle;
  bool m_bDontSkipNextFrame;
  bool m_bReadAgain; // tricky, if set to true, the main loop will start over again
  bool m_bAbortRequest;

  char m_filename[1024];
    
  int m_iCurrentStreamVideo;
  int m_iCurrentStreamAudio;
  int m_iCurrentStreamSubtitle;

  int m_iSpeed; // 1 is normal speed, 0 is paused
  
  // classes
  CDVDPlayerAudio m_dvdPlayerAudio; // audio part
  CDVDPlayerVideo m_dvdPlayerVideo; // video part
  CDVDPlayerSubtitle m_dvdPlayerSubtitle; // subtitle part
  
  CDVDPlayerMessenger m_messenger;  // thread messenger
  CDVDClock m_clock;                // master clock
  CDVDDemuxSPU m_dvdspus;           // dvd subtitle demuxer
  CDVDOverlayContainer m_overlayContainer;
  // CDVDChapterReader m_chapterReader;// dvd chapter provider
  
  CDVDInputStream* m_pInputStream;  // input stream for current playing file
  CDVDDemux* m_pDemuxer;            // demuxer for current playing file
  
  DVDInfo m_dvd;
  
  HANDLE m_hReadyEvent;
  CRITICAL_SECTION m_critStreamSection; // need to have this lock when switching streams (audio / video)
};
