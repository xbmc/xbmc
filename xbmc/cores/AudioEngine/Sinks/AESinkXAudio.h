/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include <stdint.h>

#include <mmdeviceapi.h>
#include <ppltasks.h>
#include <wrl/implements.h>
#include <x3daudio.h>
#include <xapofx.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#pragma comment(lib,"xaudio2.lib")

class CAESinkXAudio : public IAESink
{
public:
    virtual const char *GetName() { return "XAudio"; }

    CAESinkXAudio();
    virtual ~CAESinkXAudio();

    static void Register();
    static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);

    bool Initialize (AEAudioFormat &format, std::string &device) override;
    void Deinitialize() override;

    void GetDelay(AEDelayStatus& status) override;
    double GetCacheTotal() override;
    double GetLatency() override;
    unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
    void Drain() override;

    static void EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force = false);

private:
    struct buffer_ctx
    {
      uint8_t *data;
      uint32_t frames;
      CAESinkXAudio* sink;

      ~buffer_ctx()
      {
        delete[] data;
        sink->m_framesInBuffers -= frames;
        sink = nullptr;
      }
    };

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
        struct buffer_ctx *ctx = static_cast<struct buffer_ctx*>(context);
        delete ctx;
      }

      STDMETHOD_(void, OnLoopEnd)(void*) override {}
      STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

      struct handle_closer
      {
        void operator()(HANDLE h) const
        {
          assert(h != INVALID_HANDLE_VALUE);
          if (h)
            CloseHandle(h);
        }
      };
      std::unique_ptr<void, handle_closer> mBufferEnd;
    };

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
    uint64_t m_sinkFrames;
    std::atomic<uint16_t> m_framesInBuffers;

    double m_avgTimeWaiting;       /* time between next buffer of data from SoftAE and driver call for data */

    bool m_running;
    bool m_initialized;
    bool m_isSuspended;            /* sink is in a suspended state - release audio device */
    bool m_isDirty;                /* sink output failed - needs re-init or new device */
};
