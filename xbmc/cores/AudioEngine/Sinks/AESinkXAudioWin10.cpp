/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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

#include "AESinkXAudioWin10.h"
#include "cores/AudioEngine/Sinks/AESinkWASAPIWin10.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "platform/win10/AsyncHelpers.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"

#include <algorithm>
#include <collection.h>
#include <ksmedia.h>
#include <mfapi.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <stdint.h>
#include <wrl/implements.h>

// TARGET_WINDOWS_STORE version will use Windows::Medio::Audio
// https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/audio-graphs

using namespace Windows::Media::Devices;
using namespace Microsoft::WRL;

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

static const unsigned int WASAPISampleRateCount = 10;
static const unsigned int WASAPISampleRates[] = {384000, 192000, 176400, 96000, 88200, 48000, 44100, 32000, 22050, 11025};

#define WASAPI_SPEAKER_COUNT 21
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

struct winEndpointsToAEDeviceType
{
  std::string winEndpointType;
  AEDeviceType aeDeviceType;
};

static const winEndpointsToAEDeviceType winEndpoints[] =
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

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason " - %s", __VA_ARGS__, WASAPIErrToStr(hr)); goto failed;}
#define ERRTOSTR(err) case err: return #err
#define SAFE_DESTROY_VOICE(x) do { if(x) { x->DestroyVoice(); x = nullptr; } }while(0);
#define XAUDIO_BUFFERS_IN_QUEUE 2

extern std::vector<RendererDetail> GetRendererDetails();
extern DWORD ChLayoutToChMask(const enum AEChannel * layout, unsigned int * numberOfChannels = nullptr);
extern std::string localWideToUtf(LPCWSTR wstr);
extern std::wstring localUtfToWide(const std::string &text);

///  ----------------- CAESinkXAudioWin10 ------------------------

CAESinkXAudioWin10::CAESinkXAudioWin10() :
  m_masterVoice(nullptr),
  m_sourceVoice(nullptr),
  m_encodedChannels(0),
  m_encodedSampleRate(0),
  sinkReqFormat(AE_FMT_INVALID),
  sinkRetFormat(AE_FMT_INVALID),
  m_AvgBytesPerSec(0),
  m_dwChunkSize(0),
  m_dwFrameSize(0),
  m_dwBufferLen(0),
  m_running(false),
  m_initialized(false),
  m_isSuspended(false),
  m_isDirty(false),
  m_uiBufferLen(0),
  m_avgTimeWaiting(50)
{
  m_channelLayout.Reset();

  HRESULT hr = XAudio2Create(m_xAudio2.ReleaseAndGetAddressOf(), 0);
  if (FAILED(hr))
  {
    CLog::LogFunction(LOGERROR, "CAESinkXAudioWin10", "XAudio initialization failed.");
  }
#ifdef  _DEBUG
  else
  {
    XAUDIO2_DEBUG_CONFIGURATION config = { 0 };
    config.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_STREAMING;
    config.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_STREAMING;
    config.LogTiming = true;
    m_xAudio2->SetDebugConfiguration(&config, 0);
  }
#endif //  _DEBUG
}

CAESinkXAudioWin10::~CAESinkXAudioWin10()
{
  if (m_xAudio2)
    m_xAudio2.Reset();
}

bool CAESinkXAudioWin10::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  m_device = device;
  bool bdefault = false;
  HRESULT hr = S_OK;

  /* Save requested format */
  /* Clear returned format */
  sinkReqFormat = format.m_dataFormat;
  sinkRetFormat = AE_FMT_INVALID;

  if (!InitializeExclusive(device, format))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Could not Initialize voices with that format");
    goto failed;
  }

  format.m_frames       = m_uiBufferLen;
  m_format              = format;
  sinkRetFormat         = format.m_dataFormat;

  m_initialized = true;
  m_isDirty     = false;

  return true;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": XAudio initialization failed.");
  return true;
}

void CAESinkXAudioWin10::Deinitialize()
{
  if (!m_initialized && !m_isDirty)
    return;

  if (m_running)
  {
    try
    {
      m_sourceVoice->Stop();
      m_sourceVoice->FlushSourceBuffers();
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "%s: Invalidated AudioClient - Releasing", __FUNCTION__);
    }
  }
  m_running = false;

  SAFE_DESTROY_VOICE(m_sourceVoice);
  SAFE_DESTROY_VOICE(m_masterVoice);

  m_initialized = false;
}

/**
 * @brief rescale uint64_t without overflowing on large values
 */
static uint64_t rescale_u64(uint64_t val, uint64_t num, uint64_t den)
{
  return ((val / den) * num) + (((val % den) * num) / den);
}

void CAESinkXAudioWin10::GetDelay(AEDelayStatus& status)
{
  HRESULT hr = S_OK;
  uint64_t pos = 0, tick = 0;
  int retries = 0;

  if (!m_initialized)
    goto failed;

  XAUDIO2_VOICE_STATE state;
  m_sourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
  
  uint64_t framesInQueue = state.BuffersQueued * m_format.m_frames;
  status.SetDelay(framesInQueue / (double)m_format.m_sampleRate);
  return;

failed:
  status.SetDelay(0);
}

double CAESinkXAudioWin10::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  return XAUDIO_BUFFERS_IN_QUEUE * m_format.m_frames / (double)m_format.m_sampleRate;
}

double CAESinkXAudioWin10::GetLatency()
{
  if (!m_initialized || !m_xAudio2)
    return 0.0;

  XAUDIO2_PERFORMANCE_DATA perfData;
  m_xAudio2->GetPerformanceData(&perfData);

  return perfData.CurrentLatencyInSamples / (double) m_format.m_sampleRate;
}

unsigned int CAESinkXAudioWin10::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized)
    return 0;

  HRESULT hr = S_OK;
  DWORD flags = 0;

#ifndef _DEBUG
  LARGE_INTEGER timerStart;
  LARGE_INTEGER timerStop;
  LARGE_INTEGER timerFreq;
#endif
  size_t dataLenght = frames * m_format.m_frameSize;
  uint8_t* buff = new uint8_t[dataLenght];
  memcpy(buff, data[0] + offset * m_format.m_frameSize, dataLenght);

  XAUDIO2_BUFFER xbuffer = { 0 };
  xbuffer.AudioBytes = dataLenght;
  xbuffer.pAudioData = buff;
  xbuffer.pContext = buff;

  if (!m_running) //first time called, pre-fill buffer then start voice
  {
    hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__ " SourceVoice submit buffer failed due to %s", WASAPIErrToStr(hr));
      delete[] buff;
      return 0;
    }
    hr = m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__ " SourceVoice start failed due to %s", WASAPIErrToStr(hr));
      m_isDirty = true; //flag new device or re-init needed
      delete[] buff;
      return INT_MAX;
    }
    m_running = true; //signal that we're processing frames
    return frames;
  }

#ifndef _DEBUG
  /* Get clock time for latency checks */
  QueryPerformanceFrequency(&timerFreq);
  QueryPerformanceCounter(&timerStart);
#endif

  /* Wait for Audio Driver to tell us it's got a buffer available */
  XAUDIO2_VOICE_STATE state;
  while (m_sourceVoice->GetState(&state), state.BuffersQueued >= XAUDIO_BUFFERS_IN_QUEUE)
  {
    DWORD eventAudioCallback;
    eventAudioCallback = WaitForSingleObjectEx(m_voiceCallback.mBufferEnd.get(), 1100, TRUE);
    if (eventAudioCallback != WAIT_OBJECT_0)
    {
      CLog::Log(LOGERROR, __FUNCTION__": Endpoint Buffer timed out");
      delete[] buff;
      return INT_MAX;
    }
  }

  if (!m_running)
    return 0;

#ifndef _DEBUG
  QueryPerformanceCounter(&timerStop);
  LONGLONG timerDiff = timerStop.QuadPart - timerStart.QuadPart;
  double timerElapsed = (double) timerDiff * 1000.0 / (double) timerFreq.QuadPart;
  m_avgTimeWaiting += (timerElapsed - m_avgTimeWaiting) * 0.5;

  if (m_avgTimeWaiting < 3.0)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Possible AQ Loss: Avg. Time Waiting for Audio Driver callback : %dmsec", (int)m_avgTimeWaiting);
  }
#endif

  hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
  if (FAILED(hr))
  {
    #ifdef _DEBUG
      CLog::Log(LOGERROR, __FUNCTION__": SubmitSourceBuffer failed due to %s", WASAPIErrToStr(hr));
    #endif
    return INT_MAX;
  }

  return frames;
}

void CAESinkXAudioWin10::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  HRESULT hr = S_OK, hr2 = S_OK;
  CAEDeviceInfo deviceInfo;
  CAEChannelInfo deviceChannels;
  WAVEFORMATEXTENSIBLE wfxex = { 0 };
  bool add192 = false;

  try
  {
    UINT32 eflags = 0;// XAUDIO2_DEBUG_ENGINE;
    Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
    HRESULT hr = XAudio2Create(xaudio2.ReleaseAndGetAddressOf(), eflags);
    if (FAILED(hr))
    {
      CLog::Log(LOGDEBUG, __FUNCTION__": Failed to activate XAudio for capability testing.");
      goto failed;
    }

    IXAudio2MasteringVoice* mMasterVoice = nullptr;
    IXAudio2SourceVoice* mSourceVoice = nullptr;

    for(RendererDetail& details : GetRendererDetails())
    {
      deviceInfo.m_channels.Reset();
      deviceInfo.m_dataFormats.clear();
      deviceInfo.m_sampleRates.clear();

      std::wstring deviceId = localUtfToWide(details.strDeviceId);

      /* Test format DTS-HD */
      wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
      wfxex.Format.nSamplesPerSec = 192000;
      wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
      wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
      wfxex.Format.wBitsPerSample = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels = 8;
      wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                          0, deviceId.c_str(), nullptr, AudioCategory_Media);
      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);

      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if (FAILED(hr))
          CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_DTSHD), details.strDescription.c_str());

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
        add192 = true;
      }
      SAFE_DESTROY_VOICE(mSourceVoice);

      /* Test format Dolby TrueHD */
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;

      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if (FAILED(hr))
          CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_TRUEHD), details.strDescription.c_str());

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
        add192 = true;
      }

      /* Test format Dolby EAC3 */
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
      wfxex.Format.nChannels = 2;
      wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      SAFE_DESTROY_VOICE(mMasterVoice);
      SAFE_DESTROY_VOICE(mSourceVoice);
      hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                          0, deviceId.c_str(), nullptr, AudioCategory_Media);
      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);

      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if (FAILED(hr))
          CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_EAC3), details.strDescription.c_str());

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
        add192 = true;
      }

      /* Test format DTS */
      wfxex.Format.nSamplesPerSec = 48000;
      wfxex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
      wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      SAFE_DESTROY_VOICE(mMasterVoice);
      SAFE_DESTROY_VOICE(mSourceVoice);
      hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                          0, deviceId.c_str(), nullptr, AudioCategory_Media);
      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if (FAILED(hr))
          CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", "STREAM_TYPE_DTS", details.strDescription.c_str());

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
      }
      SAFE_DESTROY_VOICE(mSourceVoice);

      /* Test format Dolby AC3 */
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;

      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if (FAILED(hr))
          CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_AC3), details.strDescription.c_str());

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
      }

      /* Test format for PCM format iteration */
      wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
      wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

      for (int p = AE_FMT_FLOAT; p > AE_FMT_INVALID; p--)
      {
        if (p < AE_FMT_FLOAT)
          wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        wfxex.Format.wBitsPerSample = CAEUtil::DataFormatToBits((AEDataFormat)p);
        wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        if (p == AE_FMT_S24NE4MSB)
        {
          wfxex.Samples.wValidBitsPerSample = 24;
        }
        else if (p <= AE_FMT_S24NE4 && p >= AE_FMT_S24BE4)
        {
          // not supported
          continue;
        }
        else
        {
          wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
        }

        SAFE_DESTROY_VOICE(mSourceVoice);
        hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);

        if (SUCCEEDED(hr))
          deviceInfo.m_dataFormats.push_back((AEDataFormat)p);
      }

      /* Test format for sample rate iteration */
      wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
      wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
      wfxex.Format.wBitsPerSample = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels = 2;
      wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      for (int j = 0; j < WASAPISampleRateCount; j++)
      {
        SAFE_DESTROY_VOICE(mMasterVoice);
        SAFE_DESTROY_VOICE(mSourceVoice);

        wfxex.Format.nSamplesPerSec = WASAPISampleRates[j];
        wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

        hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                            0, deviceId.c_str(), nullptr, AudioCategory_Media);
        hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
        if (SUCCEEDED(hr))
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
        else if (wfxex.Format.nSamplesPerSec == 192000 && add192)
        {
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
          CLog::Log(LOGNOTICE, __FUNCTION__": sample rate 192khz on device \"%s\" seems to be not supported.", details.strDescription.c_str());
        }
      }
      SAFE_DESTROY_VOICE(mMasterVoice);
      SAFE_DESTROY_VOICE(mSourceVoice);

      deviceInfo.m_deviceName = details.strDeviceId;
      deviceInfo.m_displayName = details.strWinDevType.append(details.strDescription);
      deviceInfo.m_displayNameExtra = std::string("XAudio: ").append(details.strDescription);
      deviceInfo.m_deviceType = details.eDeviceType;
      deviceInfo.m_channels = layoutsByChCount[details.nChannels];

      /* Store the device info */
      deviceInfo.m_wantsIECPassthrough = true;

      if (!deviceInfo.m_streamTypes.empty())
        deviceInfo.m_dataFormats.push_back(AE_FMT_RAW);

      deviceInfoList.push_back(deviceInfo);

      if (details.bDefault)
      {
        deviceInfo.m_deviceName = std::string("default");
        deviceInfo.m_displayName = std::string("default");
        deviceInfo.m_displayNameExtra = std::string("");
        deviceInfo.m_wantsIECPassthrough = true;
        deviceInfoList.push_back(deviceInfo);
      }
    }
  }
  catch(Platform::Exception^ ex)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Throws an exception.");
  }

failed:

  if (FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate WASAPI endpoint devices (%s).", WASAPIErrToStr(hr));
}

/// ------------------- Private utility functions -----------------------------------

void CAESinkXAudioWin10::BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex)
{
  wfxex.Format.wFormatTag        = WAVE_FORMAT_EXTENSIBLE;
  wfxex.Format.cbSize            = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);

#if 1

  if (format.m_dataFormat != AE_FMT_RAW) // PCM data
  {
    wfxex.dwChannelMask          = SpeakerMaskFromAEChannels(format.m_channelLayout);
    wfxex.Format.nChannels       = (WORD)format.m_channelLayout.Count();
    wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
    wfxex.Format.wBitsPerSample  = CAEUtil::DataFormatToBits((AEDataFormat) format.m_dataFormat);
    wfxex.SubFormat              = format.m_dataFormat < AE_FMT_FLOAT ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
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
      wfxex.dwChannelMask               = bool (format.m_channelLayout.Count() == 2) ? KSAUDIO_SPEAKER_STEREO : KSAUDIO_SPEAKER_5POINT1;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = (WORD)format.m_channelLayout.Count();
      wfxex.Format.nSamplesPerSec       = format.m_sampleRate;
      if (format.m_streamInfo.m_sampleRate == 0)
      CLog::Log(LOGERROR, "Invalid sample rate supplied for RAW format");
    }
    else if (format.m_dataFormat == AE_FMT_RAW &&
             ((format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD) ||
              (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)))
    {
      // IEC 61937 transmissions over HDMI       
      wfxex.Format.nSamplesPerSec       = 192000L;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_7POINT1_SURROUND;

      switch (format.m_streamInfo.m_type)
      {
        case CAEStreamInfo::STREAM_TYPE_TRUEHD:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
          wfxex.Format.nChannels      = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
        case CAEStreamInfo::STREAM_TYPE_DTSHD:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
          wfxex.Format.nChannels      = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
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

  wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
#endif
}

bool CAESinkXAudioWin10::InitializeExclusive(std::string deviceId, AEAudioFormat &format)
{
  std::wstring device = localUtfToWide(deviceId);
  WAVEFORMATEXTENSIBLE wfxex = { 0 };

  if ( format.m_dataFormat <= AE_FMT_FLOAT
    || format.m_dataFormat == AE_FMT_RAW)
    BuildWaveFormatExtensible(format, wfxex);
  else
  {
    // planar formats are currently not supported by this sink
    format.m_dataFormat = AE_FMT_FLOAT;
    BuildWaveFormatExtensible(format, wfxex);
  }

  /* Test for incomplete format and provide defaults */
  if (format.m_sampleRate == 0 ||
      format.m_channelLayout == CAEChannelInfo(nullptr) ||
      format.m_dataFormat <= AE_FMT_INVALID ||
      format.m_dataFormat >= AE_FMT_MAX ||
      format.m_channelLayout.Count() == 0)
  {
    wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    wfxex.Format.nChannels            = 2;
    wfxex.Format.nSamplesPerSec       = 44100L;
    wfxex.Format.wBitsPerSample       = 16;
    wfxex.Format.nBlockAlign          = 4;
    wfxex.Samples.wValidBitsPerSample = 16;
    wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nBlockAlign * wfxex.Format.nSamplesPerSec;
    wfxex.dwChannelMask               = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
  }

  bool bdefault = StringUtils::EndsWithNoCase(deviceId, std::string("default"));

  HRESULT hr;
  IXAudio2MasteringVoice* pMasterVoice = nullptr;

  if (!bdefault)
  {
    try
    {
      hr = m_xAudio2->CreateMasteringVoice(&pMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                           0, device.c_str(), nullptr, AudioCategory_Media);
    }
    catch (...) {}
  }

  if (!pMasterVoice)
  {
    if (!bdefault)
      CLog::Log(LOGINFO, __FUNCTION__": Could not locate the device named \"%s\" in the list of WASAPI endpoint devices.  Trying the default device...", device.c_str());

    Platform::String^ deviceIdRT = MediaDevice::GetDefaultAudioRenderId(Windows::Media::Devices::AudioDeviceRole::Default);
    device = std::wstring(deviceIdRT->Data());
    try
    {
      hr = m_xAudio2->CreateMasteringVoice(&pMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                           0, device.c_str(), nullptr, AudioCategory_Media);
    }
    catch (...) {}

    if (!pMasterVoice)
    {
      CLog::Log(LOGINFO, __FUNCTION__": Could not retrieve the default WASAPI audio endpoint.");
      return false;
    }
  }

  m_masterVoice = pMasterVoice;

  hr = m_xAudio2->CreateSourceVoice(&m_sourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceCallback, nullptr, nullptr);
  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Format is Supported - will attempt to Initialize");
    goto initialize;
  }

  if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT) //It failed for a reason unrelated to an unsupported format.
  {
    CLog::Log(LOGERROR, __FUNCTION__": IsFormatSupported failed (%s)", WASAPIErrToStr(hr));
    return false;
  }
  else if (format.m_dataFormat == AE_FMT_RAW) //No sense in trying other formats for passthrough.
    return false;

  if (g_advancedSettings.CanLogComponent(LOGAUDIO))
    CLog::Log(LOGDEBUG, __FUNCTION__": IsFormatSupported failed (%s) - trying to find a compatible format", WASAPIErrToStr(hr));

  int closestMatch;
  unsigned int requestedChannels = wfxex.Format.nChannels;
  unsigned int noOfCh;

  /* The requested format is not supported by the device.  Find something that works */
  for (int layout = -1; layout <= (int)ARRAYSIZE(layoutsList); layout++)
  {
    // if requested layout is not supported, try standard layouts with at least
    // the number of channels as requested
    // as the last resort try stereo
    if (layout == ARRAYSIZE(layoutsList))
    {
      wfxex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
      wfxex.Format.nChannels = 2;
    }
    else if (layout >= 0)
    {
      wfxex.dwChannelMask = ChLayoutToChMask(layoutsList[layout], &noOfCh);
      wfxex.Format.nChannels = noOfCh;
      if (noOfCh < requestedChannels)
        continue;
    }

    for (int j = 0; j < sizeof(testFormats)/sizeof(sampleFormat); j++)
    {
      closestMatch = -1;

      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = testFormats[j].subFormat;
      wfxex.Format.wBitsPerSample       = testFormats[j].bitsPerSample;
      wfxex.Samples.wValidBitsPerSample = testFormats[j].validBitsPerSample;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);

      for (int i = 0 ; i < WASAPISampleRateCount; i++)
      {
        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[i];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

        hr = m_xAudio2->CreateMasteringVoice(&m_masterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                             0, device.c_str(), nullptr, AudioCategory_Media);
        if (SUCCEEDED(hr))
        {
          hr = m_xAudio2->CreateSourceVoice(&m_sourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceCallback, nullptr, nullptr);
          if (SUCCEEDED(hr))
          {
            /* If the current sample rate matches the source then stop looking and use it */
            if ((WASAPISampleRates[i] == format.m_sampleRate) && (testFormats[j].subFormatType <= format.m_dataFormat))
              goto initialize;
            /* If this rate is closer to the source then the previous one, save it */
            else if (closestMatch < 0 || abs((int)WASAPISampleRates[i] - (int)format.m_sampleRate) < abs((int)WASAPISampleRates[closestMatch] - (int)format.m_sampleRate))
              closestMatch = i;
          }
        }

        if (FAILED(hr))
          CLog::Log(LOGERROR, __FUNCTION__": creating voices failed (%s)", WASAPIErrToStr(hr));
      }

      if (closestMatch >= 0)
      {
        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[closestMatch];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        goto initialize;
      }
    }
  }

  CLog::Log(LOGERROR, __FUNCTION__": Unable to locate a supported output format for the device.  Check the speaker settings in the control panel.");

  /* We couldn't find anything supported. This should never happen      */
  /* unless the user set the wrong speaker setting in the control panel */
  return false;

initialize:

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);
  format.m_channelLayout = m_channelLayout;

  /* When the stream is raw, the values in the format structure are set to the link    */
  /* parameters, so store the encoded stream values here for the IsCompatible function */
  m_encodedChannels   = wfxex.Format.nChannels;
  m_encodedSampleRate = (format.m_dataFormat == AE_FMT_RAW) ? format.m_streamInfo.m_sampleRate : format.m_sampleRate;

  /* Set up returned sink format for engine */
  if (format.m_dataFormat != AE_FMT_RAW)
  {
    if (wfxex.Format.wBitsPerSample == 32)
    {
      if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        format.m_dataFormat = AE_FMT_FLOAT;
      else if (wfxex.Samples.wValidBitsPerSample == 32)
        format.m_dataFormat = AE_FMT_S32NE;
      else
        format.m_dataFormat = AE_FMT_S24NE4MSB;
    }
    else if (wfxex.Format.wBitsPerSample == 24)
      format.m_dataFormat = AE_FMT_S24NE3;
    else
      format.m_dataFormat = AE_FMT_S16NE;
  }

  format.m_sampleRate    = wfxex.Format.nSamplesPerSec; //PCM: Sample rate.  RAW: Link speed
  format.m_frameSize     = (wfxex.Format.wBitsPerSample >> 3) * wfxex.Format.nChannels;

  if (format.m_dataFormat == AE_FMT_RAW)
    format.m_dataFormat = AE_FMT_S16NE;

  hr = m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Voice start failed : %s", WASAPIErrToStr(hr));
    CLog::Log(LOGDEBUG, "  Sample Rate     : %d", wfxex.Format.nSamplesPerSec);
    CLog::Log(LOGDEBUG, "  Sample Format   : %s", CAEUtil::DataFormatToStr(format.m_dataFormat));
    CLog::Log(LOGDEBUG, "  Bits Per Sample : %d", wfxex.Format.wBitsPerSample);
    CLog::Log(LOGDEBUG, "  Valid Bits/Samp : %d", wfxex.Samples.wValidBitsPerSample);
    CLog::Log(LOGDEBUG, "  Channel Count   : %d", wfxex.Format.nChannels);
    CLog::Log(LOGDEBUG, "  Block Align     : %d", wfxex.Format.nBlockAlign);
    CLog::Log(LOGDEBUG, "  Avg. Bytes Sec  : %d", wfxex.Format.nAvgBytesPerSec);
    CLog::Log(LOGDEBUG, "  Samples/Block   : %d", wfxex.Samples.wSamplesPerBlock);
    CLog::Log(LOGDEBUG, "  Format cBSize   : %d", wfxex.Format.cbSize);
    CLog::Log(LOGDEBUG, "  Channel Layout  : %s", ((std::string)format.m_channelLayout).c_str());
    CLog::Log(LOGDEBUG, "  Channel Mask    : %d", wfxex.dwChannelMask);
    return false;
  }

  XAUDIO2_PERFORMANCE_DATA perfData;
  m_xAudio2->GetPerformanceData(&perfData);
  if (!perfData.TotalSourceVoiceCount)
  {
    CLog::Log(LOGERROR, __FUNCTION__": GetPerformanceData Failed : %s", WASAPIErrToStr(hr));
    return false;
  }

  m_uiBufferLen = (int)(format.m_sampleRate * 0.015);
  m_dwFrameSize = wfxex.Format.nBlockAlign;
  m_dwChunkSize = m_dwFrameSize * m_uiBufferLen;
  m_dwBufferLen = m_dwChunkSize * 4; 
  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  CLog::Log(LOGINFO, __FUNCTION__": XAudio Sink Initialized using: %s, %d, %d",
                                     CAEUtil::DataFormatToStr(format.m_dataFormat),
                                     wfxex.Format.nSamplesPerSec,
                                     wfxex.Format.nChannels);

  m_sourceVoice->Stop();

  return true;
}

void CAESinkXAudioWin10::AEChannelsFromSpeakerMask(DWORD speakers)
{
  m_channelLayout.Reset();

  for (int i = 0; i < WASAPI_SPEAKER_COUNT; i++)
  {
    if (speakers & WASAPIChannelOrder[i])
      m_channelLayout += AEChannelNames[i];
  }
}

DWORD CAESinkXAudioWin10::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
{
  DWORD mask = 0;

  for (unsigned int i = 0; i < channels.Count(); i++)
  {
    for (unsigned int j = 0; j < WASAPI_SPEAKER_COUNT; j++)
      if (channels[i] == AEChannelNames[j])
        mask |= WASAPIChannelOrder[j];
  }
  return mask;
}

const char *CAESinkXAudioWin10::WASAPIErrToStr(HRESULT err)
{
  switch(err)
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
  return nullptr;
}

void CAESinkXAudioWin10::Drain()
{
  if(!m_sourceVoice)
    return;

  AEDelayStatus status;
  GetDelay(status);

  Sleep((DWORD)(status.GetDelay() * 500));

  if (m_running)
  {
    try
    {
      m_sourceVoice->Stop();
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "%s: Invalidated AudioClient - Releasing", __FUNCTION__);
    }
  }
  m_running = false;
}

bool CAESinkXAudioWin10::IsUSBDevice()
{
#if 0 // TODO 
  IPropertyStore *pProperty = nullptr;
  PROPVARIANT varName;
  PropVariantInit(&varName);
  bool ret = false;

  HRESULT hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
  if (!SUCCEEDED(hr))
    return ret;
  hr = pProperty->GetValue(PKEY_Device_EnumeratorName, &varName);

  std::string str = localWideToUtf(varName.pwszVal);
  StringUtils::ToUpper(str);
  ret = (str == "USB");
  PropVariantClear(&varName);
  SAFE_RELEASE(pProperty);
#endif
  return false;
}
