/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#if !defined(__arm__)
#ifndef __COREAUDIO_RENDERER_H__
#define __COREAUDIO_RENDERER_H__

#include <osx/CoreAudio.h>
#include "IAudioRenderer.h"
#include <threads/LockFree.h>

struct audio_slice
{
  struct _tag_header{
    uint64_t timestamp; // Currently not used
    size_t data_len;
  } header;
  unsigned int data[1];
  unsigned char* get_data() {return (unsigned char*)&data;}
};

class CAtomicAllocator
{
public:
  CAtomicAllocator(size_t blockSize);
  ~CAtomicAllocator();
  void* Alloc();
  void Free(void* p);
  size_t GetBlockSize();
private:
  lf_heap m_Heap ;
  size_t m_BlockSize;
};

class CSliceQueue
{
public:
  CSliceQueue(size_t sliceSize);
  virtual ~CSliceQueue();
  size_t AddData(void* pBuf, size_t bufLen);
  size_t GetData(void* pBuf, size_t bufLen);
  size_t GetTotalBytes();
  void Clear();
protected:
  void Push(audio_slice* pSlice);
  audio_slice* Pop(); // Does not respect remainder, so it must be private
  CAtomicAllocator* m_pAllocator;
  lf_queue m_Queue;
  size_t m_TotalBytes;
  audio_slice* m_pPartialSlice;
  size_t m_RemainderSize;
};

class CCoreAudioPerformance
{
public:
  CCoreAudioPerformance();
  ~CCoreAudioPerformance();
  void Init(UInt32 expectedBytesPerSec, UInt32 watchdogInterval = 1000, UInt32 flags = 0);
  void ReportData(UInt32 bytesIn, UInt32 bytesOut);
  void EnableWatchdog(bool enable);
  void SetPreroll(UInt32 bytes); // Fixed bytes
  void SetPreroll(float seconds); // Calculated for time (seconds)
  void Reset();
  enum
  {
    FlagDefault = 0
  };
protected:
  UInt64 m_TotalBytesIn;
  UInt64 m_TotalBytesOut;
  UInt32 m_ExpectedBytesPerSec;
  UInt32 m_ActualBytesPerSec;
  UInt32 m_Flags;
  bool m_WatchdogEnable;
  UInt32 m_WatchdogInterval;
  UInt32 m_LastWatchdogCheck;
  UInt32 m_LastWatchdogBytesIn;
  UInt32 m_LastWatchdogBytesOut;
  float m_WatchdogBitrateSensitivity;
  UInt32 m_WatchdogPreroll;
};

class CCoreAudioRenderer : public IAudioRenderer
  {
  public:
    CCoreAudioRenderer();
    virtual ~CCoreAudioRenderer();
    virtual unsigned int GetChunkLen();
    virtual float GetDelay();
    virtual bool Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic=false, bool bPassthrough = false);
    virtual bool Deinitialize();
    virtual unsigned int AddPackets(const void* data, unsigned int len);
    virtual unsigned int GetSpace();
    virtual float GetCacheTime();
    virtual float GetCacheTotal();
    virtual bool Pause();
    virtual bool Stop();
    virtual bool Resume();

    virtual long GetCurrentVolume() const;
    virtual void Mute(bool bMute);
    virtual bool SetCurrentVolume(long nVolume);
    virtual void WaitCompletion();

    // Unimplemented IAudioRenderer methods
    virtual int SetPlaySpeed(int iSpeed) {return 0;};
    virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers) {};
    virtual void UnRegisterAudioCallback() {};
    virtual void RegisterAudioCallback(IAudioCallback* pCallback) {};

    static void EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough) {};
  private:
    OSStatus OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
    static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
    static OSStatus DirectRenderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData);
    bool InitializeEncoded(AudioDeviceID outputDevice, UInt32 sampleRate);
    bool InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample, enum PCMChannels *channelMap);
    bool InitializePCMEncoded(UInt32 sampleRate);

    bool m_Pause;
    bool m_Initialized; // Prevent multiple init/deinit

    long m_CurrentVolume; // Courtesy of the jerk that made GetCurrentVolume a const...
    unsigned int m_ChunkLen; // Minimum amount of data accepted by AddPackets
    char* m_RemapBuffer; // Temporary buffer for channel remapping
    CSliceQueue* m_pCache;
    size_t m_MaxCacheLen; // Maximum number of bytes to be cached by the renderer.

    CAUOutputDevice m_AUOutput;
    CCoreAudioUnit m_MixerUnit;
    CCoreAudioDevice m_AudioDevice;
    CCoreAudioStream m_OutputStream;
    UInt32 m_OutputBufferIndex;

    bool m_Passthrough;
    bool m_EnableVolumeControl;

    // Stream format
    size_t m_AvgBytesPerSec;
    size_t m_BytesPerFrame; // Input frame size
    UInt32 m_NumLatencyFrames;

#ifdef _DEBUG
    // Performace Monitoring
    CCoreAudioPerformance m_PerfMon;
#endif
    // Thread synchronization
    MPEventID m_RunoutEvent;
    long m_DoRunout;
  };

#endif
#endif
