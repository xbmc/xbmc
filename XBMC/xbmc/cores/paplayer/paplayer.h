#pragma once
#include "../iplayer.h"
#include "../../utils/thread.h"
#include "AudioDecoder.h"
#include "../ssrc.h"

#define PACKET_COUNT  20 // number of packets of size PACKET_SIZE (defined in AudioDecoder.h)

#define STATUS_NO_FILE  0
#define STATUS_QUEUING  1
#define STATUS_QUEUED   2
#define STATUS_PLAYING  3
#define STATUS_ENDING   4
#define STATUS_ENDED    5

struct AudioPacket
{
  BYTE *packet;
  DWORD length;
  DWORD status;
  int   stream;
};

class PAPlayer : public IPlayer, public CThread
{
public:
  PAPlayer(IPlayerCallback& callback);
  virtual ~PAPlayer();

  virtual bool OpenFile(const CFileItem& file, __int64 iStartTime);
  virtual bool QueueNextFile(const CFileItem &file);
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
  virtual void SeekPercentage(float fPercent = 0.0f);
  virtual float GetPercentage();
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
  virtual void ToFFRW(int iSpeed = 0);
  virtual int GetTotalTime();
  __int64 GetTotalTime64();
  virtual int GetBitrate();
  virtual int GetChannels();
  virtual int GetBitsPerSample();
  virtual int GetSampleRate();
  virtual CStdString GetCodec();
  virtual __int64 GetTime();
  virtual void SeekTime(__int64 iTime = 0);
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext();

  void StreamCallback( LPVOID pPacketContext );

  virtual void RegisterAudioCallback(IAudioCallback *pCallback);
  virtual void UnRegisterAudioCallback();

  static bool HandlesType(const CStdString &type);
  virtual void DoAudioWork();
protected:
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit();

  bool ReadData(int amount);
  void HandleSeeking();
  bool HandleFFwdRewd();

  bool m_bPaused;
  bool m_bIsPlaying;
  bool m_bStopPlaying;
  bool m_cachingNextFile;
  int  m_crossFading;
  bool m_currentlyCrossFading;
  __int64 m_crossFadeLength;

  CEvent m_startEvent;

  int m_iSpeed;   // current playing speed

private:
  
  bool ProcessPAP();    // does the actual reading and decode from our PAP dll

  __int64 m_SeekTime;
  int     m_IsFFwdRewding;
  __int64 m_timeOffset;
  bool    m_forceFadeToNext;

  int m_currentDecoder;
  CAudioDecoder m_decoder[2]; // our 2 audiodecoders (for crossfading + precaching)

  void SetupDirectSound(int channels);

  // Our directsoundstream
  friend static void CALLBACK StaticStreamCallback( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus );
  bool AddPacketsToStream(int stream, CAudioDecoder &dec);
  bool FindFreePacket(int stream, DWORD *pdwPacket );     // Looks for a free packet
  void FreeStream(int stream);
  bool CreateStream(int stream, int channels, int samplerate, int bitspersample);
  void FlushStreams();
  void SetStreamVolume(int stream, long nVolume);
  
  void UpdateCrossFadingTime(const CFileItem& file);
  bool QueueNextFile(const CFileItem &file, bool checkCrossFading);

  int m_currentStream;
  IDirectSoundStream *m_pStream[2];
  AudioPacket         m_packet[2][PACKET_COUNT];

  __int64                 m_bytesSentOut;

  // format (this should be stored/retrieved from the audio device object probably)
  unsigned int     m_SampleRate;
  unsigned int     m_Channels;
  unsigned int     m_BitsPerSample;
  unsigned int     m_BytesPerSecond;

    // resampler
  Cssrc            m_resampler[2];
  bool             m_resampleAudio;

  // our file
  CFileItem        m_currentFile;
  CFileItem        m_nextFile;

  // stuff for visualisation
  BYTE             m_visBuffer[PACKET_SIZE];
  unsigned int     m_visBufferLength;
  IAudioCallback*  m_pCallback;
};
