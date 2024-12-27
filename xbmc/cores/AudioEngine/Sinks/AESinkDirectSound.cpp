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
#include "platform/win32/WIN32Util.h"

#include <algorithm>
#include <mutex>

#include <Mmreg.h>
#include <initguid.h>

// include order is important here
// clang-format off
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
// clang-format on

extern HWND g_hWnd;

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

#define EXIT_ON_FAILURE(hr, reason) \
  if (FAILED(hr)) \
  { \
    CLog::LogF(LOGERROR, reason " - error {}", hr, CWIN32Util::FormatHRESULT(hr)); \
    goto failed; \
  }

namespace
{
constexpr unsigned int DS_SPEAKER_COUNT = 8U;
constexpr unsigned int DSChannelOrder[] = {
    SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_CENTER, SPEAKER_LOW_FREQUENCY,
    SPEAKER_BACK_LEFT,  SPEAKER_BACK_RIGHT,  SPEAKER_SIDE_LEFT,    SPEAKER_SIDE_RIGHT};
constexpr enum AEChannel AEChannelNamesDS[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL,
                                               AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};
} // namespace

using namespace Microsoft::WRL;

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

bool CAESinkDirectSound::Initialize(AEAudioFormat& format, std::string& device)
{
  if (m_initialized)
    return false;

  bool deviceIsBluetooth = false;
  GUID deviceGUID = {};
  HRESULT hr = E_FAIL;
  std::string strDeviceGUID = device;
  std::string deviceFriendlyName = "unknown";

  if (StringUtils::EndsWithNoCase(device, "default"))
    strDeviceGUID = CAESinkFactoryWin::GetDefaultDeviceId();

  hr = CLSIDFromString(KODI::PLATFORM::WINDOWS::ToW(strDeviceGUID).c_str(), &deviceGUID);
  if (FAILED(hr))
    CLog::LogF(LOGERROR, "Failed to convert the device '{}' to a GUID, with error {}.",
               strDeviceGUID, CWIN32Util::FormatHRESULT(hr));

  for (const auto& detail : CAESinkFactoryWin::GetRendererDetails())
  {
    if (StringUtils::CompareNoCase(detail.strDeviceId, strDeviceGUID) != 0)
      continue;

    if (detail.strDeviceEnumerator == "BTHENUM")
    {
      deviceIsBluetooth = true;
      CLog::LogF(LOGDEBUG, "Audio device '{}' is detected as Bluetooth device", deviceFriendlyName);
    }

    deviceFriendlyName = detail.strDescription;

    break;
  }

  hr = DirectSoundCreate(&deviceGUID, m_pDSound.ReleaseAndGetAddressOf(), nullptr);

  if (FAILED(hr))
  {
    CLog::LogF(
        LOGERROR,
        "Failed to create the DirectSound device {} with error {}, trying the default device.",
        deviceFriendlyName, CWIN32Util::FormatHRESULT(hr));

    deviceFriendlyName = "default";

    hr = DirectSoundCreate(nullptr, m_pDSound.ReleaseAndGetAddressOf(), nullptr);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Failed to create the default DirectSound device with error {}.",
                 CWIN32Util::FormatHRESULT(hr));
      return false;
    }
  }

  /* Dodge the null handle on first init by using desktop handle */
  HWND tmp_hWnd = g_hWnd == nullptr ? GetDesktopWindow() : g_hWnd;
  CLog::LogF(LOGDEBUG, "Using Window handle: {}", fmt::ptr(tmp_hWnd));

  hr = m_pDSound->SetCooperativeLevel(tmp_hWnd, DSSCL_PRIORITY);

  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Failed to create the DirectSound device cooperative level with error {}.",
               CWIN32Util::FormatHRESULT(hr));
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

  const unsigned int chunkDurationMs = deviceIsBluetooth ? 50 : 20;
  const unsigned int uiFrameCount = format.m_sampleRate * chunkDurationMs / 1000U;
  m_dwFrameSize = wfxex.Format.nBlockAlign;
  m_dwChunkSize = m_dwFrameSize * uiFrameCount;

  // BT: 500ms total buffer (50 * 10); non-BT: 200ms total buffer (20 * 10)
  m_dwBufferLen = m_dwChunkSize * 10;

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
                 CWIN32Util::FormatHRESULT(res));
      // Try without DSBCAPS_LOCHARDWARE
      dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
      res = m_pDSound->CreateSoundBuffer(&dsbdesc, m_pBuffer.ReleaseAndGetAddressOf(), nullptr);
    }
    if (res != DS_OK)
    {
      m_pBuffer = nullptr;
      CLog::LogF(LOGERROR, "cannot create secondary buffer ({})", CWIN32Util::FormatHRESULT(res));
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
  CAEDeviceInfo deviceInfo{};

  for (RendererDetail& detail : CAESinkFactoryWin::GetRendererDetails())
  {
    deviceInfo.m_channels.Reset();
    deviceInfo.m_dataFormats.clear();
    deviceInfo.m_sampleRates.clear();
    deviceInfo.m_streamTypes.clear();

    if (detail.nChannels)
      deviceInfo.m_channels =
          layoutsByChCount[std::max(std::min(detail.nChannels, DS_SPEAKER_COUNT), 2U)];
    deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_FLOAT));

    if (detail.eDeviceType != AE_DEVTYPE_PCM)
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

    if (detail.m_samplesPerSec)
      deviceInfo.m_sampleRates.push_back(std::min(detail.m_samplesPerSec, 192000UL));
    deviceInfo.m_deviceName = detail.strDeviceId;
    deviceInfo.m_displayName = detail.strWinDevType.append(detail.strDescription);
    deviceInfo.m_displayNameExtra = std::string("DIRECTSOUND: ").append(detail.strDescription);
    deviceInfo.m_deviceType = detail.eDeviceType;
    deviceInfo.m_wantsIECPassthrough = true;
    deviceInfoList.push_back(deviceInfo);

    // add the default device with m_deviceName = default
    if (detail.bDefault)
    {
      deviceInfo.m_deviceName = std::string("default");
      deviceInfo.m_displayName = std::string("default");
      deviceInfo.m_displayNameExtra = std::string("");
      deviceInfo.m_wantsIECPassthrough = true;
      deviceInfoList.push_back(deviceInfo);
    }
  }
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

  if (!(status & DSBSTATUS_PLAYING) &&
      m_CacheLen != 0) // If we have some data, see if we can start playback
  {
    HRESULT hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    CLog::LogF(LOGDEBUG, "Resuming Playback");
    if (FAILED(hr))
      CLog::LogF(LOGERROR, "Failed to play the DirectSound buffer: {}",
                 CWIN32Util::FormatHRESULT(hr));
  }
}

bool CAESinkDirectSound::UpdateCacheStatus()
{
  std::unique_lock<CCriticalSection> lock(m_runLock);

  DWORD playCursor = 0, writeCursor = 0;
  HRESULT res = m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor); // Get the current playback and safe write positions
  if (DS_OK != res)
  {
    CLog::LogF(LOGERROR, "GetCurrentPosition failed. Unable to determine buffer status ({})",
               CWIN32Util::FormatHRESULT(res));
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
