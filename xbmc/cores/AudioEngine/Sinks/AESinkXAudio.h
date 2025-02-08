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

#include <x3daudio.h>
#include <xapofx.h>
#include <xaudio2.h>
#include <xaudio2fx.h>

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
          throw std::exception("CreateEventEx BufferEnd");
        }
        if (NULL == (m_StreamEndEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET,
                                                      EVENT_MODIFY_STATE | SYNCHRONIZE)))
        {
          throw std::exception("CreateEventEx StreamEnd");
        }
      }
      virtual ~VoiceCallback()
      {
        if (m_StreamEndEvent != NULL)
          CloseHandle(m_StreamEndEvent);
      }

      STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) override {}
      STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
      STDMETHOD_(void, OnStreamEnd)() override
      {
        if (m_StreamEndEvent != NULL)
          SetEvent(m_StreamEndEvent);
      }
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
      HANDLE m_StreamEndEvent{0};
    };

    bool InitializeInternal(std::string deviceId, AEAudioFormat& format);

    /*!
     * \brief Add a 1 frame long buffer with the end of stream flag to the voice.
     * \return true for success, false for failure
     */
    bool AddEndOfStreamPacket();
    /*!
     * \brief Create a XAUDIO2_BUFFER with a struct buffer_ctx in pContext member, which must
     * be deleted either manually or by XAudio2 BufferEnd callback to avoid memory leaks.
     * \param data data of the frames to copy. if null, the new buffer will contain silence.
     * \param frames number of frames
     * \param offset offset from the start in the data buffer.
     * \return the new buffer
     */
    XAUDIO2_BUFFER BuildXAudio2Buffer(uint8_t** data, unsigned int frames, unsigned int offset);

    Microsoft::WRL::ComPtr<IXAudio2> m_xAudio2;
    IXAudio2MasteringVoice* m_masterVoice{nullptr};
    IXAudio2SourceVoice* m_sourceVoice{nullptr};
    VoiceCallback m_voiceCallback;

    AEAudioFormat m_format;

    uint64_t m_sinkFrames{0};
    std::atomic<uint16_t> m_framesInBuffers{0};

    // time between next buffer of data from SoftAE and driver call for data
    double m_avgTimeWaiting{50.0};

    bool m_running{false};
    bool m_initialized{false};
    bool m_isSuspended{false}; // sink is in a suspended state - release audio device
    bool m_isDirty{false}; // sink output failed - needs re-init or new device

    LARGE_INTEGER m_timerFreq{}; // performance counter frequency for latency calculations
};
