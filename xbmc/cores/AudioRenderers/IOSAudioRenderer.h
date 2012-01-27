/*
 *      Copyright (C) 2010 Team XBMC
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

#ifndef __IOSAUDIO_RENDERER_H__
#define __IOSAUDIO_RENDERER_H__

#include "IOSCoreAudio.h"
#include "PlatformDefs.h"
#include "IAudioRenderer.h"
#include "threads/Event.h"

class IOSAudioRingBuffer;
class CIOSAudioRenderer : public IAudioRenderer
  {
  public:
    CIOSAudioRenderer();
    virtual ~CIOSAudioRenderer();
    virtual unsigned int GetChunkLen();
    virtual float GetDelay();
    virtual bool Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic=false, EEncoded encoded = IAudioRenderer::ENCODED_NONE);
    virtual bool Deinitialize();
    virtual void Flush();
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
    virtual void SetDynamicRangeCompression(long drc) { m_drc = drc; }
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
    static void PropertyChanged(AudioSessionPropertyID inID, UInt32 inDataSize, const void* inPropertyValue);
    static void PropertyChangeCallback(void* inClientData, AudioSessionPropertyID inID, UInt32 inDataSize, const void* inPropertyValue);
    bool InitializePCM(UInt32 channels, UInt32 samplesPerSecond, UInt32 bitsPerSample, enum PCMChannels *channelMap);

    bool m_Pause;
    bool m_Initialized; // Prevent multiple init/deinit

    long m_CurrentVolume; // Courtesy of the jerk that made GetCurrentVolume a const...
    bool m_EnableVolumeControl;

    CIOSCoreAudioDevice m_AudioDevice;
    UInt32 m_OutputBufferIndex;

    // Stream format
    int m_BitsPerChannel;
    int m_ChannelsPerFrame;

    IOSAudioRingBuffer *m_Buffer;
    unsigned int m_BytesPerSec;
    unsigned int m_BufferLen; ///< must always be num_chunks * chunk_size
    unsigned int m_NumChunks;
    unsigned int m_PacketSize;
    unsigned int m_BytesPerFrame;
    unsigned int m_BufferFrames;
    unsigned int m_SamplesPerSec;

    CEvent m_RunoutEvent;
    long m_DoRunout;
    unsigned int m_DataChannels;
    unsigned int m_Channels;
    bool m_Passthrough;

    long m_drc;

  };

#endif
