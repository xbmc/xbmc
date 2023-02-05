/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Sinks/windows/AESinkFactoryWin.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include <stdint.h>
#include <vector>

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>

class CAESinkWASAPI : public IAESink
{
public:
    CAESinkWASAPI();
    virtual ~CAESinkWASAPI();

    static void Register();
    static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
    static void EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force = false);

    // IAESink overrides
    const char *GetName() override { return "WASAPI"; }
    bool Initialize(AEAudioFormat &format, std::string &device) override;
    void Deinitialize() override;
    void GetDelay(AEDelayStatus& status) override;
    double GetCacheTotal() override;
    unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
    void Drain() override;

private:
    bool InitializeExclusive(AEAudioFormat &format);
    static void BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex);
    bool IsUSBDevice();

    HANDLE m_needDataEvent{0};
    IAEWASAPIDevice* m_pDevice{nullptr};
    Microsoft::WRL::ComPtr<IAudioClient> m_pAudioClient;
    Microsoft::WRL::ComPtr<IAudioRenderClient> m_pRenderClient;
    Microsoft::WRL::ComPtr<IAudioClock> m_pAudioClock;

    AEAudioFormat m_format{};
    unsigned int m_encodedChannels{0};
    unsigned int m_encodedSampleRate{0};
    CAEChannelInfo m_channelLayout;
    std::string m_device;

    enum AEDataFormat sinkReqFormat = AE_FMT_INVALID;
    enum AEDataFormat sinkRetFormat = AE_FMT_INVALID;

    bool m_running{false};
    bool m_initialized{false};
    bool m_isSuspended{false}; // sink is in a suspended state - release audio device
    bool m_isDirty{false}; // sink output failed - needs re-init or new device

    // time between next buffer of data from SoftAE and driver call for data
    double m_avgTimeWaiting{50.0};
    double m_sinkLatency{0.0}; // time in seconds of total duration of the two WASAPI buffers

    unsigned int m_uiBufferLen{0}; // wasapi endpoint buffer size, in frames
    uint64_t m_sinkFrames{0};
    uint64_t m_clockFreq{0};

    std::vector<uint8_t> m_buffer;
    int m_bufferPtr{0};
};
