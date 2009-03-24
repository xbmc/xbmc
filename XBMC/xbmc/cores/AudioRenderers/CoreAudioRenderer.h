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

#ifndef __COREAUDIO_RENDERER_H__
#define __COREAUDIO_RENDERER_H__

#include "IAudioRenderer.h"
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

#include "SliceQueue.h"

struct core_audio_packet
{
  Uint32 id;
  BYTE* data;
};

class CCoreAudioRenderer : public IAudioRenderer
  {
  public:
    CCoreAudioRenderer();
    virtual ~CCoreAudioRenderer();
    virtual DWORD GetChunkLen();
    virtual FLOAT GetDelay();
    virtual bool Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec = "", bool bIsMusic=false, bool bPassthrough = false);
    virtual HRESULT Deinitialize();
    virtual DWORD AddPackets(unsigned char* data, DWORD len);
    virtual DWORD GetSpace();
    virtual HRESULT Pause();
    virtual HRESULT Stop();
    virtual HRESULT Resume();
    
    virtual LONG GetMinimumVolume() const;
    virtual LONG GetMaximumVolume() const;
    virtual LONG GetCurrentVolume() const;
    virtual void Mute(bool bMute);
    virtual HRESULT SetCurrentVolume(LONG nVolume);
    virtual void WaitCompletion();

    // Unimplemented IAudioRenderer methods
    virtual int SetPlaySpeed(int iSpeed) {return 0;};
    virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers) {};
    virtual void UnRegisterAudioCallback() {};
    virtual void RegisterAudioCallback(IAudioCallback* pCallback) {};
  private:
    static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
    OSStatus OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
    
    bool InitializeEncoded(UInt32 channels);
    bool InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample);
    bool m_Pause;
   
    DWORD m_ChunkLen;
    CSliceQueue m_Cache;
    size_t m_MaxCacheLen;
    
    AudioStreamBasicDescription m_InputDesc;
    AudioUnit m_OutputUnit;
    
    size_t m_AvgBytesPerSec;
    UInt64 m_TotalBytesIn;
    UInt64 m_TotalBytesOut;
    
    UInt32 m_CacheLock;
    
    // Helpers
    UInt32 GetAUPropUInt32(AudioUnit au, AudioUnitPropertyID propId, AudioUnitScope scope)
    {
      UInt32 val = 0, propSize = sizeof(val);
      OSStatus ret = AudioUnitGetProperty(au, propId, scope, 0, &val, &propSize);
      if (ret == noErr)
        return val;
      else
        return 0;
    }
  };

#endif 
