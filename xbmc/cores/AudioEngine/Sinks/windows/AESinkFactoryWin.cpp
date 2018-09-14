/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AESinkFactoryWin.h"
#include "utils/log.h"

#define ERRTOSTR(err) case err: return #err

const char *WASAPIErrToStr(HRESULT err)
{
  switch (err)
  {
    ERRTOSTR(AUDCLNT_E_NOT_INITIALIZED);
    ERRTOSTR(AUDCLNT_E_ALREADY_INITIALIZED);
    ERRTOSTR(AUDCLNT_E_WRONG_ENDPOINT_TYPE);
    ERRTOSTR(AUDCLNT_E_DEVICE_INVALIDATED);
    ERRTOSTR(AUDCLNT_E_NOT_STOPPED);
    ERRTOSTR(AUDCLNT_E_BUFFER_TOO_LARGE);
    ERRTOSTR(AUDCLNT_E_OUT_OF_ORDER);
    ERRTOSTR(AUDCLNT_E_UNSUPPORTED_FORMAT);
    ERRTOSTR(AUDCLNT_E_INVALID_SIZE);
    ERRTOSTR(AUDCLNT_E_DEVICE_IN_USE);
    ERRTOSTR(AUDCLNT_E_BUFFER_OPERATION_PENDING);
    ERRTOSTR(AUDCLNT_E_THREAD_NOT_REGISTERED);
    ERRTOSTR(AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED);
    ERRTOSTR(AUDCLNT_E_ENDPOINT_CREATE_FAILED);
    ERRTOSTR(AUDCLNT_E_SERVICE_NOT_RUNNING);
    ERRTOSTR(AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED);
    ERRTOSTR(AUDCLNT_E_EXCLUSIVE_MODE_ONLY);
    ERRTOSTR(AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL);
    ERRTOSTR(AUDCLNT_E_EVENTHANDLE_NOT_SET);
    ERRTOSTR(AUDCLNT_E_INCORRECT_BUFFER_SIZE);
    ERRTOSTR(AUDCLNT_E_BUFFER_SIZE_ERROR);
    ERRTOSTR(AUDCLNT_E_CPUUSAGE_EXCEEDED);
    ERRTOSTR(AUDCLNT_E_BUFFER_ERROR);
    ERRTOSTR(AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED);
    ERRTOSTR(AUDCLNT_E_INVALID_DEVICE_PERIOD);
    ERRTOSTR(E_POINTER);
    ERRTOSTR(E_INVALIDARG);
    ERRTOSTR(E_OUTOFMEMORY);
  default: break;
  }
  return NULL;
}

  void CAESinkFactoryWin::AEChannelsFromSpeakerMask(CAEChannelInfo& channelLayout, DWORD speakers)
  {
    channelLayout.Reset();

    for (int i = 0; i < WASAPI_SPEAKER_COUNT; i++)
    {
      if (speakers & WASAPIChannelOrder[i])
        channelLayout += AEChannelNames[i];
    }

  };

  DWORD CAESinkFactoryWin::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
  {
    DWORD mask = 0;

    for (unsigned int i = 0; i < channels.Count(); i++)
    {
      for (unsigned int j = 0; j < WASAPI_SPEAKER_COUNT; j++)
        if (channels[i] == AEChannelNames[j])
          mask |= WASAPIChannelOrder[j];
    }
    return mask;
  };

  DWORD CAESinkFactoryWin::ChLayoutToChMask(const enum AEChannel * layout, unsigned int * numberOfChannels /*= NULL*/)
  {
    if (numberOfChannels)
      *numberOfChannels = 0;
    if (!layout)
      return 0;

    DWORD mask = 0;
    unsigned int i;
    for (i = 0; layout[i] != AE_CH_NULL; i++)
      mask |= WASAPIChannelOrder[layout[i]];

    if (numberOfChannels)
      *numberOfChannels = i;

    return mask;
  };

  void CAESinkFactoryWin::BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex)
  {
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    if (format.m_dataFormat != AE_FMT_RAW) // PCM data
    {
      wfxex.dwChannelMask = CAESinkFactoryWin::SpeakerMaskFromAEChannels(format.m_channelLayout);
      wfxex.Format.nChannels = (WORD)format.m_channelLayout.Count();
      wfxex.Format.nSamplesPerSec = format.m_sampleRate;
      wfxex.Format.wBitsPerSample = CAEUtil::DataFormatToBits((AEDataFormat)format.m_dataFormat);
      wfxex.SubFormat = format.m_dataFormat < AE_FMT_FLOAT ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    }
    else //Raw bitstream
    {
      wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
      if (format.m_dataFormat == AE_FMT_RAW &&
         ((format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_AC3) ||
          (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3) ||
          (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_CORE) ||
          (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_2048) ||
          (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_1024) ||
          (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_512)))
      {
        if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3)
          wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
        else
          wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
        wfxex.dwChannelMask = bool(format.m_channelLayout.Count() == 2) ? KSAUDIO_SPEAKER_STEREO : KSAUDIO_SPEAKER_5POINT1;
        wfxex.Format.wBitsPerSample = 16;
        wfxex.Samples.wValidBitsPerSample = 16;
        wfxex.Format.nChannels = (WORD)format.m_channelLayout.Count();
        wfxex.Format.nSamplesPerSec = format.m_sampleRate;
        if (format.m_streamInfo.m_sampleRate == 0)
          CLog::Log(LOGERROR, "Invalid sample rate supplied for RAW format");
      }
      else if (format.m_dataFormat == AE_FMT_RAW &&
        ((format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_MA) ||
        (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD) ||
        (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD)))
      {
        // IEC 61937 transmissions over HDMI
        wfxex.Format.nSamplesPerSec = 192000L;
        wfxex.Format.wBitsPerSample = 16;
        wfxex.Samples.wValidBitsPerSample = 16;
        wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;

        switch (format.m_streamInfo.m_type)
        {
        case CAEStreamInfo::STREAM_TYPE_TRUEHD:
          wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
          wfxex.Format.nChannels = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
        case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
          wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
          wfxex.Format.nChannels = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
        case CAEStreamInfo::STREAM_TYPE_DTSHD:
          wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
          wfxex.Format.nChannels = 2; // One IEC 60958 Lines.
          wfxex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
          break;
        }

        if (format.m_channelLayout.Count() == 8)
          wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
        else
          wfxex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
      }
    }

    if (format.m_dataFormat == AE_FMT_S24NE4MSB)
      wfxex.Samples.wValidBitsPerSample = 24;
    else
      wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;

    wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
  };
