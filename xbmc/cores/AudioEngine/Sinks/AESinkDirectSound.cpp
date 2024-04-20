/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#define INITGUID


#include "AESinkDirectSound.h"

#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/windows/AESinkFactoryWin.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <algorithm>
#include <list>
#include <mutex>

#include <Audioclient.h>
#include <Mmreg.h>
#include <Rpc.h>
#include <initguid.h>

// include order is important here
// clang-format off
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
// clang-format on

#pragma comment(lib, "Rpcrt4.lib")

extern HWND g_hWnd;

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

extern const char *WASAPIErrToStr(HRESULT err);
#define EXIT_ON_FAILURE(hr, reason) \
  if (FAILED(hr)) \
  { \
    CLog::LogF(LOGERROR, reason " - HRESULT = {} ErrorMessage = {}", hr, WASAPIErrToStr(hr)); \
    goto failed; \
  }

#define DS_SPEAKER_COUNT 8
static const unsigned int DSChannelOrder[] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_CENTER, SPEAKER_LOW_FREQUENCY, SPEAKER_BACK_LEFT, SPEAKER_BACK_RIGHT, SPEAKER_SIDE_LEFT, SPEAKER_SIDE_RIGHT};
static const enum AEChannel AEChannelNamesDS[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

using namespace Microsoft::WRL;

struct DSDevice
{
  std::string name;
  LPGUID     lpGuid;
};

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
  DSDevice dev;
  std::list<DSDevice> &enumerator = *static_cast<std::list<DSDevice>*>(lpContext);

  dev.name = KODI::PLATFORM::WINDOWS::FromW(lpcstrDescription);

  dev.lpGuid = lpGuid;

  if (lpGuid)
    enumerator.push_back(dev);

  return TRUE;
}

CAESinkDirectSound::CAESinkDirectSound() :
  m_pBuffer       (nullptr),
  m_pDSound       (nullptr),
  m_encodedFormat (AE_FMT_INVALID),
  m_AvgBytesPerSec(0    ),
  m_dwChunkSize   (0    ),
  m_dwFrameSize   (0    ),
  m_dwBufferLen   (0    ),
  m_BufferOffset  (0    ),
  m_CacheLen      (0    ),
  m_BufferTimeouts(0    ),
  m_running       (false),
  m_initialized   (false),
  m_isDirtyDS     (false)
{
  m_channelLayout.Reset();
}

CAESinkDirectSound::~CAESinkDirectSound()
{
  Deinitialize();
}

void CAESinkDirectSound::Register()
{
  AE::AESinkRegEntry reg;
  reg.sinkName = "DIRECTSOUND";
  reg.createFunc = CAESinkDirectSound::Create;
  reg.enumerateFunc = CAESinkDirectSound::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(reg);
}

std::unique_ptr<IAESink> CAESinkDirectSound::Create(std::string& device,
                                                    AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkDirectSound>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

bool CAESinkDirectSound::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  LPGUID deviceGUID = nullptr;
  RPC_WSTR wszUuid  = nullptr;
  HRESULT hr = E_FAIL;
  std::string strDeviceGUID = device;
  std::list<DSDevice> DSDeviceList;
  std::string deviceFriendlyName;
  DirectSoundEnumerate(DSEnumCallback, &DSDeviceList);

  if(StringUtils::EndsWithNoCase(device, std::string("default")))
    strDeviceGUID = GetDefaultDevice();

  for (std::list<DSDevice>::iterator itt = DSDeviceList.begin(); itt != DSDeviceList.end(); ++itt)
  {
    if ((*itt).lpGuid)
    {
      hr = (UuidToString((*itt).lpGuid, &wszUuid));
      std::string sztmp = KODI::PLATFORM::WINDOWS::FromW(reinterpret_cast<wchar_t*>(wszUuid));
      std::string szGUID = "{" + std::string(sztmp.begin(), sztmp.end()) + "}";
      if (StringUtils::CompareNoCase(szGUID, strDeviceGUID) == 0)
      {
        deviceGUID = (*itt).lpGuid;
        deviceFriendlyName = (*itt).name.c_str();
        break;
      }
    }
    if (hr == RPC_S_OK) RpcStringFree(&wszUuid);
  }

  hr = DirectSoundCreate(deviceGUID, m_pDSound.ReleaseAndGetAddressOf(), nullptr);

  if (FAILED(hr))
  {
    CLog::LogF(
        LOGERROR,
        "Failed to create the DirectSound device {} with error {}, trying the default device.",
        deviceFriendlyName, dserr2str(hr));

    hr = DirectSoundCreate(nullptr, m_pDSound.ReleaseAndGetAddressOf(), nullptr);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Failed to create the default DirectSound device with error {}.",
                 dserr2str(hr));
      return false;
    }
  }

  /* Dodge the null handle on first init by using desktop handle */
  HWND tmp_hWnd = g_hWnd == nullptr ? GetDesktopWindow() : g_hWnd;
  CLog::LogF(LOGDEBUG, "Using Window handle: {}", fmt::ptr(tmp_hWnd));

  hr = m_pDSound->SetCooperativeLevel(tmp_hWnd, DSSCL_PRIORITY);

  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Failed to create the DirectSound device cooperative level.");
    CLog::LogF(LOGERROR, "DSErr: {}", dserr2str(hr));
    m_pDSound = nullptr;
    return false;
  }

  // clamp samplerate between 44100 and 192000
  if (format.m_sampleRate < 44100)
    format.m_sampleRate = 44100;

  if (format.m_sampleRate > 192000)
    format.m_sampleRate = 192000;

  // fill waveformatex
  WAVEFORMATEXTENSIBLE wfxex = {};
  wfxex.Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = format.m_channelLayout.Count();
  wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  }
  else
  {
    wfxex.dwChannelMask          = SpeakerMaskFromAEChannels(format.m_channelLayout);
    wfxex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    wfxex.Format.wBitsPerSample  = 32;
  }

  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  const unsigned int uiFrameCount = static_cast<int>(format.m_sampleRate * 0.050); // 50ms chunks
  m_dwFrameSize = wfxex.Format.nBlockAlign;
  m_dwChunkSize = m_dwFrameSize * uiFrameCount;
  m_dwBufferLen = m_dwChunkSize * 8; // 400ms total buffer

  // fill in the secondary sound buffer descriptor
  DSBUFFERDESC dsbdesc = {};
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_TRUEPLAYPOSITION    /** Vista+ accurate position */
                  | DSBCAPS_GLOBALFOCUS;         /** Allows background playing */

  dsbdesc.dwBufferBytes = m_dwBufferLen;
  dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wfxex;

  // now create the stream buffer
  HRESULT res = m_pDSound->CreateSoundBuffer(&dsbdesc, m_pBuffer.ReleaseAndGetAddressOf(), nullptr);
  if (res != DS_OK)
  {
    if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE)
    {
      CLog::LogF(LOGDEBUG, "Couldn't create secondary buffer ({}). Trying without LOCHARDWARE.",
                 dserr2str(res));
      // Try without DSBCAPS_LOCHARDWARE
      dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
      res = m_pDSound->CreateSoundBuffer(&dsbdesc, m_pBuffer.ReleaseAndGetAddressOf(), nullptr);
    }
    if (res != DS_OK)
    {
      m_pBuffer = nullptr;
      CLog::LogF(LOGERROR, "cannot create secondary buffer ({})", dserr2str(res));
      return false;
    }
  }
  CLog::LogF(LOGDEBUG, "secondary buffer created");

  m_pBuffer->Stop();

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);
  format.m_channelLayout = m_channelLayout;
  m_encodedFormat = format.m_dataFormat;
  format.m_frames = uiFrameCount;
  format.m_frameSize =  ((format.m_dataFormat == AE_FMT_RAW) ? (wfxex.Format.wBitsPerSample >> 3) : sizeof(float)) * format.m_channelLayout.Count();
  format.m_dataFormat = (format.m_dataFormat == AE_FMT_RAW) ? AE_FMT_S16NE : AE_FMT_FLOAT;

  m_format = format;
  m_device = device;

  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_initialized = true;
  m_isDirtyDS = false;

  CLog::LogF(LOGDEBUG, "Initializing DirectSound with the following parameters:");
  CLog::Log(LOGDEBUG, "  Audio Device    : {}", ((std::string)deviceFriendlyName));
  CLog::Log(LOGDEBUG, "  Sample Rate     : {}", wfxex.Format.nSamplesPerSec);
  CLog::Log(LOGDEBUG, "  Sample Format   : {}", CAEUtil::DataFormatToStr(format.m_dataFormat));
  CLog::Log(LOGDEBUG, "  Bits Per Sample : {}", wfxex.Format.wBitsPerSample);
  CLog::Log(LOGDEBUG, "  Valid Bits/Samp : {}", wfxex.Samples.wValidBitsPerSample);
  CLog::Log(LOGDEBUG, "  Channel Count   : {}", wfxex.Format.nChannels);
  CLog::Log(LOGDEBUG, "  Block Align     : {}", wfxex.Format.nBlockAlign);
  CLog::Log(LOGDEBUG, "  Avg. Bytes Sec  : {}", wfxex.Format.nAvgBytesPerSec);
  CLog::Log(LOGDEBUG, "  Samples/Block   : {}", wfxex.Samples.wSamplesPerBlock);
  CLog::Log(LOGDEBUG, "  Format cBSize   : {}", wfxex.Format.cbSize);
  CLog::Log(LOGDEBUG, "  Channel Layout  : {}", ((std::string)format.m_channelLayout));
  CLog::Log(LOGDEBUG, "  Channel Mask    : {}", wfxex.dwChannelMask);
  CLog::Log(LOGDEBUG, "  Frames          : {}", format.m_frames);
  CLog::Log(LOGDEBUG, "  Frame Size      : {}", format.m_frameSize);

  return true;
}

void CAESinkDirectSound::Deinitialize()
{
  if (!m_initialized)
    return;

  CLog::LogF(LOGDEBUG, "Cleaning up");

  if (m_pBuffer)
  {
    m_pBuffer->Stop();
  }

  m_initialized = false;
  m_pBuffer = nullptr;
  m_pDSound = nullptr;
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
}

unsigned int CAESinkDirectSound::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized)
    return 0;

  DWORD total = m_dwFrameSize * frames;
  DWORD len = total;
  unsigned char* pBuffer = (unsigned char*)data[0]+offset*m_format.m_frameSize;

  DWORD bufferStatus = 0;
  if (m_pBuffer->GetStatus(&bufferStatus) != DS_OK)
  {
    CLog::LogF(LOGERROR, "GetStatus() failed");
    return 0;
  }
  if (bufferStatus & DSBSTATUS_BUFFERLOST)
  {
    CLog::LogF(LOGDEBUG, "Buffer allocation was lost. Restoring buffer.");
    m_pBuffer->Restore();
  }

  while (GetSpace() < total)
  {
    if(m_isDirtyDS)
      return INT_MAX;
    else
    {
      KODI::TIME::Sleep(
          std::chrono::milliseconds(static_cast<int>(total * 1000 / m_AvgBytesPerSec)));
    }
  }

  while (len)
  {
    void* start = nullptr, *startWrap = nullptr;
    DWORD size = 0, sizeWrap = 0;
    if (m_BufferOffset >= m_dwBufferLen) // Wrap-around manually
      m_BufferOffset = 0;
    DWORD dwWriteBytes = std::min((int)m_dwChunkSize, (int)len);
    HRESULT res = m_pBuffer->Lock(m_BufferOffset, dwWriteBytes, &start, &size, &startWrap, &sizeWrap, 0);
    if (DS_OK != res)
    {
      CLog::LogF(LOGERROR, "Unable to lock buffer at offset {}. HRESULT: {:#08x}", m_BufferOffset,
                 res);
      m_isDirtyDS = true;
      return INT_MAX;
    }

    memcpy(start, pBuffer, size);

    pBuffer += size;
    len     -= size;

    m_BufferOffset += size;
    if (startWrap) // Write-region wraps to beginning of buffer
    {
      memcpy(startWrap, pBuffer, sizeWrap);
      m_BufferOffset = sizeWrap;

      pBuffer += sizeWrap;
      len     -= sizeWrap;
    }

    m_CacheLen += size + sizeWrap; // This data is now in the cache
    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
  }

  CheckPlayStatus();

  return (total - len) / m_dwFrameSize; // Frames used
}

void CAESinkDirectSound::Stop()
{
  if (m_pBuffer)
    m_pBuffer->Stop();
}

void CAESinkDirectSound::Drain()
{
  if (!m_initialized || m_isDirtyDS)
    return;

  m_pBuffer->Stop();
  HRESULT res = m_pBuffer->SetCurrentPosition(0);
  if (DS_OK != res)
  {
    CLog::LogF(LOGERROR,
               "SetCurrentPosition failed. Unable to determine buffer status. HRESULT = {:#08x}",
               res);
    m_isDirtyDS = true;
    return;
  }
  m_BufferOffset = 0;
  UpdateCacheStatus();
}

void CAESinkDirectSound::GetDelay(AEDelayStatus& status)
{
  if (!m_initialized)
  {
    status.SetDelay(0);
    return;
  }

  /* Make sure we know how much data is in the cache */
  if (!UpdateCacheStatus())
    m_isDirtyDS = true;

  /** returns current cached data duration in seconds */
  status.SetDelay((double)m_CacheLen / (double)m_AvgBytesPerSec);
}

double CAESinkDirectSound::GetCacheTotal()
{
  /** returns total cache capacity in seconds */
  return (double)m_dwBufferLen / (double)m_AvgBytesPerSec;
}

void CAESinkDirectSound::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  CAEDeviceInfo        deviceInfo;

  ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  ComPtr<IMMDeviceCollection> pEnumDevices = nullptr;
  UINT uiCount = 0;

  HRESULT                hr;

  std::string strDD = GetDefaultDevice();

  /* Windows Vista or later - supporting WASAPI device probing */
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate WASAPI device enumerator.")

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, pEnumDevices.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    ComPtr<IMMDevice> pDevice = nullptr;
    ComPtr<IPropertyStore> pProperty = nullptr;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    deviceInfo.m_channels.Reset();
    deviceInfo.m_dataFormats.clear();
    deviceInfo.m_sampleRates.clear();

    hr = pEnumDevices->Item(i, pDevice.GetAddressOf());
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of DirectSound endpoint failed.");
      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of DirectSound endpoint properties failed.");
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of DirectSound endpoint device name failed.");
      goto failed;
    }

    std::string strFriendlyName = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of DirectSound endpoint GUID failed.");
      goto failed;
    }

    std::string strDevName = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of DirectSound endpoint form factor failed.");
      goto failed;
    }
    std::string strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    AEDeviceType aeDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;

    PropVariantClear(&varName);

    /* In shared mode Windows tells us what format the audio must be in. */
    ComPtr<IAudioClient> pClient;
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(pClient.GetAddressOf()));
    EXIT_ON_FAILURE(hr, "Activate device failed.")

    //hr = pClient->GetMixFormat(&pwfxex);
    hr = pProperty->GetValue(PKEY_AudioEngine_DeviceFormat, &varName);
    if (SUCCEEDED(hr) && varName.blob.cbSize > 0)
    {
      WAVEFORMATEX* smpwfxex = (WAVEFORMATEX*)varName.blob.pBlobData;
      deviceInfo.m_channels = layoutsByChCount[std::max(std::min(smpwfxex->nChannels, (WORD) DS_SPEAKER_COUNT), (WORD) 2)];
      deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_FLOAT));
      if (aeDeviceType != AE_DEVTYPE_PCM)
      {
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
        // DTS is played with the same infrastructure as AC3
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
        // signal that we can doe AE_FMT_RAW
        deviceInfo.m_dataFormats.push_back(AE_FMT_RAW);
      }
      deviceInfo.m_sampleRates.push_back(std::min(smpwfxex->nSamplesPerSec, (DWORD) 192000));
    }
    else
    {
      CLog::LogF(LOGERROR, "Getting DeviceFormat failed ({})", WASAPIErrToStr(hr));
    }

    deviceInfo.m_deviceName       = strDevName;
    deviceInfo.m_displayName      = strWinDevType.append(strFriendlyName);
    deviceInfo.m_displayNameExtra = std::string("DIRECTSOUND: ").append(strFriendlyName);
    deviceInfo.m_deviceType       = aeDeviceType;

    deviceInfo.m_wantsIECPassthrough = true;
    deviceInfoList.push_back(deviceInfo);

    // add the default device with m_deviceName = default
    if(strDD == strDevName)
    {
      deviceInfo.m_deviceName = std::string("default");
      deviceInfo.m_displayName = std::string("default");
      deviceInfo.m_displayNameExtra = std::string("");
      deviceInfo.m_wantsIECPassthrough = true;
      deviceInfoList.push_back(deviceInfo);
    }
  }

  return;

failed:

  if (FAILED(hr))
    CLog::LogF(LOGERROR, "Failed to enumerate WASAPI endpoint devices ({}).", WASAPIErrToStr(hr));
}

///////////////////////////////////////////////////////////////////////////////

void CAESinkDirectSound::CheckPlayStatus()
{
  DWORD status = 0;
  if (m_pBuffer->GetStatus(&status) != DS_OK)
  {
    CLog::LogF(LOGERROR, "GetStatus() failed");
    return;
  }

  if (!(status & DSBSTATUS_PLAYING) && m_CacheLen != 0) // If we have some data, see if we can start playback
  {
    HRESULT hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    CLog::LogF(LOGDEBUG, "Resuming Playback");
    if (FAILED(hr))
      CLog::LogF(LOGERROR, "Failed to play the DirectSound buffer: {}", dserr2str(hr));
  }
}

bool CAESinkDirectSound::UpdateCacheStatus()
{
  std::unique_lock<CCriticalSection> lock(m_runLock);

  DWORD playCursor = 0, writeCursor = 0;
  HRESULT res = m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor); // Get the current playback and safe write positions
  if (DS_OK != res)
  {
    CLog::LogF(LOGERROR,
               "GetCurrentPosition failed. Unable to determine buffer status. HRESULT = {:#08x}",
               res);
    m_isDirtyDS = true;
    return false;
  }

  // Check the state of the ring buffer (P->O->W == underrun)
  // These are the logical situations that can occur
  // O: CurrentOffset  W: WriteCursor  P: PlayCursor
  // | | | | | | | | | |
  // ***O----W----P***** < underrun   P > W && O < W (1)
  // | | | | | | | | | |
  // ---P****O----W----- < underrun   O > P && O < W (2)
  // | | | | | | | | | |
  // ---W----P****O----- < underrun   P > W && P < O (3)
  // | | | | | | | | | |
  // ***W****O----P*****              P > W && P > O (4)
  // | | | | | | | | | |
  // ---P****W****O-----              P < W && O > W (5)
  // | | | | | | | | | |
  // ***O----P****W*****              P < W && O < P (6)

  // Check for underruns
  if ((playCursor > writeCursor && m_BufferOffset < writeCursor) ||    // (1)
      (playCursor < m_BufferOffset && m_BufferOffset < writeCursor) || // (2)
      (playCursor > writeCursor && playCursor <  m_BufferOffset))      // (3)
  {
    CLog::Log(LOGWARNING, "CWin32DirectSound::GetSpace - buffer underrun - W:{}, P:{}, O:{}.",
              writeCursor, playCursor, m_BufferOffset);
    m_BufferOffset = writeCursor; // Catch up
    //m_pBuffer->Stop(); // Wait until someone gives us some data to restart playback (prevents glitches)
    m_BufferTimeouts++;
    if (m_BufferTimeouts > 10)
    {
      m_isDirtyDS = true;
      return false;
    }
  }
  else
    m_BufferTimeouts = 0;

  // Calculate available space in the ring buffer
  if (playCursor == m_BufferOffset && m_BufferOffset ==  writeCursor) // Playback is stopped and we are all at the same place
    m_CacheLen = 0;
  else if (m_BufferOffset > playCursor)
    m_CacheLen = m_BufferOffset - playCursor;
  else
    m_CacheLen = m_dwBufferLen - (playCursor - m_BufferOffset);

  return true;
}

unsigned int CAESinkDirectSound::GetSpace()
{
  std::unique_lock<CCriticalSection> lock(m_runLock);
  if (!UpdateCacheStatus())
    m_isDirtyDS = true;
  unsigned int space = m_dwBufferLen - m_CacheLen;

  // We can never allow the internal buffers to fill up complete
  // as we get confused between if the buffer is full or empty
  // so never allow the last chunk to be added
  if (space > m_dwChunkSize)
    return space - m_dwChunkSize;
  else
    return 0;
}

void CAESinkDirectSound::AEChannelsFromSpeakerMask(DWORD speakers)
{
  m_channelLayout.Reset();

  for (int i = 0; i < DS_SPEAKER_COUNT; i++)
  {
    if (speakers & DSChannelOrder[i])
      m_channelLayout += AEChannelNamesDS[i];
  }
}

DWORD CAESinkDirectSound::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
{
  DWORD mask = 0;

  for (unsigned int i = 0; i < channels.Count(); i++)
  {
    for (unsigned int j = 0; j < DS_SPEAKER_COUNT; j++)
      if (channels[i] == AEChannelNamesDS[j])
        mask |= DSChannelOrder[j];
  }

  return mask;
}

const char *CAESinkDirectSound::dserr2str(int err)
{
  switch (err)
  {
    case DS_OK: return "DS_OK";
    case DS_NO_VIRTUALIZATION: return "DS_NO_VIRTUALIZATION";
    case DSERR_ALLOCATED: return "DS_NO_VIRTUALIZATION";
    case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
    case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
    case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
    case DSERR_GENERIC: return "DSERR_GENERIC";
    case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
    case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
    case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
    case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
    case DSERR_NODRIVER: return "DSERR_NODRIVER";
    case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
    case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
    case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
    case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
    case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
    case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
    case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
    case DSERR_BUFFERTOOSMALL: return "DSERR_BUFFERTOOSMALL";
    case DSERR_DS8_REQUIRED: return "DSERR_DS8_REQUIRED";
    case DSERR_SENDLOOP: return "DSERR_SENDLOOP";
    case DSERR_BADSENDBUFFERGUID: return "DSERR_BADSENDBUFFERGUID";
    case DSERR_OBJECTNOTFOUND: return "DSERR_OBJECTNOTFOUND";
    case DSERR_FXUNAVAILABLE: return "DSERR_FXUNAVAILABLE";
    default: return "unknown";
  }
}

std::string CAESinkDirectSound::GetDefaultDevice()
{
  HRESULT hr;
  ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  ComPtr<IMMDevice> pDevice = nullptr;
  ComPtr<IPropertyStore> pProperty = nullptr;
  PROPVARIANT varName;
  std::string strDevName = "default";
  AEDeviceType aeDeviceType;

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate WASAPI device enumerator")

  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, pDevice.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint enumeration failed.")

  hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of DirectSound endpoint properties failed.")

  PropVariantInit(&varName);
  hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
  EXIT_ON_FAILURE(hr, "Retrieval of DirectSound endpoint form factor failed.")

  aeDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;
  PropVariantClear(&varName);

  hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
  EXIT_ON_FAILURE(hr, "Retrieval of DirectSound endpoint GUID failed")

  strDevName = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
  PropVariantClear(&varName);

failed:

  return strDevName;
}
