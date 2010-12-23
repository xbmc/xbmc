#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "cores/IPlayer.h"
#include "utils/Thread.h"
#include "AudioDecoder.h"
#include "cores/ssrc.h"
#include "cores/AudioRenderers/IAudioRenderer.h"

class CFileItem;
#ifndef _LINUX
#define PACKET_COUNT  20 // number of packets of size PACKET_SIZE (defined in AudioDecoder.h)
#else
#define PACKET_COUNT  1
#endif

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

  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool QueueNextFile(const CFileItem &file);
  virtual void OnNothingToQueueNotify();
  virtual bool CloseFile()       { return CloseFileInternal(true); }
  virtual bool CloseFileInternal(bool bAudioDevice = true);
  virtual bool IsPlaying() const { return m_bIsPlaying; }
  virtual void Pause();
  virtual bool IsPaused() const { return m_bPaused; }
  virtual bool HasVideo() const { return false; }
  virtual bool HasAudio() const { return true; }
  virtual bool CanSeek();
  virtual void Seek(bool bPlus = true, bool bLargeStep = false);
  virtual void SeekPercentage(float fPercent = 0.0f);
  virtual float GetPercentage();
  virtual void SetVolume(long nVolume);
  virtual void SetDynamicRangeCompression(long drc);
  virtual void GetAudioInfo( CStdString& strAudioInfo) {}
  virtual void GetVideoInfo( CStdString& strVideoInfo) {}
  virtual void GetGeneralInfo( CStdString& strVideoInfo) {}
  virtual void Update(bool bPauseDrawing = false) {}
  virtual void ToFFRW(int iSpeed = 0);
  virtual int GetCacheLevel() const;
  virtual int GetTotalTime();
  __int64 GetTotalTime64();
  virtual int GetAudioBitrate();
  virtual int GetChannels();
  virtual int GetBitsPerSample();
  virtual int GetSampleRate();
  virtual CStdString GetAudioCodecName();
  virtual __int64 GetTime();
  virtual void ResetTime();
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

  void HandleSeeking();
  bool HandleFFwdRewd();

  bool m_bPaused;
  bool m_bIsPlaying;
  bool m_bQueueFailed;
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

#ifndef _LINUX
  void SetupDirectSound(int channels);
#endif

  // Our directsoundstream
  friend void CALLBACK StaticStreamCallback( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus );
  bool AddPacketsToStream(int stream, CAudioDecoder &dec);
  bool FindFreePacket(int stream, DWORD *pdwPacket );     // Looks for a free packet
  void FreeStream(int stream);
#if defined(_LINUX) || defined(_WIN32)
  void DrainStream(int stream);
#endif
  bool CreateStream(int stream, unsigned int channels, unsigned int samplerate, unsigned int bitspersample, CStdString codec = "");
  void FlushStreams();
  void WaitForStream();
  void SetStreamVolume(int stream, long nVolume);

  void UpdateCrossFadingTime(const CFileItem& file);
  bool QueueNextFile(const CFileItem &file, bool checkCrossFading);
  void UpdateCacheLevel();

  int m_currentStream;

  IAudioRenderer*   m_pAudioDecoder[2];
  float             m_latency[2];
  unsigned char*    m_pcmBuffer[2];
  int               m_bufferPos[2];
  unsigned int      m_Chunklen[2];

  unsigned int     m_SampleRate;
  unsigned int     m_Channels;
  unsigned int     m_BitsPerSample;

  unsigned int     m_SampleRateOutput;
  unsigned int     m_BitsPerSampleOutput;

  AudioPacket      m_packet[2][PACKET_COUNT];

  IAudioCallback*  m_pCallback;

  __int64          m_bytesSentOut;

  // format (this should be stored/retrieved from the audio device object probably)
  unsigned int     m_channelCount[2];
  enum PCMChannels*m_channelMap[2];
  unsigned int     m_sampleRate[2];
  unsigned int     m_bitsPerSample[2];
  unsigned int     m_BytesPerSecond;

  unsigned int     m_CacheLevel;
  unsigned int     m_LastCacheLevelCheck;

    // resampler
  Cssrc            m_resampler[2];
  bool             m_resampleAudio;

  // our file
  CFileItem*        m_currentFile;
  CFileItem*        m_nextFile;

  // stuff for visualisation
  unsigned int     m_visBufferLength;
  short            m_visBuffer[PACKET_SIZE+2];

};

