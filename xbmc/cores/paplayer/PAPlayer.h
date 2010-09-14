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

#include <list>

#include "cores/IPlayer.h"
#include "utils/Thread.h"
#include "AudioDecoder.h"
#include "utils/CriticalSection.h"

#include "AudioEngine/AEFactory.h"
#include "AudioEngine/AEStream.h"
#include "AudioEngine/PostProc/AEPPAnimationFade.h"

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
  virtual bool CloseFile();
  virtual bool IsPlaying() const { return m_current || !m_streams.empty(); }
  virtual void Pause();
  virtual bool IsPaused() const { return m_isPaused; }
  virtual bool HasVideo() const { return false; }
  virtual bool HasAudio() const { return true; }
  virtual bool CanSeek();
  virtual void Seek(bool bPlus = true, bool bLargeStep = false);
  virtual void SeekPercentage(float fPercent = 0.0f);
  virtual float GetPercentage();
  virtual void SetVolume(float volume);
  virtual void SetDynamicRangeCompression(long drc);
  virtual void GetAudioInfo( CStdString& strAudioInfo) {}
  virtual void GetVideoInfo( CStdString& strVideoInfo) {}
  virtual void GetGeneralInfo( CStdString& strVideoInfo) {}
  virtual void Update(bool bPauseDrawing = false) {}
  virtual void ToFFRW(int iSpeed = 0);
  virtual int GetCacheLevel() const;
  virtual int GetTotalTime();
  virtual int GetAudioBitrate();
  virtual int GetChannels();
  virtual int GetBitsPerSample();
  virtual int GetSampleRate();
  virtual CStdString GetAudioCodecName();
  virtual __int64 GetTime();
  virtual void ResetTime();
  virtual void SeekTime(__int64 iTime = 0);
  virtual bool SkipNext();

  static bool HandlesType(const CStdString &type);
protected:
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit();

private:
  IAudioCallback    *m_audioCallback;
  CCriticalSection   m_critSection;

  typedef struct
  {
    CAudioDecoder       m_decoder;        /* the decoder instance */
    PAPlayer           *m_player;         /* the PAPlayer instance */
    IAEStream          *m_stream;         /* the audio stream */
    unsigned int        m_sent;           /* frames sent */
    unsigned int        m_change;         /* frame to start xfade at */
    unsigned int        m_prepare;        /* frame to prepare next file at */
    bool                m_triggered;      /* if the queue callback has been called */
    unsigned int        m_bytesPerSample; /* bytes per audio sample */
    unsigned int        m_snippetEnd;     /* frame to perform the next FF/RW */
  } StreamInfo;

  std::list<StreamInfo*>  m_streams;    /* queued streams */
  std::list<StreamInfo*>  m_finishing;  /* finishing streams */
  StreamInfo             *m_current;    /* the current playing stream */
  bool                    m_isPaused;
  int                     m_iSpeed;
  bool                    m_fastOpen;
  bool                    m_queueFailed;
  bool                    m_playOnQueue;

  void FreeStreamInfo(StreamInfo *si);
  bool PlayNextStream();

  static void StaticStreamOnData (IAEStream *sender, void *arg, unsigned int needed);
  static void StaticStreamOnDrain(IAEStream *sender, void *arg, unsigned int unused);
  static void StaticFadeOnDone   (CAEPPAnimationFade *sender, void *arg);
};

