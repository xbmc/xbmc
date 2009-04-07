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
#include <osx/CoreAudio.h>

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
    
    static OSStatus PassthroughRenderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData);
    
    bool InitializeEncoded(AudioDeviceID outputDevice);
    bool InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample);
    
    bool m_Pause;
    bool m_Initialized; // Prevent multiple init/deinit
   
    LONG m_CurrentVolume; // Coutesy of the jerk that made GetCurrentVolume a const...
    DWORD m_ChunkLen; // Minimum amount of data accepted by AddPackets
    CSliceQueue m_Cache;
    size_t m_MaxCacheLen; // Maximum number of bytes cached by the renderer.
        
    CCoreAudioUnit m_AudioUnit;
    CCoreAudioDevice m_AudioDevice;
    CCoreAudioStream m_OutputStream;
    
    bool m_Passthrough;
    size_t m_AvgBytesPerSec;
    size_t m_BytesPerFrame;
    UInt64 m_TotalBytesIn;
    UInt64 m_TotalBytesOut;
    
    int m_Magic;
        
    // Helper Methods
    UInt32 GetAUPropUInt32(AudioUnit au, AudioUnitPropertyID propId, AudioUnitScope scope)
    {
      UInt32 val = 0, propSize = sizeof(val);
      OSStatus ret = AudioUnitGetProperty(au, propId, scope, 0, &val, &propSize);
      if (ret == noErr)
        return val;
      else
        return 0;
    }
    
    void StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str)
    {
      char fourCC[5];
      UInt32ToFourCC(fourCC, desc.mFormatID);
      
      switch (desc.mFormatID)
      {
        case kAudioFormatLinearPCM:
          str.Format("[%4.4s] %s%u Channel %u-bit %s (%uHz)", 
                     fourCC,
                     (desc.mFormatFlags & kAudioFormatFlagIsNonMixable) ? "" : "Mixable ",
                     desc.mChannelsPerFrame,
                     desc.mBitsPerChannel,
                     (desc.mFormatFlags & kAudioFormatFlagIsFloat) ? "Floating Point" : "Signed Integer",
                     (UInt32)desc.mSampleRate);
          break;
        case kAudioFormatAC3:
          str.Format("[%s] AC-3/DTS", fourCC);
          break;
        case kAudioFormat60958AC3:
          str.Format("[%s] AC-3/DTS for S/PDIF", fourCC);
          break;
        default:
          str.Format("[%s]", fourCC);
          break;
      }
    }
  };

#endif 
