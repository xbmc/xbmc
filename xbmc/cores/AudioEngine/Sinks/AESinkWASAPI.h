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

#pragma once

#include <stdint.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Sinks/windows/AESinkFactoryWin.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include <wrl/client.h>

class CAESinkWASAPI : public IAESink
{
public:
    CAESinkWASAPI();
    virtual ~CAESinkWASAPI();

    static void Register();
    static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);
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

    HANDLE m_needDataEvent;
    IAEWASAPIDevice* m_pDevice;
    Microsoft::WRL::ComPtr<IAudioClient> m_pAudioClient;
    Microsoft::WRL::ComPtr<IAudioRenderClient> m_pRenderClient;
    Microsoft::WRL::ComPtr<IAudioClock> m_pAudioClock;

    AEAudioFormat       m_format;
    unsigned int        m_encodedChannels;
    unsigned int        m_encodedSampleRate;
    CAEChannelInfo      m_channelLayout;
    std::string         m_device;

    enum AEDataFormat   sinkReqFormat;
    enum AEDataFormat   sinkRetFormat;

    bool                m_running;
    bool                m_initialized;
    bool                m_isSuspended;    /* sink is in a suspended state - release audio device */
    bool                m_isDirty;        /* sink output failed - needs re-init or new device */

    unsigned int        m_uiBufferLen;    /* wasapi endpoint buffer size, in frames */
    double              m_avgTimeWaiting; /* time between next buffer of data from SoftAE and driver call for data */
    double              m_sinkLatency;    /* time in seconds of total duration of the two WASAPI buffers */
    uint64_t            m_sinkFrames;
    uint64_t            m_clockFreq;

    uint8_t            *m_pBuffer;
    int                 m_bufferPtr;
};
