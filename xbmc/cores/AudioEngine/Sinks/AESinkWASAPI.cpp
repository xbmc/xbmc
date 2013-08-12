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

#include "AESinkWASAPI.h"
#include <Audioclient.h>
#include <avrt.h>
#include <initguid.h>
#include <Mmreg.h>
#include <stdint.h>

#include "../Utils/AEUtil.h"
#include "settings/AdvancedSettings.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "../Utils/AEDeviceInfo.h"
#include <Mmreg.h>
#include <mmdeviceapi.h>
#include "utils/StringUtils.h"

#pragma comment(lib, "Avrt.lib")

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

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
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_SL,  AE_CH_SR,  AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // Standard 7.1
  /* Less common configurations */
  {AE_CH_FL,  AE_CH_FR,  AE_CH_LFE, AE_CH_NULL}, // 2.1
  {AE_CH_FL,  AE_CH_FR,  AE_CH_FC,  AE_CH_LFE, AE_CH_BL,  AE_CH_BR,  AE_CH_NULL}, // 5.1 wide (obsolete)
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

/* Sample formats go from float -> 32 bit int -> 24 bit int (packed in 32) -> -> 24 bit int -> 16 bit int */
static const sampleFormat testFormats[] = { {KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 32, 32, AE_FMT_FLOAT},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 32, 32, AE_FMT_S32NE},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 32, 24, AE_FMT_S24NE4},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 24, 24, AE_FMT_S24NE3},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 16, 16, AE_FMT_S16NE} };

struct winEndpointsToAEDeviceType
{
  std::string winEndpointType;
  AEDeviceType aeDeviceType;
};

static const winEndpointsToAEDeviceType winEndpoints[EndpointFormFactor_enum_count] =
{
  {"Network Device - ",         AE_DEVTYPE_PCM},
  {"Speakers - ",               AE_DEVTYPE_PCM},
  {"LineLevel - ",              AE_DEVTYPE_PCM},
  {"Headphones - ",             AE_DEVTYPE_PCM},
  {"Microphone - ",             AE_DEVTYPE_PCM},
  {"Headset - ",                AE_DEVTYPE_PCM},
  {"Handset - ",                AE_DEVTYPE_PCM},
  {"Digital Passthrough - ", AE_DEVTYPE_IEC958},
  {"SPDIF - ",               AE_DEVTYPE_IEC958},
  {"HDMI - ",                  AE_DEVTYPE_HDMI},
  {"Unknown - ",                AE_DEVTYPE_PCM},
};

AEDeviceInfoList DeviceInfoList;

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason " - %s", __VA_ARGS__, WASAPIErrToStr(hr)); goto failed;}

#define ERRTOSTR(err) case err: return #err

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);

DWORD ChLayoutToChMask(const enum AEChannel * layout, unsigned int * numberOfChannels = NULL)
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

CStdStringA localWideToUtf(LPCWSTR wstr)
{
  if (wstr == NULL)
    return "";
  int bufSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  CStdStringA strA ("", bufSize);
  if ( bufSize == 0 || WideCharToMultiByte(CP_UTF8, 0, wstr, -1, strA.GetBuf(bufSize), bufSize, NULL, NULL) != bufSize )
    strA.clear();
  strA.RelBuf();
  return strA;
}

CAESinkWASAPI::CAESinkWASAPI() :
  m_needDataEvent(0),
  m_pDevice(NULL),
  m_pAudioClient(NULL),
  m_pRenderClient(NULL),
  m_encodedFormat(AE_FMT_INVALID),
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
  m_pBuffer(NULL),
  m_bufferPtr(0),
  m_hnsRequestedDuration(0)
{
  m_channelLayout.Reset();
}

CAESinkWASAPI::~CAESinkWASAPI()
{

}

bool CAESinkWASAPI::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  m_device = device;
  bool bdefault = false;

  /* Save requested format */
  /* Clear returned format */
  sinkReqFormat = format.m_dataFormat;
  sinkRetFormat = AE_FMT_INVALID;

  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)

  /* Get our device. First try to find the named device. */
  UINT uiCount = 0;

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  if(StringUtils::EndsWith(device, std::string("default")))
    bdefault = true;

  if(!bdefault)
  {
    for (UINT i = 0; i < uiCount; i++)
    {
      IPropertyStore *pProperty = NULL;
      PROPVARIANT varName;

      hr = pEnumDevices->Item(i, &m_pDevice);
      EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint failed.")

      hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
      EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.")

      hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
      if (FAILED(hr))
      {
        CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint GUID failed.");
        SAFE_RELEASE(pProperty);
        goto failed;
      }

      std::string strDevName = localWideToUtf(varName.pwszVal);

      if (device == strDevName)
        i = uiCount;
      else
        SAFE_RELEASE(m_pDevice);

      PropVariantClear(&varName);
      SAFE_RELEASE(pProperty);
    }
  }
  SAFE_RELEASE(pEnumDevices);

  if (!m_pDevice)
  {
    if(!bdefault)
      CLog::Log(LOGINFO, __FUNCTION__": Could not locate the device named \"%s\" in the list of WASAPI endpoint devices.  Trying the default device...", device.c_str());
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Could not retrieve the default WASAPI audio endpoint.")

    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;

    hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);

    device = localWideToUtf(varName.pwszVal);
    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty);
  }

  SAFE_RELEASE(pEnumerator);

  hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Activating the WASAPI endpoint device failed.")

  if (!InitializeExclusive(format))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Could not Initialize Exclusive with that format");
    goto failed;
  }

  /* get the buffer size and calculate the frames for AE */
  m_pAudioClient->GetBufferSize(&m_uiBufferLen);

  format.m_frames       = m_uiBufferLen;
  format.m_frameSamples = format.m_frames * format.m_channelLayout.Count();
  m_format              = format;
  sinkRetFormat         = format.m_dataFormat;

  hr = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI render client interface.")

  m_needDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pDevice);
  if(m_needDataEvent)
  {
    CloseHandle(m_needDataEvent);
    m_needDataEvent = 0;
  }

  return false;
}

void CAESinkWASAPI::Deinitialize()
{
  if (!m_initialized && !m_isDirty)
    return;

  if (m_running)
  {
    try
    {
    m_pAudioClient->Stop();  //stop the audio output
    m_pAudioClient->Reset(); //flush buffer and reset audio clock stream position
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__, "Invalidated AudioClient - Releasing");
    }
  }
  m_running = false;

  CloseHandle(m_needDataEvent);

  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pDevice);

  m_initialized = false;

  delete [] m_pBuffer;
  m_bufferPtr = 0;
}

bool CAESinkWASAPI::IsCompatible(const AEAudioFormat &format, const std::string &device)
{
  if (!m_initialized || m_isDirty)
    return false;

  u_int notCompatible         = 0;
  const u_int numTests        = 5;
  std::string strDiffBecause ("");
  static const char* compatibleParams[numTests] = {":Devices",
                                                   ":Channels",
                                                   ":Sample Rates",
                                                   ":Data Formats",
                                                   ":Passthrough Formats"};

  notCompatible = (notCompatible  +!((AE_IS_RAW(format.m_dataFormat)  == AE_IS_RAW(m_encodedFormat))        ||
                                     (!AE_IS_RAW(format.m_dataFormat) == !AE_IS_RAW(m_encodedFormat))))     << 1;
  notCompatible = (notCompatible  +!((sinkReqFormat                   == format.m_dataFormat)               &&
                                     (sinkRetFormat                   == m_format.m_dataFormat)))           << 1;
  notCompatible = (notCompatible  + !(format.m_sampleRate             == m_format.m_sampleRate))            << 1;
  notCompatible = (notCompatible  + !(format.m_channelLayout.Count()  == m_format.m_channelLayout.Count())) << 1;
  notCompatible = (notCompatible  + !(m_device                        == device));

  if (!notCompatible)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Formats compatible - reusing existing sink");
    return true;
  }

  for (int i = 0; i < numTests ; i++)
  {
    strDiffBecause += (notCompatible & 0x01) ? (std::string) compatibleParams[i] : "";
    notCompatible    = notCompatible >> 1;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__": Formats Incompatible due to different %s", strDiffBecause.c_str());
  return false;
}

double CAESinkWASAPI::GetDelay()
{
  if (!m_initialized)
    return 0.0;

  return m_sinkLatency;
}

double CAESinkWASAPI::GetCacheTime()
{
  /* This function deviates from the defined usage due to the event-driven */
  /* mode of WASAPI utilizing twin buffers which are written to in single  */
  /* buffer chunks. Therefore the buffers are either 100% full or 50% full */
  /* At 50% issues arise with water levels in the stream and player. For   */
  /* this reason the cache is shown as 100% full at all times, and control */
  /* of the buffer filling is assumed in AddPackets() and by the WASAPI    */
  /* implementation of the WaitforSingleObject event indicating one of the */
  /* buffers is ready for filling via AddPackets                           */
  if (!m_initialized)
    return 0.0;

  return m_sinkLatency;
}

double CAESinkWASAPI::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  return m_sinkLatency;
}

unsigned int CAESinkWASAPI::AddPackets(uint8_t *data, unsigned int frames, bool hasAudio, bool blocking)
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
  if (m_bufferPtr != 0 || frames != m_format.m_frames)
  {
    memcpy(m_pBuffer+m_bufferPtr*m_format.m_frameSize, data, FramesToCopy*m_format.m_frameSize);
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

    /* Inject one buffer of silence if sink has just opened */
    /* to avoid losing start of stream or GUI sound         */
    if (g_advancedSettings.m_streamSilence)
      memcpy(buf, data, NumFramesRequested * m_format.m_frameSize); //fill buffer with audio
    else
      memset(buf,    0, NumFramesRequested * m_format.m_frameSize); //fill buffer with silence

    hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
    if (FAILED(hr))
    {
      #ifdef _DEBUG
      CLog::Log(LOGDEBUG, __FUNCTION__": ReleaseBuffer failed due to %s.", WASAPIErrToStr(hr));
      #endif
      m_isDirty = true; //flag new device or re-init needed
      return INT_MAX;
    }
    hr = m_pAudioClient->Start(); //start the audio driver running
    if (FAILED(hr))
      CLog::Log(LOGERROR, __FUNCTION__": AudioClient Start Failed");
    m_running = true; //signal that we're processing frames
    return g_advancedSettings.m_streamSilence ? NumFramesRequested : 0U;
  }

#ifndef _DEBUG
  /* Get clock time for latency checks */
  QueryPerformanceFrequency(&timerFreq);
  QueryPerformanceCounter(&timerStart);
#endif

  /* Wait for Audio Driver to tell us it's got a buffer available */
  DWORD eventAudioCallback;
  if(!blocking)
    eventAudioCallback = WaitForSingleObject(m_needDataEvent, 0);
  else
    eventAudioCallback = WaitForSingleObject(m_needDataEvent, 1100);

  if (!blocking)
  {
    if(eventAudioCallback != WAIT_OBJECT_0)
	  return 0;
  }
  else
  {
    if(eventAudioCallback != WAIT_OBJECT_0 || !&buf)
    {
      /* Event handle timed out - flag sink as dirty for re-initializing */
      CLog::Log(LOGERROR, __FUNCTION__": Endpoint Buffer timed out");
      if (g_advancedSettings.m_streamSilence)
      {
        m_isDirty = true; //flag new device or re-init needed
        Deinitialize();
        m_running = false;
        return INT_MAX;
      }
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

  hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
  if (FAILED(hr))
  {
    #ifdef _DEBUG
      CLog::Log(LOGERROR, __FUNCTION__": GetBuffer failed due to %s", WASAPIErrToStr(hr));
    #endif
    return INT_MAX;
  }
  memcpy(buf, m_bufferPtr == 0 ? data : m_pBuffer, NumFramesRequested * m_format.m_frameSize); //fill buffer
  m_bufferPtr = 0;
  hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
  if (FAILED(hr))
  {
    #ifdef _DEBUG
    CLog::Log(LOGDEBUG, __FUNCTION__": ReleaseBuffer failed due to %s.", WASAPIErrToStr(hr));
    #endif
    return INT_MAX;
  }

  if (FramesToCopy != frames)
  {
    m_bufferPtr = frames-FramesToCopy;
    memcpy(m_pBuffer, data+FramesToCopy*m_format.m_frameSize, m_bufferPtr*m_format.m_frameSize);
  }

  return frames;
}

bool CAESinkWASAPI::SoftSuspend()
{
  /* Sink has been asked to suspend output - we release audio   */
  /* device as we are in exclusive mode and thus allow external */
  /* audio sources to play. This requires us to reinitialize    */
  /* on resume.                                                 */

  Deinitialize();

  return true;
}

bool CAESinkWASAPI::SoftResume()
{
  /* Sink asked to resume output. To release audio device in    */
  /* exclusive mode we release the device context and therefore */
  /* must reinitialize. Return false to force re-init by engine */

  return false;
}

void CAESinkWASAPI::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;
  IMMDevice*           pDefaultDevice = NULL;
  CAEDeviceInfo        deviceInfo;
  CAEChannelInfo       deviceChannels;
  LPWSTR               pwszID = NULL;
  std::wstring         wstrDDID;

  WAVEFORMATEXTENSIBLE wfxex = {0};
  HRESULT              hr;

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)

  UINT uiCount = 0;

  // get the default audio endpoint
  if(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice) == S_OK)
  {
    if(pDefaultDevice->GetId(&pwszID) == S_OK)
    {
      wstrDDID = pwszID;
      CoTaskMemFree(pwszID);
    }
    SAFE_RELEASE(pDefaultDevice);
  }

  // enumerate over all audio endpoints
  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    deviceInfo.m_channels.Reset();
    deviceInfo.m_dataFormats.clear();
    deviceInfo.m_sampleRates.clear();

    hr = pEnumDevices->Item(i, &pDevice);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint failed.");
      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.");
      SAFE_RELEASE(pDevice);
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint device name failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::string strFriendlyName = localWideToUtf(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint GUID failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::string strDevName = localWideToUtf(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint form factor failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }
    std::string strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    AEDeviceType aeDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;

    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint speaker layout failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }
    unsigned int uiChannelMask = std::max(varName.uintVal, (unsigned int) KSAUDIO_SPEAKER_STEREO);

    deviceChannels.Reset();

    for (unsigned int c = 0; c < WASAPI_SPEAKER_COUNT; c++)
    {
      if (uiChannelMask & WASAPIChannelOrder[c])
        deviceChannels += AEChannelNames[c];
    }

    PropVariantClear(&varName);

    IAudioClient *pClient;
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pClient);
    if (SUCCEEDED(hr))
    {
      /* Test format DTS-HD */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.Format.nSamplesPerSec       = 192000;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_7POINT1_SURROUND;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 8;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_DTSHD));

      /* Test format Dolby TrueHD */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_TRUEHD));

      /* Test format Dolby EAC3 */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_EAC3));

      /* Test format DTS */
      wfxex.Format.nSamplesPerSec       = 48000;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_5POINT1;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_DTS));

      /* Test format Dolby AC3 */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_AC3));

      /* Test format AAC */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_AAC;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_AAC));

      /* Test format for PCM format iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

      for (int p = AE_FMT_FLOAT; p > AE_FMT_INVALID; p--)
      {
        if (p < AE_FMT_FLOAT)
          wfxex.SubFormat               = KSDATAFORMAT_SUBTYPE_PCM;
        wfxex.Format.wBitsPerSample     = CAEUtil::DataFormatToBits((AEDataFormat) p);
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        if (p <= AE_FMT_S24NE4 && p >= AE_FMT_S24BE4)
        {
          wfxex.Samples.wValidBitsPerSample = 24;
        }
        else
        {
          wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
        }

        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
          deviceInfo.m_dataFormats.push_back((AEDataFormat) p);
      }

      /* Test format for sample rate iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      for (int j = 0; j < WASAPISampleRateCount; j++)
      {
        wfxex.Format.nSamplesPerSec     = WASAPISampleRates[j];
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
      }

      /* Test format for channels iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
      wfxex.Format.nSamplesPerSec       = 48000;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      bool hasLpcm = false;

      // Try with KSAUDIO_SPEAKER_DIRECTOUT
      for (unsigned int k = WASAPI_SPEAKER_COUNT; k > 0; k--)
      {
        wfxex.dwChannelMask             = KSAUDIO_SPEAKER_DIRECTOUT;
        wfxex.Format.nChannels          = k;
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
        {
          if (k > 3) // Add only multichannel LPCM
          {
            deviceInfo.m_dataFormats.push_back(AE_FMT_LPCM);
            hasLpcm = true;
          }
          break;
        }
      }

      /* Try with reported channel mask */
      for (unsigned int k = WASAPI_SPEAKER_COUNT; k > 0; k--)
      {
        wfxex.dwChannelMask             = uiChannelMask;
        wfxex.Format.nChannels          = k;
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
        {
          if ( !hasLpcm && k > 3) // Add only multichannel LPCM
          {
            deviceInfo.m_dataFormats.push_back(AE_FMT_LPCM);
            hasLpcm = true;
          }
          break;
        }
      }

      /* Try with specific speakers configurations */
      for (unsigned int i = 0; i < ARRAYSIZE(layoutsList); i++)
      {
        unsigned int nmbOfCh;
        wfxex.dwChannelMask             = ChLayoutToChMask(layoutsList[i], &nmbOfCh);
        wfxex.Format.nChannels          = nmbOfCh;
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
        {
          if ( deviceChannels.Count() < nmbOfCh)
            deviceChannels = layoutsList[i];
          if ( !hasLpcm && nmbOfCh > 3) // Add only multichannel LPCM
          {
            deviceInfo.m_dataFormats.push_back(AE_FMT_LPCM);
            hasLpcm = true;
          }
        }
      }
      pClient->Release();
    }
    else
    {
      CLog::Log(LOGDEBUG, __FUNCTION__": Failed to activate device for passthrough capability testing.");
    }

    deviceInfo.m_deviceName       = strDevName;
    deviceInfo.m_displayName      = strWinDevType.append(strFriendlyName);
    deviceInfo.m_displayNameExtra = std::string("WASAPI: ").append(strFriendlyName);
    deviceInfo.m_deviceType       = aeDeviceType;
    deviceInfo.m_channels         = deviceChannels;

    /* Store the device info */
    deviceInfoList.push_back(deviceInfo);

    if(pDevice->GetId(&pwszID) == S_OK)
    {
      if(wstrDDID.compare(pwszID) == 0)
      {
        deviceInfo.m_deviceName = std::string("default");
        deviceInfo.m_displayName = std::string("default");
        deviceInfo.m_displayNameExtra = std::string("");
        deviceInfoList.push_back(deviceInfo);
      }
      CoTaskMemFree(pwszID);
    }

    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pProperty);
  }
  return;

failed:

  if (FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate WASAPI endpoint devices (%s).", WASAPIErrToStr(hr));

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
}

//Private utility functions////////////////////////////////////////////////////

void CAESinkWASAPI::BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex)
{
  wfxex.Format.wFormatTag        = WAVE_FORMAT_EXTENSIBLE;
  wfxex.Format.cbSize            = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);


  if (!AE_IS_RAW(format.m_dataFormat)) // PCM data
  {
    wfxex.dwChannelMask          = SpeakerMaskFromAEChannels(format.m_channelLayout);
    wfxex.Format.nChannels       = (WORD)format.m_channelLayout.Count();
    wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
    wfxex.Format.wBitsPerSample  = CAEUtil::DataFormatToBits((AEDataFormat) format.m_dataFormat);
    wfxex.SubFormat              = format.m_dataFormat <= AE_FMT_FLOAT ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
  }
  else //Raw bitstream
  {
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    if (format.m_dataFormat == AE_FMT_AC3 || format.m_dataFormat == AE_FMT_DTS)
    {
      wfxex.dwChannelMask               = bool (format.m_channelLayout.Count() == 2) ? KSAUDIO_SPEAKER_STEREO : KSAUDIO_SPEAKER_5POINT1;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = (WORD)format.m_channelLayout.Count();
      wfxex.Format.nSamplesPerSec       = format.m_sampleRate;
    }
    else if (format.m_dataFormat == AE_FMT_EAC3 || format.m_dataFormat == AE_FMT_TRUEHD || format.m_dataFormat == AE_FMT_DTSHD)
    {
      /* IEC 61937 transmissions over HDMI */            
      wfxex.Format.nSamplesPerSec       = 192000L;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_7POINT1_SURROUND;

      switch (format.m_dataFormat)
      {
        case AE_FMT_EAC3:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
          wfxex.Format.nChannels      = 2; // One IEC 60958 Line.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_5POINT1;
          break;
        case AE_FMT_TRUEHD:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
          wfxex.Format.nChannels      = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
        case AE_FMT_DTSHD:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
          wfxex.Format.nChannels      = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
      }

      if (format.m_channelLayout.Count() == 8)
        wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
      else
        wfxex.dwChannelMask         = KSAUDIO_SPEAKER_5POINT1;
    }
  }

  if (wfxex.Format.wBitsPerSample == 32 && wfxex.SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    wfxex.Samples.wValidBitsPerSample = 24;
  else
    wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;

  wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
}

void CAESinkWASAPI::BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex)
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

bool CAESinkWASAPI::InitializeExclusive(AEAudioFormat &format)
{
  WAVEFORMATEXTENSIBLE_IEC61937 wfxex_iec61937;
  WAVEFORMATEXTENSIBLE &wfxex = wfxex_iec61937.FormatExt;

  if (format.m_dataFormat <= AE_FMT_FLOAT)
    BuildWaveFormatExtensible(format, wfxex);
  else
    BuildWaveFormatExtensibleIEC61397(format, wfxex_iec61937);

  /* Test for incomplete format and provide defaults */
  if (format.m_sampleRate == 0 ||
      format.m_channelLayout == NULL ||
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

  HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

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
  else if (AE_IS_RAW(format.m_dataFormat)) //No sense in trying other formats for passthrough.
    return false;

  CLog::Log(LOGERROR, __FUNCTION__": IsFormatSupported failed (%s) - trying to find a compatible format", WASAPIErrToStr(hr));

  int closestMatch;

  /* The requested format is not supported by the device.  Find something that works */
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

      hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

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

  CLog::Log(LOGERROR, __FUNCTION__": Unable to locate a supported output format for the device.  Check the speaker settings in the control panel.");

  /* We couldn't find anything supported. This should never happen      */
  /* unless the user set the wrong speaker setting in the control panel */
  return false;

initialize:

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);

  /* When the stream is raw, the values in the format structure are set to the link    */
  /* parameters, so store the encoded stream values here for the IsCompatible function */
  m_encodedFormat     = format.m_dataFormat;
  m_encodedChannels   = wfxex.Format.nChannels;
  m_encodedSampleRate = format.m_encodedRate;
  wfxex_iec61937.dwEncodedChannelCount = wfxex.Format.nChannels;
  wfxex_iec61937.dwEncodedSamplesPerSec = m_encodedSampleRate;

  /* Set up returned sink format for engine */
  if (!AE_IS_RAW(format.m_dataFormat))
  {
    if (wfxex.Format.wBitsPerSample == 32)
    {
      if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        format.m_dataFormat = AE_FMT_FLOAT;
      else if (wfxex.Samples.wValidBitsPerSample == 32)
        format.m_dataFormat = AE_FMT_S32NE;
      else
        format.m_dataFormat = AE_FMT_S24NE4;
    }
    else if (wfxex.Format.wBitsPerSample == 24)
      format.m_dataFormat = AE_FMT_S24NE3;
    else
      format.m_dataFormat = AE_FMT_S16NE;
  }

  format.m_sampleRate    = wfxex.Format.nSamplesPerSec; //PCM: Sample rate.  RAW: Link speed
  format.m_frameSize     = (wfxex.Format.wBitsPerSample >> 3) * wfxex.Format.nChannels;

  REFERENCE_TIME audioSinkBufferDurationMsec, hnsLatency;

  /* Get m_audioSinkBufferSizeMsec from advancedsettings.xml */
  audioSinkBufferDurationMsec = (REFERENCE_TIME)g_advancedSettings.m_audioSinkBufferDurationMsec * 10000;

  /* Use advancedsetting value for buffer size as long as it's over minimum set above */
  audioSinkBufferDurationMsec = (REFERENCE_TIME)std::max(audioSinkBufferDurationMsec, (REFERENCE_TIME)500000);
  audioSinkBufferDurationMsec = (REFERENCE_TIME)((audioSinkBufferDurationMsec / format.m_frameSize) * format.m_frameSize); //even number of frames

  if (AE_IS_RAW(format.m_dataFormat))
    format.m_dataFormat = AE_FMT_S16NE;

  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                    audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, NULL);

  m_hnsRequestedDuration = audioSinkBufferDurationMsec;

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
    hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Device Activation Failed : %s", WASAPIErrToStr(hr));
      return false;
    }

    /* Open the stream and associate it with an audio session */
    hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                      audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, NULL);
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
    CLog::Log(LOGDEBUG, "  Periodicty      : %d", audioSinkBufferDurationMsec);
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

void CAESinkWASAPI::AEChannelsFromSpeakerMask(DWORD speakers)
{
  m_channelLayout.Reset();

  for (int i = 0; i < WASAPI_SPEAKER_COUNT; i++)
  {
    if (speakers & WASAPIChannelOrder[i])
      m_channelLayout += AEChannelNames[i];
  }
}

DWORD CAESinkWASAPI::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
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

const char *CAESinkWASAPI::WASAPIErrToStr(HRESULT err)
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
  return NULL;
}

void CAESinkWASAPI::Drain()
{
  if(!m_pAudioClient)
    return;

  Sleep( (DWORD)(m_hnsRequestedDuration / 10000));

  if (m_running)
  {
    try
    {
      m_pAudioClient->Stop();  //stop the audio output
      m_pAudioClient->Reset(); //flush buffer and reset audio clock stream position
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__, "Invalidated AudioClient - Releasing");
    }
  }
  m_running = false;
}
