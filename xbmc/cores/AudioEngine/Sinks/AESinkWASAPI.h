#pragma once
/*
*      Copyright (C) 2010-2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "../Interfaces/AESink.h"
#include <stdint.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "../Utils/AEDeviceInfo.h"

#include "threads/CriticalSection.h"

class CAESinkWASAPI : public IAESink
{
public:
    virtual const char *GetName() { return "WASAPI"; }

    CAESinkWASAPI();
    virtual ~CAESinkWASAPI();

    virtual bool Initialize  (AEAudioFormat &format, std::string &device);
    virtual void Deinitialize();
    virtual bool IsCompatible(const AEAudioFormat format, const std::string device);

    virtual double       GetDelay                    ();
    virtual double       GetCacheTime                ();
    virtual double       GetCacheTotal               ();
    virtual unsigned int AddPackets                  (uint8_t *data, unsigned int frames, bool hasAudio);
    static  void         EnumerateDevicesEx          (AEDeviceInfoList &deviceInfoList);
private:
    enum SearchStrategy { DefaultOnly = 0, TryHigherBits, TryHigherSampleRates, 
                          TryHigherSampleRatesWODefault, TryHigherSampleRatesOnly};
    bool         InitializeExclusive(AEAudioFormat &format);
    HRESULT      TryAndInitializeExclusive(const AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex_iec61937);
    void         AEChannelsFromSpeakerMask(DWORD speakers);
    static DWORD GetSpeakerMaskFromAEChannels(const CAEChannelInfo &channels);
    void         BuildWaveFormatExtensible(const AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex);
    void         BuildWaveFormatExtensibleIEC61397(const AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex);
    static AEAudioFormat SimplifyFormat(const AEAudioFormat &format);
    bool         FindCompatibleFormatAlmostLossless(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, SearchStrategy strategy, bool formaitIsSimplified = false);
    bool         FindCompatibleFormatHigherBits(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, bool tryDefault = true, bool formaitIsSimplified = false);
    bool         FindCompatibleFormatHigherSampleRate(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, SearchStrategy strategy, bool formaitIsSimplified = false);
    bool         FindCompatibleFormatWithLFE(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, SearchStrategy strategy, bool formaitIsSimplified = false);
    bool         FindCompatibleFormatSideOrBack(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, SearchStrategy strategy, bool formaitIsSimplified = false);
    bool         FindCompatibleFormatAnySampleRate(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, bool checkLowerRates = false, SearchStrategy strategy = TryHigherBits, bool formaitIsSimplified = false);
    bool         FindCompatibleFormatAnyDataFormat(AEAudioFormat format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex, bool checkLowerRatesOnly, bool formaitIsSimplified = false);

    static const char  *WASAPIErrToStr(HRESULT err);

    HANDLE              m_needDataEvent;
    IMMDevice          *m_pDevice;
    IAudioClient       *m_pAudioClient;
    IAudioRenderClient *m_pRenderClient;

    AEAudioFormat       m_format;
    enum AEDataFormat   m_encodedFormat;
    unsigned int        m_encodedChannels;
    unsigned int        m_encodedSampleRate;
    CAEChannelInfo      m_channelLayout;
    std::string         m_device;

    enum AEDataFormat   sinkReqFormat;
    enum AEDataFormat   sinkRetFormat;

    bool                m_running;
    bool                m_initialized;
    bool                m_isDirty;        /* sink output failed - needs re-init or new device */

    unsigned int        m_uiBufferLen;    /* wasapi endpoint buffer size, in frames */
    double              m_avgTimeWaiting; /* time between next buffer of data from SoftAE and driver call for data */
    double              m_sinkLatency;    /* time in seconds of total duration of the two WASAPI buffers */
};