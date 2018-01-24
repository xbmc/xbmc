#pragma once
/*
*      Copyright (C) 2010-2013 Team XBMC
*      http://kodi.tv
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include <stdint.h>
#include <mmdeviceapi.h>
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include <wrl/implements.h>
#include <ppltasks.h>

#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <xapofx.h>
#pragma comment(lib,"xaudio2.lib")

struct VoiceCallback : public IXAudio2VoiceCallback
{
  VoiceCallback()
  {
    mBufferEnd.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!mBufferEnd)
    {
      throw std::exception("CreateEvent");
    }
  }
  virtual ~VoiceCallback() { }

  STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) override {}
  STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
  STDMETHOD_(void, OnStreamEnd)() override {}
  STDMETHOD_(void, OnBufferStart)(void*) override {}
  STDMETHOD_(void, OnBufferEnd)(void* context) override 
  {
    SetEvent(mBufferEnd.get());
    uint8_t *buff = static_cast<uint8_t*>(context);
    delete[] buff;
  }

  STDMETHOD_(void, OnLoopEnd)(void*) override {}
  STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

  struct handle_closer 
  { 
    void operator()(HANDLE h) 
    { 
      assert(h != INVALID_HANDLE_VALUE); 
      if (h) 
        CloseHandle(h); 
    } 
  };
  std::unique_ptr<void, handle_closer> mBufferEnd;
};

class CAESinkXAudio : public IAESink
{
public:
    virtual const char *GetName() { return "XAudio"; }

    CAESinkXAudio();
    virtual ~CAESinkXAudio();

    static void Register();
    static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);

    bool Initialize (AEAudioFormat &format, std::string &device) override;
    void Deinitialize() override;

    void GetDelay(AEDelayStatus& status) override;
    double GetCacheTotal() override;
    double GetLatency() override; 
    unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
    void Drain() override;

    static void EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force = false);

private:
    bool InitializeInternal(std::string deviceId, AEAudioFormat &format);
    bool IsUSBDevice();

    Microsoft::WRL::ComPtr<IXAudio2> m_xAudio2;
    IXAudio2MasteringVoice* m_masterVoice;
    IXAudio2SourceVoice* m_sourceVoice;
    VoiceCallback m_voiceCallback;

    AEAudioFormat m_format;
    unsigned int m_encodedChannels;
    unsigned int m_encodedSampleRate;
    CAEChannelInfo m_channelLayout;
    std::string m_device;

    enum AEDataFormat sinkReqFormat;
    enum AEDataFormat sinkRetFormat;

    unsigned int m_uiBufferLen;    /* xaudio endpoint buffer size, in frames */
    unsigned int m_AvgBytesPerSec;
    unsigned int m_dwChunkSize;
    unsigned int m_dwFrameSize;
    unsigned int m_dwBufferLen;

    double m_avgTimeWaiting;       /* time between next buffer of data from SoftAE and driver call for data */

    bool m_running;
    bool m_initialized;
    bool m_isSuspended;            /* sink is in a suspended state - release audio device */
    bool m_isDirty;                /* sink output failed - needs re-init or new device */
};
