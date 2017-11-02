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

#include "AESinkWASAPIWin10.h"
#include <Audioclient.h>
#include <stdint.h>
#include <algorithm>

#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "platform/win10/AsyncHelpers.h"
#include <Mmreg.h>
#include "utils/StringUtils.h"
#include <collection.h>
#include <ksmedia.h>
#include <wrl/implements.h>
#include <mmreg.h>
#include <mfapi.h>
#include <mmreg.h>
#include <mfapi.h>
#include <Mmdeviceapi.h>
#include <ppltasks.h>

// TARGET_WINDOWS_STORE version will use Windows::Medio::Audio
// https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/audio-graphs

using namespace Windows::Media::Devices;
using namespace Windows::Media::Devices::Core;
using namespace Windows::Devices::Enumeration;
using namespace Microsoft::WRL;

static Platform::String^ PKEY_Device_FriendlyName = "System.ItemNameDisplay";
static Platform::String^ PKEY_AudioEndpoint_FormFactor = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 0";
static Platform::String^ PKEY_AudioEndpoint_ControlPanelPageProvider = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 1";
static Platform::String^ PKEY_AudioEndpoint_Association = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 2";
static Platform::String^ PKEY_AudioEndpoint_PhysicalSpeakers = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 3";
static Platform::String^ PKEY_AudioEndpoint_GUID = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 4";
static Platform::String^ PKEY_AudioEndpoint_Disable_SysFx = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 5";
static Platform::String^ PKEY_AudioEndpoint_FullRangeSpeakers = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 6";
static Platform::String^ PKEY_AudioEndpoint_Supports_EventDriven_Mode = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 7";
static Platform::String^ PKEY_AudioEndpoint_JackSubType = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 8";
static Platform::String^ PKEY_AudioEndpoint_Default_VolumeInDb = "{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 9";
static Platform::String^ PKEY_AudioEngine_DeviceFormat = "{f19f064d-082c-4e27-bc73-6882a1bb8e4c} 0";

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

const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioClock = __uuidof(IAudioClock);

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

AEDeviceInfoList DeviceInfoList;

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason " - %s", __VA_ARGS__, WASAPIErrToStr(hr)); goto failed;}

#define ERRTOSTR(err) case err: return #err

DWORD ChLayoutToChMask(const enum AEChannel * layout, unsigned int * numberOfChannels = nullptr)
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
}

std::string localWideToUtf(LPCWSTR wstr)
{
  if (wstr == nullptr)
    return "";
  int bufSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
  char *multiStr = new char[bufSize + 1];
  if (bufSize == 0 || WideCharToMultiByte(CP_UTF8, 0, wstr, -1, multiStr, bufSize, nullptr, nullptr) != bufSize)
    multiStr[0] = 0;
  else
    multiStr[bufSize] = 0;
  std::string ret(multiStr);
  delete[] multiStr;
  return ret;
}

std::wstring localUtfToWide(const std::string &text)
{
  if (text.empty())
    return L"";

  int bufSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), -1, nullptr, 0);
  if (bufSize == 0)
    return L"";
  wchar_t *converted = new wchar_t[bufSize];
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), -1, converted, bufSize) != bufSize)
  {
    delete[] converted;
    return L"";
  }

  std::wstring Wret(converted);
  delete[] converted;
  return Wret;
}

///  ----------------- CAudioInterfaceActivator ------------------------

HRESULT CAudioInterfaceActivator::ActivateCompleted(IActivateAudioInterfaceAsyncOperation  *pAsyncOp)
{
  HRESULT hr = S_OK, hr2 = S_OK;
  ComPtr<IUnknown> clientUnk;
  IAudioClient* audioClient2;

  // Get the audio activation result as IUnknown pointer
  hr2 = pAsyncOp->GetActivateResult(&hr, &clientUnk);

  // Report activation failure
  if (FAILED(hr))
  {
    m_ActivateCompleted.set_exception(ref new Platform::COMException(hr));
    goto exit;
  }

  // Report failure to get activate result
  if (FAILED(hr2))
  {
    m_ActivateCompleted.set_exception(ref new Platform::COMException(hr));
    goto exit;
  }

  // Query for the activated IAudioClient2 interface
  hr = clientUnk->QueryInterface(IID_PPV_ARGS(&audioClient2));

  if (FAILED(hr))
  {
    m_ActivateCompleted.set_exception(ref new Platform::COMException(hr));
    goto exit;
  }

  // Set the completed event and return success
  m_ActivateCompleted.set(audioClient2);

exit:
  return hr;
}

Concurrency::task<IAudioClient*> CAudioInterfaceActivator::ActivateAsync(LPCWCHAR deviceId)
{
  ComPtr<CAudioInterfaceActivator> activator = Make<CAudioInterfaceActivator>();
  ComPtr<IActivateAudioInterfaceAsyncOperation> asyncOp;

  HRESULT hr = ActivateAudioInterfaceAsync(
    deviceId,
    __uuidof(IAudioClient2),
    nullptr,
    activator.Get(),
    &asyncOp);

  if (FAILED(hr))
    throw ref new Platform::COMException(hr);

  // Wait for the activate completed event and return result
  return Concurrency::create_task(activator->m_ActivateCompleted);

}

///  ----------------- CAESinkWASAPIWin10 ------------------------

CAESinkWASAPIWin10::CAESinkWASAPIWin10() :
  m_needDataEvent(0),
  m_pAudioClient(nullptr),
  m_pRenderClient(nullptr),
  m_pAudioClock(nullptr),
  m_encodedChannels(0),
  m_encodedSampleRate(0),
  sinkReqFormat(AE_FMT_INVALID),
  sinkRetFormat(AE_FMT_INVALID),
  m_running(false),
  m_initialized(false),
  m_isSuspended(false),
  m_isDirty(false),
  m_uiBufferLen(0),
  m_avgTimeWaiting(50),
  m_sinkLatency(0.0),
  m_sinkFrames(0),
  m_clockFreq(0),
  m_pBuffer(nullptr),
  m_bufferPtr(0)
{
  m_channelLayout.Reset();
}

CAESinkWASAPIWin10::~CAESinkWASAPIWin10()
{
}

bool CAESinkWASAPIWin10::Initialize(AEAudioFormat &format, std::string &device)
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

  if (StringUtils::EndsWithNoCase(device, std::string("default")))
    bdefault = true;

  IAudioClient* pClient = nullptr;
  if (!bdefault)
  {
    try
    {
      pClient = Wait(CAudioInterfaceActivator::ActivateAsync(localUtfToWide(device).c_str()));
    }
    catch(...) { }
  }

  if (!pClient)
  {
    if (!bdefault)
      CLog::Log(LOGINFO, __FUNCTION__": Could not locate the device named \"%s\" in the list of WASAPI endpoint devices.  Trying the default device...", device.c_str());
    Platform::String^ deviceId = MediaDevice::GetDefaultAudioRenderId(Windows::Media::Devices::AudioDeviceRole::Default);

    try
    {
      pClient = Wait(CAudioInterfaceActivator::ActivateAsync(deviceId->Data()));
    }
    catch (...) {}
    if (!pClient)
    {
      CLog::Log(LOGINFO, __FUNCTION__": Could not retrieve the default WASAPI audio endpoint.");
      goto failed;
    }

    device = localWideToUtf(deviceId->Data());
  }

  m_pAudioClient = pClient;

  if (!InitializeExclusive(format))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Could not Initialize Exclusive with that format");
    goto failed;
  }

  /* get the buffer size and calculate the frames for AE */
  m_pAudioClient->GetBufferSize(&m_uiBufferLen);

  format.m_frames       = m_uiBufferLen;
  m_format              = format;
  sinkRetFormat         = format.m_dataFormat;

  hr = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI render client interface.")

  hr = m_pAudioClient->GetService(IID_IAudioClock, (void**)&m_pAudioClock);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI audio clock interface.")

  hr = m_pAudioClock->GetFrequency(&m_clockFreq);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of IAudioClock::GetFrequency failed.")

  m_needDataEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  hr = m_pAudioClient->SetEventHandle(m_needDataEvent);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not set the WASAPI event handler.");

  m_initialized = true;
  m_isDirty     = false;

  // allow feeding less samples than buffer size
  // if the device is opened exclusive and event driven, provided samples must match buffersize
  // ActiveAE tries to align provided samples with buffer size but cannot guarantee (e.g. transcoding)
  // this can be avoided by dropping the event mode which has not much benefit; SoftAE polls anyway
  delete [] m_pBuffer;
  m_pBuffer = new uint8_t[format.m_frames * format.m_frameSize];
  m_bufferPtr = 0;

  return true;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": WASAPI initialization failed.");
  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pAudioClock);
  if(m_needDataEvent)
  {
    CloseHandle(m_needDataEvent);
    m_needDataEvent = 0;
  }

  return true;
}

void CAESinkWASAPIWin10::Deinitialize()
{
  if (!m_initialized && !m_isDirty)
    return;

  if (m_running)
  {
    try
    {
      m_pAudioClient->Stop();  //stop the audio output
      m_pAudioClient->Reset(); //flush buffer and reset audio clock stream position
      m_sinkFrames = 0;
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "%s: Invalidated AudioClient - Releasing", __FUNCTION__);
    }
  }
  m_running = false;

  CloseHandle(m_needDataEvent);

  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pAudioClock);

  m_initialized = false;

  delete [] m_pBuffer;
  m_bufferPtr = 0;
}

/**
 * @brief rescale uint64_t without overflowing on large values
 */
static uint64_t rescale_u64(uint64_t val, uint64_t num, uint64_t den)
{
  return ((val / den) * num) + (((val % den) * num) / den);
}


void CAESinkWASAPIWin10::GetDelay(AEDelayStatus& status)
{
  HRESULT hr;
  uint64_t pos, tick;
  int retries = 0;

  if (!m_initialized)
    goto failed;

  do {
    hr = m_pAudioClock->GetPosition(&pos, &tick);
  } while (hr != S_OK && ++retries < 100);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of IAudioClock::GetPosition failed.")

  status.delay = (double)(m_sinkFrames + m_bufferPtr) / m_format.m_sampleRate - (double)pos / m_clockFreq;
  status.tick  = rescale_u64(tick, CurrentHostFrequency(), 10000000); /* convert from 100ns back to qpc ticks */
  return;
failed:
  status.SetDelay(0);
}

double CAESinkWASAPIWin10::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  return m_sinkLatency;
}

unsigned int CAESinkWASAPIWin10::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized)
    return 0;

  HRESULT hr;
  BYTE *buf;
  DWORD flags = 0;

#ifndef _DEBUG
  LARGE_INTEGER timerStart;
  LARGE_INTEGER timerStop;
  LARGE_INTEGER timerFreq;
#endif

  unsigned int NumFramesRequested = m_format.m_frames;
  unsigned int FramesToCopy = std::min(m_format.m_frames - m_bufferPtr, frames);
  uint8_t *buffer = data[0]+offset*m_format.m_frameSize;
  if (m_bufferPtr != 0 || frames != m_format.m_frames)
  {
    memcpy(m_pBuffer+m_bufferPtr*m_format.m_frameSize, buffer, FramesToCopy*m_format.m_frameSize);
    m_bufferPtr += FramesToCopy;
    if (m_bufferPtr != m_format.m_frames)
      return frames;
  }

  if (!m_running) //first time called, pre-fill buffer then start audio client
  {
    hr = m_pAudioClient->Reset();
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__ " AudioClient reset failed due to %s", WASAPIErrToStr(hr));
      return 0;
    }
    hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
    if (FAILED(hr))
    {
      #ifdef _DEBUG
      CLog::Log(LOGERROR, __FUNCTION__": GetBuffer failed due to %s", WASAPIErrToStr(hr));
      #endif
      m_isDirty = true; //flag new device or re-init needed
      return INT_MAX;
    }

    memset(buf, 0, NumFramesRequested * m_format.m_frameSize); //fill buffer with silence

    hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
    if (FAILED(hr))
    {
      #ifdef _DEBUG
      CLog::Log(LOGDEBUG, __FUNCTION__": ReleaseBuffer failed due to %s.", WASAPIErrToStr(hr));
      #endif
      m_isDirty = true; //flag new device or re-init needed
      return INT_MAX;
    }
    m_sinkFrames += NumFramesRequested;

    hr = m_pAudioClient->Start(); //start the audio driver running
    if (FAILED(hr))
      CLog::Log(LOGERROR, __FUNCTION__": AudioClient Start Failed");
    m_running = true; //signal that we're processing frames
    return 0U;
  }

#ifndef _DEBUG
  /* Get clock time for latency checks */
  QueryPerformanceFrequency(&timerFreq);
  QueryPerformanceCounter(&timerStart);
#endif

  /* Wait for Audio Driver to tell us it's got a buffer available */
  DWORD eventAudioCallback;
  eventAudioCallback = WaitForSingleObject(m_needDataEvent, 1100);

  if(eventAudioCallback != WAIT_OBJECT_0 || !&buf)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Endpoint Buffer timed out");
    return INT_MAX;
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

  hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
  if (FAILED(hr))
  {
    #ifdef _DEBUG
      CLog::Log(LOGERROR, __FUNCTION__": GetBuffer failed due to %s", WASAPIErrToStr(hr));
    #endif
    return INT_MAX;
  }
  memcpy(buf, m_bufferPtr == 0 ? buffer : m_pBuffer, NumFramesRequested * m_format.m_frameSize); //fill buffer
  m_bufferPtr = 0;
  hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
  if (FAILED(hr))
  {
    #ifdef _DEBUG
    CLog::Log(LOGDEBUG, __FUNCTION__": ReleaseBuffer failed due to %s.", WASAPIErrToStr(hr));
    #endif
    return INT_MAX;
  }
  m_sinkFrames += NumFramesRequested;

  if (FramesToCopy != frames)
  {
    m_bufferPtr = frames-FramesToCopy;
    memcpy(m_pBuffer, buffer+FramesToCopy*m_format.m_frameSize, m_bufferPtr*m_format.m_frameSize);
  }

  return frames;
}

void CAESinkWASAPIWin10::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  HRESULT hr = S_OK;
  CAEDeviceInfo deviceInfo;
  CAEChannelInfo deviceChannels;
  WAVEFORMATEXTENSIBLE wfxex = { 0 };
  bool add192 = false;

  // Get the string identifier of the audio renderer
  Platform::String^ defaultId = MediaDevice::GetDefaultAudioRenderId(Windows::Media::Devices::AudioDeviceRole::Default);
  Platform::String^ audioSelector = MediaDevice::GetAudioRenderSelector();

  // Add custom properties to the query
  auto propertyList = ref new Platform::Collections::Vector<Platform::String^>();
  propertyList->Append(PKEY_AudioEndpoint_FormFactor);
  propertyList->Append(PKEY_AudioEndpoint_GUID);
  propertyList->Append(PKEY_AudioEndpoint_PhysicalSpeakers);

  DeviceInformationCollection^ devInfocollection = Wait(DeviceInformation::FindAllAsync(audioSelector, propertyList));
  try
  {
    if (devInfocollection == nullptr || devInfocollection->Size == 0)
      goto failed;

    for (unsigned int i = 0; i < devInfocollection->Size; i++)
    {
      deviceInfo.m_channels.Reset();
      deviceInfo.m_dataFormats.clear();
      deviceInfo.m_sampleRates.clear();

      DeviceInformation^ devInfo = devInfocollection->GetAt(i);
      std::string strFriendlyName = localWideToUtf(devInfo->Name->Data());

      if (devInfo->Properties->Size == 0)
        goto failed;

      Platform::Object^ propObj;

      // not needed, using devInfo->Id instead
      //propObj = devInfo->Properties->Lookup(PKEY_AudioEndpoint_GUID);
      //if (nullptr == propObj)
      //  goto failed;
      //Platform::String^ deviceId = propObj->ToString();

      std::string strDevName = localWideToUtf(devInfo->Id->Data());

      propObj = devInfo->Properties->Lookup(PKEY_AudioEndpoint_FormFactor);
      if (nullptr == propObj)
        goto failed;

      std::string strWinDevType = winEndpoints[safe_cast<uint32>(propObj)].winEndpointType;
      AEDeviceType aeDeviceType = winEndpoints[safe_cast<uint32>(propObj)].aeDeviceType;

      // TODO unable to receive this prop
      propObj = devInfo->Properties->Lookup(PKEY_AudioEndpoint_PhysicalSpeakers);
      //if (nullptr == propObj)
      //  goto failed;

      unsigned int uiChannelMask = std::max(0u/*safe_cast<unsigned int>(propObj)*/, (unsigned int)(SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT));
      deviceChannels.Reset();

      for (unsigned int c = 0; c < WASAPI_SPEAKER_COUNT; c++)
      {
        if (uiChannelMask & WASAPIChannelOrder[c])
          deviceChannels += AEChannelNames[c];
      }

      IAudioClient *pClient = Wait(CAudioInterfaceActivator::ActivateAsync(devInfo->Id->Data()));
      if (pClient)
      {
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
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
        if (SUCCEEDED(hr) || aeDeviceType == AE_DEVTYPE_HDMI)
        {
          if (FAILED(hr))
            CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_DTSHD), strFriendlyName.c_str());

          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
          add192 = true;
        }

        /* Test format Dolby TrueHD */
        wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
        if (SUCCEEDED(hr) || aeDeviceType == AE_DEVTYPE_HDMI)
        {
          if (FAILED(hr))
            CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_TRUEHD), strFriendlyName.c_str());

          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
          add192 = true;
        }

        /* Test format Dolby EAC3 */
        wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
        wfxex.Format.nChannels = 2;
        wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
        if (SUCCEEDED(hr) || aeDeviceType == AE_DEVTYPE_HDMI)
        {
          if (FAILED(hr))
            CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_EAC3), strFriendlyName.c_str());

          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
          add192 = true;
        }

        /* Test format DTS */
        wfxex.Format.nSamplesPerSec = 48000;
        wfxex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
        wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
        wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
        if (SUCCEEDED(hr) || aeDeviceType == AE_DEVTYPE_HDMI)
        {
          if (FAILED(hr))
            CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", "STREAM_TYPE_DTS", strFriendlyName.c_str());

          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
          deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
        }

        /* Test format Dolby AC3 */
        wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
        if (SUCCEEDED(hr) || aeDeviceType == AE_DEVTYPE_HDMI)
        {
          if (FAILED(hr))
            CLog::Log(LOGNOTICE, __FUNCTION__": stream type \"%s\" on device \"%s\" seems to be not supported.", CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_AC3), strFriendlyName.c_str());

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

          hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
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
          wfxex.Format.nSamplesPerSec = WASAPISampleRates[j];
          wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
          hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);
          if (SUCCEEDED(hr))
            deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
          else if (wfxex.Format.nSamplesPerSec == 192000 && add192)
          {
            deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
            CLog::Log(LOGNOTICE, __FUNCTION__": sample rate 192khz on device \"%s\" seems to be not supported.", strFriendlyName.c_str());
          }
        }
        pClient->Release();
      }
      else
      {
        CLog::Log(LOGDEBUG, __FUNCTION__": Failed to activate device for passthrough capability testing.");
      }

      deviceInfo.m_deviceName = strDevName;
      deviceInfo.m_displayName = strWinDevType.append(strFriendlyName);
      deviceInfo.m_displayNameExtra = std::string("WASAPI: ").append(strFriendlyName);
      deviceInfo.m_deviceType = aeDeviceType;
      deviceInfo.m_channels = deviceChannels;

      /* Store the device info */
      deviceInfo.m_wantsIECPassthrough = true;

      if (!deviceInfo.m_streamTypes.empty())
        deviceInfo.m_dataFormats.push_back(AE_FMT_RAW);

      deviceInfoList.push_back(deviceInfo);

      if (devInfo->Id->Equals(defaultId))
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

void CAESinkWASAPIWin10::BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex)
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

void CAESinkWASAPIWin10::BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex)
{
  /* Fill the common structure */
  BuildWaveFormatExtensible(format, wfxex.FormatExt);

  /* Code below kept for future use - preferred for later Windows versions */
  /* but can cause problems on older Windows versions and drivers          */
  /*
  wfxex.FormatExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE_IEC61937)-sizeof(WAVEFORMATEX);
  wfxex.dwEncodedChannelCount   = format.m_channelLayout.Count();
  wfxex.dwEncodedSamplesPerSec  = bool(format.m_dataFormat == AE_FMT_TRUEHD ||
  format.m_dataFormat == AE_FMT_DTSHD  ||
  format.m_dataFormat == AE_FMT_EAC3) ? 96000L : 48000L;
  wfxex.dwAverageBytesPerSec    = 0; //Ignored */
}

bool CAESinkWASAPIWin10::InitializeExclusive(AEAudioFormat &format)
{
  WAVEFORMATEXTENSIBLE_IEC61937 wfxex_iec61937;
  WAVEFORMATEXTENSIBLE &wfxex = wfxex_iec61937.FormatExt;

  if (format.m_dataFormat <= AE_FMT_FLOAT)
    BuildWaveFormatExtensible(format, wfxex);
  else if (format.m_dataFormat == AE_FMT_RAW)
    BuildWaveFormatExtensibleIEC61397(format, wfxex_iec61937);
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

  HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);

  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Format is Supported - will attempt to Initialize");
    goto initialize;
  }
  else if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT) //It failed for a reason unrelated to an unsupported format.
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

        /* Trace format match iteration loop via log */
#if 0
        CLog::Log(LOGDEBUG, "WASAPI: Trying Format: %s, %d, %d, %d", CAEUtil::DataFormatToStr(testFormats[j].subFormatType),
          wfxex.Format.nSamplesPerSec,
          wfxex.Format.wBitsPerSample,
          wfxex.Samples.wValidBitsPerSample);
#endif

        hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, nullptr);

        if (SUCCEEDED(hr))
        {
          /* If the current sample rate matches the source then stop looking and use it */
          if ((WASAPISampleRates[i] == format.m_sampleRate) && (testFormats[j].subFormatType <= format.m_dataFormat))
            goto initialize;
          /* If this rate is closer to the source then the previous one, save it */
          else if (closestMatch < 0 || abs((int)WASAPISampleRates[i] - (int)format.m_sampleRate) < abs((int)WASAPISampleRates[closestMatch] - (int)format.m_sampleRate))
            closestMatch = i;
        }
        else if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT)
          CLog::Log(LOGERROR, __FUNCTION__": IsFormatSupported failed (%s)", WASAPIErrToStr(hr));
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
  wfxex_iec61937.dwEncodedChannelCount = wfxex.Format.nChannels;
  wfxex_iec61937.dwEncodedSamplesPerSec = m_encodedSampleRate;

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

  REFERENCE_TIME audioSinkBufferDurationMsec, hnsLatency;

  audioSinkBufferDurationMsec = (REFERENCE_TIME)500000;
  if (IsUSBDevice())
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": detected USB device, increasing buffer size");
    audioSinkBufferDurationMsec = (REFERENCE_TIME)1000000;
  }
  audioSinkBufferDurationMsec = (REFERENCE_TIME)((audioSinkBufferDurationMsec / format.m_frameSize) * format.m_frameSize); //even number of frames

  if (format.m_dataFormat == AE_FMT_RAW)
    format.m_dataFormat = AE_FMT_S16NE;

  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                    audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, nullptr);

  if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
  {
    /* WASAPI requires aligned buffer */
    /* Get the next aligned frame     */
    hr = m_pAudioClient->GetBufferSize(&m_uiBufferLen);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": GetBufferSize Failed : %s", WASAPIErrToStr(hr));
      return false;
    }

    audioSinkBufferDurationMsec = (REFERENCE_TIME) ((10000.0 * 1000 / wfxex.Format.nSamplesPerSec * m_uiBufferLen) + 0.5);

    /* Release the previous allocations */
    SAFE_RELEASE(m_pAudioClient);

    /* Create a new audio client */
    auto deviceId = MediaDevice::GetDefaultAudioRenderId(Windows::Media::Devices::AudioDeviceRole::Default);
    m_pAudioClient = Wait(CAudioInterfaceActivator::ActivateAsync(deviceId->Data()));

    /* Open the stream and associate it with an audio session */
    hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                      audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, nullptr);
  }
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to initialize WASAPI in exclusive mode %d - (%s).", HRESULT(hr), WASAPIErrToStr(hr));
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
    CLog::Log(LOGDEBUG, "  Enc. Channels   : %d", wfxex_iec61937.dwEncodedChannelCount);
    CLog::Log(LOGDEBUG, "  Enc. Samples/Sec: %d", wfxex_iec61937.dwEncodedSamplesPerSec);
    CLog::Log(LOGDEBUG, "  Channel Mask    : %d", wfxex.dwChannelMask);
    CLog::Log(LOGDEBUG, "  Periodicty      : %I64d", audioSinkBufferDurationMsec);
    return false;
  }

  /* Latency of WASAPI buffers in event-driven mode is equal to the returned value  */
  /* of GetStreamLatency converted from 100ns intervals to seconds then multiplied  */
  /* by two as there are two equally-sized buffers and playback starts when the     */
  /* second buffer is filled. Multiplying the returned 100ns intervals by 0.0000002 */
  /* is handles both the unit conversion and twin buffers.                          */
  hr = m_pAudioClient->GetStreamLatency(&hnsLatency);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": GetStreamLatency Failed : %s", WASAPIErrToStr(hr));
    return false;
  }

  m_sinkLatency = hnsLatency * 0.0000002;

  CLog::Log(LOGINFO, __FUNCTION__": WASAPI Exclusive Mode Sink Initialized using: %s, %d, %d",
                                     CAEUtil::DataFormatToStr(format.m_dataFormat),
                                     wfxex.Format.nSamplesPerSec,
                                     wfxex.Format.nChannels);

  return true;
}

void CAESinkWASAPIWin10::AEChannelsFromSpeakerMask(DWORD speakers)
{
  m_channelLayout.Reset();

  for (int i = 0; i < WASAPI_SPEAKER_COUNT; i++)
  {
    if (speakers & WASAPIChannelOrder[i])
      m_channelLayout += AEChannelNames[i];
  }
}

DWORD CAESinkWASAPIWin10::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
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

const char *CAESinkWASAPIWin10::WASAPIErrToStr(HRESULT err)
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

void CAESinkWASAPIWin10::Drain()
{
  if(!m_pAudioClient)
    return;

  AEDelayStatus status;
  GetDelay(status);

  Sleep((DWORD)(status.GetDelay() * 500));

  if (m_running)
  {
    try
    {
      m_pAudioClient->Stop();  //stop the audio output
      m_pAudioClient->Reset(); //flush buffer and reset audio clock stream position
      m_sinkFrames = 0;
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "%s: Invalidated AudioClient - Releasing", __FUNCTION__);
    }
  }
  m_running = false;
}

bool CAESinkWASAPIWin10::IsUSBDevice()
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
