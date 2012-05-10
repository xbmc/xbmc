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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
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
    virtual unsigned int AddPackets                  (uint8_t *data, unsigned int frames);
    static  void         EnumerateDevicesEx          (AEDeviceInfoList &deviceInfoList);
private:
    bool         InitializeExclusive(AEAudioFormat &format);
    void         AEChannelsFromSpeakerMask(DWORD speakers);
    DWORD        SpeakerMaskFromAEChannels(const CAEChannelInfo &channels);
    void         BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex);
    void         BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex);

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

    unsigned int        m_uiBufferLen;    /* wasapi endpoint buffer size, in frames */
    double              m_avgTimeWaiting; /* time between next buffer of data from SoftAE and driver call for data */
};