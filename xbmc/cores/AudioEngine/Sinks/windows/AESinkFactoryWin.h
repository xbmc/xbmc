/*
 *      Copyright (C) 2010-2017 Team Kodi
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

#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

#include <audioclient.h>

#define WASAPI_SPEAKER_COUNT 21

#ifdef TARGET_WINDOWS_STORE
#ifndef _WAVEFORMATEXTENSIBLE_IEC61937_
#define _WAVEFORMATEXTENSIBLE_IEC61937_
typedef struct _WAVEFORMATEXTENSIBLE_IEC61937
{
  WAVEFORMATEXTENSIBLE    FormatExt;
  uint32_t                dwEncodedSamplesPerSec;
  uint32_t                dwEncodedChannelCount;
  uint32_t                dwAverageBytesPerSec;
} WAVEFORMATEXTENSIBLE_IEC61937, *PWAVEFORMATEXTENSIBLE_IEC61937;
#endif /* _WAVEFORMATEXTENSIBLE_IEC61937_ */

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS \
    0x0000000aL, 0x0cea, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("0000000a-0cea-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL\
    DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DOLBY_AC3_SPDIF)
DEFINE_GUIDSTRUCT("00000092-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP \
    0x0000000cL, 0x0cea, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("0000000c-0cea-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD \
    0x0000000bL, 0x0cea, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("0000000b-0cea-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DTS\
    DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DTS)
DEFINE_GUIDSTRUCT("00000008-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DTS);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DTS DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DTS)


#define KSAUDIO_SPEAKER_STEREO          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
#define KSAUDIO_SPEAKER_7POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                         SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                         SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | \
                                         SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
#define KSAUDIO_SPEAKER_5POINT1         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                         SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                         SPEAKER_BACK_LEFT  | SPEAKER_BACK_RIGHT)
#endif

static const unsigned int WASAPISampleRateCount = 10;
static const unsigned int WASAPISampleRates[] = { 384000, 192000, 176400, 96000, 88200, 48000, 44100, 32000, 22050, 11025 };

static const enum AEChannel layoutsList[][16] =
{
  /* Most common configurations */
  {AE_CH_FC,  AE_CH_NULL}, // Mono
  {AE_CH_FL,  AE_CH_FR,  AE_CH_NULL}, // Stereo
  {AE_CH_FL,  AE_CH_FR,  AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // Quad
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_BC,  AE_CH_NULL}, // Surround
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_NULL}, // Standard 5.1
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // 5.1 wide (obsolete)
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // Standard 7.1
  /* Less common configurations */
  {AE_CH_FL,  AE_CH_FR,  AE_CH_LFE, AE_CH_NULL}, // 2.1
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_BL,  AE_CH_BR,  AE_CH_FLOC,AE_CH_FROC,AE_CH_NULL}, // 7.1 wide (obsolete)
  /* Exotic configurations */
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_NULL}, // 3 front speakers
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_NULL}, // 3 front speakers + LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_BL,  AE_CH_BR,  AE_CH_LFE, AE_CH_NULL}, // Quad + LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_BC,  AE_CH_LFE, AE_CH_NULL}, // Surround + LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_SL,  AE_CH_SR,  AE_CH_NULL}, // Standard 5.1 w/o LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // 5.1 wide w/o LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_SL,  AE_CH_SR,  AE_CH_BC,  AE_CH_NULL}, // Standard 5.1 w/o LFE + Back Center
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_BL,  AE_CH_BC,  AE_CH_BR,  AE_CH_NULL}, // 5.1 wide w/o LFE + Back Center
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_BL,  AE_CH_BR,  AE_CH_TC,  AE_CH_NULL}, // DVD speakers
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_BL,  AE_CH_BR,  AE_CH_BC,  AE_CH_LFE, AE_CH_NULL}, // 5.1 wide + Back Center
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // Standard 7.1 w/o LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_BL,  AE_CH_BR,  AE_CH_FLOC,AE_CH_FROC,AE_CH_NULL}, // 7.1 wide w/o LFE
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BC,  AE_CH_BR,  AE_CH_NULL}, // Standard 7.1 + Back Center
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_FLOC,AE_CH_FROC,AE_CH_NULL}, // Standard 7.1 + front wide
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_TFL, AE_CH_TFR, AE_CH_NULL}, // Standard 7.1 + 2 front top
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_TFL, AE_CH_TFR, AE_CH_TFC, AE_CH_NULL}, // Standard 7.1 + 3 front top
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_TFL, AE_CH_TFR, AE_CH_TBL, AE_CH_TBR, AE_CH_NULL}, // Standard 7.1 + 2 front top + 2 back top
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_TFL, AE_CH_TFR, AE_CH_TFC, AE_CH_TBL, AE_CH_TBR, AE_CH_NULL}, // Standard 7.1 + 3 front top + 2 back top
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_TFL, AE_CH_TFR, AE_CH_TFC, AE_CH_TBL, AE_CH_TBR, AE_CH_TBC, AE_CH_NULL}, // Standard 7.1 + 3 front top + 3 back top
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_TFL, AE_CH_TFR, AE_CH_TFC, AE_CH_TBL, AE_CH_TBR, AE_CH_TBC, AE_CH_TC,  AE_CH_NULL} // Standard 7.1 + 3 front top + 3 back top + Top Center
};

static enum AEChannel layoutsByChCount[9][9] = {
    {AE_CH_NULL},
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_BC, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_LFE, AE_CH_NULL}};

static const unsigned int WASAPIChannelOrder[] = {AE_CH_RAW,
                                                  SPEAKER_FRONT_LEFT,           SPEAKER_FRONT_RIGHT,           SPEAKER_FRONT_CENTER,
                                                  SPEAKER_LOW_FREQUENCY,        SPEAKER_BACK_LEFT,             SPEAKER_BACK_RIGHT,
                                                  SPEAKER_FRONT_LEFT_OF_CENTER, SPEAKER_FRONT_RIGHT_OF_CENTER,
                                                  SPEAKER_BACK_CENTER,          SPEAKER_SIDE_LEFT,             SPEAKER_SIDE_RIGHT,
                                                  SPEAKER_TOP_FRONT_LEFT,       SPEAKER_TOP_FRONT_RIGHT,       SPEAKER_TOP_FRONT_CENTER,
                                                  SPEAKER_TOP_CENTER,           SPEAKER_TOP_BACK_LEFT,         SPEAKER_TOP_BACK_RIGHT,
                                                  SPEAKER_TOP_BACK_CENTER,      SPEAKER_RESERVED,              SPEAKER_RESERVED};

static const enum AEChannel AEChannelNames[]   = {AE_CH_RAW,
                                                  AE_CH_FL,                     AE_CH_FR,                      AE_CH_FC,
                                                  AE_CH_LFE,                    AE_CH_BL,                      AE_CH_BR,
                                                  AE_CH_FLOC,                   AE_CH_FROC,
                                                  AE_CH_BC,                     AE_CH_SL,                      AE_CH_SR,
                                                  AE_CH_TFL,                    AE_CH_TFR,                     AE_CH_TFC ,
                                                  AE_CH_TC  ,                   AE_CH_TBL,                     AE_CH_TBR,
                                                  AE_CH_TBC,                    AE_CH_BLOC,                    AE_CH_BROC};

struct winEndpointsToAEDeviceType
{
  std::string winEndpointType;
  AEDeviceType aeDeviceType;
};

const winEndpointsToAEDeviceType winEndpoints[] =
{
  { "Network Device - ",         AE_DEVTYPE_PCM },
  { "Speakers - ",               AE_DEVTYPE_PCM },
  { "LineLevel - ",              AE_DEVTYPE_PCM },
  { "Headphones - ",             AE_DEVTYPE_PCM },
  { "Microphone - ",             AE_DEVTYPE_PCM },
  { "Headset - ",                AE_DEVTYPE_PCM },
  { "Handset - ",                AE_DEVTYPE_PCM },
  { "Digital Passthrough - ", AE_DEVTYPE_IEC958 },
  { "SPDIF - ",               AE_DEVTYPE_IEC958 },
  { "HDMI - ",                  AE_DEVTYPE_HDMI },
  { "Unknown - ",                AE_DEVTYPE_PCM },
};

struct sampleFormat
{
  GUID subFormat;
  unsigned int bitsPerSample;
  unsigned int validBitsPerSample;
  AEDataFormat subFormatType;
};

//! @todo
//! Sample formats go from float -> 32 bit int -> 24 bit int (packed in 32) -> -> 24 bit int -> 16 bit int */
//! versions of Kodi before 14.0 had a bug which made S24NE4MSB the first format selected
//! this bug worked around some driver bug of some IEC958 devices which report S32 but can't handle it
//! correctly. So far I have never seen and WASAPI device using S32 and don't think probing S24 before
//! S32 has any negative impact.
static const sampleFormat testFormats[] = { {KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 32, 32, AE_FMT_FLOAT},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 32, 24, AE_FMT_S24NE4MSB},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 32, 32, AE_FMT_S32NE},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 24, 24, AE_FMT_S24NE3},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 16, 16, AE_FMT_S16NE} };

struct RendererDetail
{
  std::string strDeviceId;
  std::string strDescription;
  std::string strWinDevType;
  AEDeviceType eDeviceType;
  unsigned int nChannels;
  unsigned int uiChannelMask;
  bool bDefault;
};

struct IAEWASAPIDevice
{
  std::string deviceId;
  virtual int Release() = 0;
  virtual HRESULT Activate(IAudioClient** ppAudioClient) = 0;
  virtual bool IsUSBDevice() = 0;
};

class CAESinkFactoryWin
{
public:
  /*
    Gets list of audio renderers available on platform
  */
  static std::vector<RendererDetail> GetRendererDetails();
  /*
    Gets default device id
  */
  static std::string GetDefaultDeviceId();
  /*
    Initialize IAEWASAPIDevice
  */
  static HRESULT ActivateWASAPIDevice(std::string &device, IAEWASAPIDevice** ppDevice);

  static void AEChannelsFromSpeakerMask(CAEChannelInfo& channelLayout, DWORD speakers);

  static DWORD SpeakerMaskFromAEChannels(const CAEChannelInfo &channels);

  static DWORD ChLayoutToChMask(const enum AEChannel * layout, unsigned int * numberOfChannels = NULL);

  static void BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex);
};
