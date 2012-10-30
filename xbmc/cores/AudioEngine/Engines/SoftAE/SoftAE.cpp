/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include <string.h>
#include <sstream>
#include <iterator>

#include "system.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/MathUtils.h"
#include "utils/EndianSwap.h"
#include "threads/SingleLock.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"

#include "SoftAE.h"
#include "SoftAESound.h"
#include "SoftAEStream.h"
#include "AESinkFactory.h"
#include "Interfaces/AESink.h"
#include "Utils/AEUtil.h"
#include "Encoders/AEEncoderFFmpeg.h"

using namespace std;

/* Define idle wait time based on platform in milliseconds */
/* Higher wait times reduce thread CPU overhead when in    */
/* idle or Suspend() modes                                 */
#if defined (TARGET_WINDOWS) || defined (TARGET_LINUX) || \
  defined (TARGET_DARWIN_OSX) || defined (TARGET_FREEBSD)
#define SOFTAE_IDLE_WAIT_MSEC 50  // shorter sleep for HTPC's
#elif defined (TARGET_RASPBERRY_PI) || defined (TARGET_ANDROID)
#define SOFTAE_IDLE_WAIT_MSEC 100 // longer for R_PI and Android
#else
#define SOFTAE_IDLE_WAIT_MSEC 100 // catchall for undefined platforms
#endif

CSoftAE::CSoftAE():
  m_thread             (NULL        ),
  m_audiophile         (true        ),
  m_running            (false       ),
  m_reOpen             (false       ),
  m_isSuspended        (false       ),
  m_softSuspend        (false       ),
  m_softSuspendTimer   (0           ),
  m_sink               (NULL        ),
  m_transcode          (false       ),
  m_rawPassthrough     (false       ),
  m_soundMode          (AE_SOUND_OFF),
  m_streamsPlaying     (false       ),
  m_encoder            (NULL        ),
  m_converted          (NULL        ),
  m_convertedSize      (0           ),
  m_masterStream       (NULL        ),
  m_outputStageFn      (NULL        ),
  m_streamStageFn      (NULL        )
{
  CAESinkFactory::EnumerateEx(m_sinkInfoList);
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    CLog::Log(LOGNOTICE, "Enumerated %s devices:", itt->m_sinkName.c_str());
    int count = 0;
    for (AEDeviceInfoList::iterator itt2 = itt->m_deviceInfoList.begin(); itt2 != itt->m_deviceInfoList.end(); ++itt2)
    {
      CLog::Log(LOGNOTICE, "    Device %d", ++count);
      CAEDeviceInfo& info = *itt2;
      std::stringstream ss((std::string)info);
      std::string line;
      while(std::getline(ss, line, '\n'))
        CLog::Log(LOGNOTICE, "        %s", line.c_str());
    }
  }
}

CSoftAE::~CSoftAE()
{
  Deinitialize();

  /* free the streams */
  CSingleLock streamLock(m_streamLock);
  while (!m_streams.empty())
  {
    CSoftAEStream *s = m_streams.front();
    delete s;
  }

  /* free the sounds */
  CSingleLock soundLock(m_soundLock);
  while (!m_sounds.empty())
  {
    CSoftAESound *s = m_sounds.front();
    m_sounds.pop_front();
    delete s;
  }
}

IAESink *CSoftAE::GetSink(AEAudioFormat &newFormat, bool passthrough, std::string &device)
{
  device = passthrough ? m_passthroughDevice : m_device;

  /* if we are raw, force the sample rate */
  if (AE_IS_RAW(newFormat.m_dataFormat))
  {
    switch (newFormat.m_dataFormat)
    {
      case AE_FMT_AC3:
      case AE_FMT_DTS:
        break;

      case AE_FMT_EAC3:
        newFormat.m_sampleRate = 192000;
        break;

      case AE_FMT_TRUEHD:
      case AE_FMT_DTSHD:
        newFormat.m_sampleRate = 192000;
        break;

      default:
        break;
    }
  }

  IAESink *sink = CAESinkFactory::Create(device, newFormat, passthrough);
  return sink;
}

/* this method MUST be called while holding m_streamLock */
inline CSoftAEStream *CSoftAE::GetMasterStream()
{
  /* remove any destroyed streams first */
  for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end();)
  {
    CSoftAEStream *stream = *itt;
    if (stream->IsDestroyed())
    {
      RemoveStream(m_playingStreams, stream);
      RemoveStream(m_streams       , stream);
      delete stream;
      continue;
    }
    ++itt;
  }

  if (!m_newStreams.empty())
    return m_newStreams.back();

  if (!m_streams.empty())
    return m_streams.back();

  return NULL;
}

/* save method to call outside of the main thread, use this one */
void CSoftAE::OpenSink()
{
  m_reOpenEvent.Reset();
  m_reOpen = true;
  m_reOpenEvent.Wait();
  m_wake.Set();
}

/* this must NEVER be called from outside the main thread or Initialization */
void CSoftAE::InternalOpenSink()
{
  /* save off our raw/passthrough mode for checking */
  bool wasTranscode           = m_transcode;
  bool wasRawPassthrough      = m_rawPassthrough;
  bool reInit                 = false;

  LoadSettings();

  /* initialize for analog output */
  m_rawPassthrough = false;
  m_streamStageFn = &CSoftAE::RunStreamStage;
  m_outputStageFn = &CSoftAE::RunOutputStage;

  /* initialize the new format for basic 2.0 output */
  AEAudioFormat newFormat;
  newFormat.m_dataFormat    = AE_FMT_FLOAT;
  newFormat.m_sampleRate    = 44100;
  newFormat.m_encodedRate   = 0;
  newFormat.m_channelLayout = m_stereoUpmix ? m_stdChLayout : AE_CH_LAYOUT_2_0;
  newFormat.m_frames        = 0;
  newFormat.m_frameSamples  = 0;
  newFormat.m_frameSize     = 0;

  CSingleLock streamLock(m_streamLock);

  m_masterStream = GetMasterStream();
  if (m_masterStream)
  {
    /* choose the sample rate & channel layout based on the master stream */
    newFormat.m_sampleRate = m_masterStream->GetSampleRate();
    if (!m_stereoUpmix)
      newFormat.m_channelLayout = m_masterStream->m_initChannelLayout;

    if (m_masterStream->IsRaw())
    {
      newFormat.m_sampleRate    = m_masterStream->GetSampleRate();
      newFormat.m_encodedRate   = m_masterStream->GetEncodedSampleRate();
      newFormat.m_dataFormat    = m_masterStream->GetDataFormat();
      newFormat.m_channelLayout = m_masterStream->m_initChannelLayout;
      m_rawPassthrough = true;
      m_streamStageFn  = &CSoftAE::RunRawStreamStage;
      m_outputStageFn  = &CSoftAE::RunRawOutputStage;
    }
    else
    {
      if (!m_transcode)
        newFormat.m_channelLayout.ResolveChannels(m_stdChLayout);
      else
      {
        if (m_masterStream->m_initChannelLayout == AE_CH_LAYOUT_2_0)
          m_transcode = false;
        m_encoderInitFrameSizeMul  = 1.0 / (newFormat.m_channelLayout.Count() * 
                                           (CAEUtil::DataFormatToBits(newFormat.m_dataFormat) >> 3));
        m_encoderInitSampleRateMul = 1.0 / newFormat.m_sampleRate;
      }
    }

    /* if the stream is paused we cant use it for anything else */
    if (m_masterStream->m_paused)
      m_masterStream = NULL;
  }
  else
    m_transcode = false;

  if (!m_rawPassthrough && m_transcode)
    newFormat.m_dataFormat = AE_FMT_AC3;

  streamLock.Leave();

  std::string device, driver;
  if (m_transcode || m_rawPassthrough)
    device = m_passthroughDevice;
  else
    device = m_device;

  CAESinkFactory::ParseDevice(device, driver);
  if (driver.empty() && m_sink)
    driver = m_sink->GetName();

  if (m_rawPassthrough)
    CLog::Log(LOGINFO, "CSoftAE::InternalOpenSink - RAW passthrough enabled");
  else if (m_transcode)
    CLog::Log(LOGINFO, "CSoftAE::InternalOpenSink - Transcode passthrough enabled");

  /*
    try to use 48000hz if we are going to transcode, this prevents the sink
    from being re-opened repeatedly when switching sources, which locks up
    some receivers & crappy integrated sound drivers. Check for as.xml override
  */
  if (m_transcode && !m_rawPassthrough)
  {
    enum AEChannel ac3Layout[3] = {AE_CH_RAW, AE_CH_RAW, AE_CH_NULL};
    newFormat.m_channelLayout = ac3Layout;
    m_outputStageFn = &CSoftAE::RunTranscodeStage;
    if (!g_advancedSettings.m_allowTranscode44100)
      newFormat.m_sampleRate    = 48000;
  }

  /*
    if there is an audio resample rate set, use it, this MAY NOT be honoured as
    the audio sink may not support the requested format, and may change it.
  */
  if (g_advancedSettings.m_audioResample)
  {
    newFormat.m_sampleRate = g_advancedSettings.m_audioResample;
    CLog::Log(LOGINFO, "CSoftAE::InternalOpenSink - Forcing samplerate to %d", newFormat.m_sampleRate);
  }

  /* only re-open the sink if its not compatible with what we need */
  std::string sinkName;
  if (m_sink)
  {
    sinkName = m_sink->GetName();
    std::transform(sinkName.begin(), sinkName.end(), sinkName.begin(), ::toupper);
  }

  if (!m_sink || sinkName != driver || !m_sink->IsCompatible(newFormat, device))
  {
    CLog::Log(LOGINFO, "CSoftAE::InternalOpenSink - sink incompatible, re-starting");

    /* take the sink lock */
    CExclusiveLock sinkLock(m_sinkLock);

    reInit = true;

    /* we are going to open, so close the old sink if it was open */
    if (m_sink)
    {
      m_sink->Drain();
      m_sink->Deinitialize();
      delete m_sink;
      m_sink = NULL;
    }

    /* get the display name of the device */
    GetDeviceFriendlyName(device);

    /* if we already have a driver, prepend it to the device string */
    if (!driver.empty())
      device = driver + ":" + device;

    /* create the new sink */
    m_sink = GetSink(newFormat, m_transcode || m_rawPassthrough, device);

    /* perform basic sanity checks on the format returned by the sink */
    ASSERT(newFormat.m_channelLayout.Count() > 0);
    ASSERT(newFormat.m_dataFormat           <= AE_FMT_FLOAT);
    ASSERT(newFormat.m_frames                > 0);
    ASSERT(newFormat.m_frameSamples          > 0);
    ASSERT(newFormat.m_frameSize            == (CAEUtil::DataFormatToBits(newFormat.m_dataFormat) >> 3) * newFormat.m_channelLayout.Count());
    ASSERT(newFormat.m_sampleRate            > 0);

    CLog::Log(LOGDEBUG, "CSoftAE::InternalOpenSink - %s Initialized:", m_sink->GetName());
    CLog::Log(LOGDEBUG, "  Output Device : %s", m_deviceFriendlyName.c_str());
    CLog::Log(LOGDEBUG, "  Sample Rate   : %d", newFormat.m_sampleRate);
    CLog::Log(LOGDEBUG, "  Sample Format : %s", CAEUtil::DataFormatToStr(newFormat.m_dataFormat));
    CLog::Log(LOGDEBUG, "  Channel Count : %d", newFormat.m_channelLayout.Count());
    CLog::Log(LOGDEBUG, "  Channel Layout: %s", ((std::string)newFormat.m_channelLayout).c_str());
    CLog::Log(LOGDEBUG, "  Frames        : %d", newFormat.m_frames);
    CLog::Log(LOGDEBUG, "  Frame Samples : %d", newFormat.m_frameSamples);
    CLog::Log(LOGDEBUG, "  Frame Size    : %d", newFormat.m_frameSize);

    m_sinkFormat              = newFormat;
    m_sinkFormatSampleRateMul = 1.0 / (double)newFormat.m_sampleRate;
    m_sinkFormatFrameSizeMul  = 1.0 / (double)newFormat.m_frameSize;
    m_sinkBlockSize           = newFormat.m_frames * newFormat.m_frameSize;
    // check if sink controls volume, if so, init the volume.
    m_sinkHandlesVolume       = m_sink->HasVolume();
    if (m_sinkHandlesVolume)
      m_sink->SetVolume(m_volume);

    /* invalidate the buffer */
    m_buffer.Empty();
  }
  else
    CLog::Log(LOGINFO, "CSoftAE::InternalOpenSink - keeping old sink with : %s, %s, %dhz",
                          CAEUtil::DataFormatToStr(newFormat.m_dataFormat),
                          ((std::string)newFormat.m_channelLayout).c_str(),
                          newFormat.m_sampleRate);

  reInit = (reInit || m_chLayout != m_sinkFormat.m_channelLayout);
  m_chLayout = m_sinkFormat.m_channelLayout;

  size_t neededBufferSize = 0;
  if (m_rawPassthrough)
  {
    if (!wasRawPassthrough)
      m_buffer.Empty();

    m_convertFn      = NULL;
    m_bytesPerSample = CAEUtil::DataFormatToBits(m_sinkFormat.m_dataFormat) >> 3;
    m_frameSize      = m_sinkFormat.m_frameSize;
    neededBufferSize = m_sinkFormat.m_frames * m_sinkFormat.m_frameSize;
  }
  else
  {
    /* if we are transcoding */
    if (m_transcode)
    {
      if (!wasTranscode || wasRawPassthrough)
      {
        /* invalidate the buffer */
        m_buffer.Empty();
        if (m_encoder)
          m_encoder->Reset();
      }

      /* configure the encoder */
      AEAudioFormat encoderFormat;
      encoderFormat.m_dataFormat    = AE_FMT_FLOAT;
      encoderFormat.m_sampleRate    = m_sinkFormat.m_sampleRate;
      encoderFormat.m_encodedRate   = 0;
      encoderFormat.m_channelLayout = m_chLayout;
      encoderFormat.m_frames        = 0;
      encoderFormat.m_frameSamples  = 0;
      encoderFormat.m_frameSize     = 0;
      
      if (!m_encoder || !m_encoder->IsCompatible(encoderFormat))
      {
        m_buffer.Empty();
        SetupEncoder(encoderFormat);
        m_encoderFormat       = encoderFormat;
        if (encoderFormat.m_frameSize > 0)
          m_encoderFrameSizeMul = 1.0 / (double)m_sinkFormat.m_frameSize;
        else
          m_encoderFrameSizeMul = 1.0;
      }

      /* remap directly to the format we need for encode */
      reInit = (reInit || m_chLayout != m_encoderFormat.m_channelLayout);
      m_chLayout       = m_encoderFormat.m_channelLayout;
      m_convertFn      = CAEConvert::FrFloat(m_encoderFormat.m_dataFormat);
      neededBufferSize = m_encoderFormat.m_frames * sizeof(float) * m_chLayout.Count();
      CLog::Log(LOGDEBUG, "CSoftAE::InternalOpenSink - Encoding using layout: %s", ((std::string)m_chLayout).c_str());
    }
    else
    {
      m_convertFn      = CAEConvert::FrFloat(m_sinkFormat.m_dataFormat);
      neededBufferSize = m_sinkFormat.m_frames * sizeof(float) * m_chLayout.Count();
      CLog::Log(LOGDEBUG, "CSoftAE::InternalOpenSink - Using speaker layout: %s", CAEUtil::GetStdChLayoutName(m_stdChLayout));
    }

    m_bytesPerSample = CAEUtil::DataFormatToBits(AE_FMT_FLOAT) >> 3;
    m_frameSize      = m_bytesPerSample * m_chLayout.Count();
  }

  CLog::Log(LOGDEBUG, "CSoftAE::InternalOpenSink - Internal Buffer Size: %d", (int)neededBufferSize);
  if (m_buffer.Size() < neededBufferSize)
    m_buffer.Alloc(neededBufferSize);

  if (reInit)
  {
    if (!m_rawPassthrough)
    {
      /* re-init incompatible sounds */
      CSingleLock soundLock(m_soundLock);
      for (SoundList::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
      {
        CSoftAESound *sound = *itt;
        if (!sound->IsCompatible())
        {
          StopSound(sound);
          sound->Initialize();
        }
      }
    }

    /* re-init streams */
    streamLock.Enter();
    for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->Initialize();
    streamLock.Leave();
  }

  /* any new streams need to be initialized */
  for (StreamList::iterator itt = m_newStreams.begin(); itt != m_newStreams.end(); ++itt)
  {
    (*itt)->Initialize();
    m_streams.push_back(*itt);
    if (!(*itt)->m_paused)
      m_playingStreams.push_back(*itt);
  }
  m_newStreams.clear();
  m_streamsPlaying = !m_playingStreams.empty();

  m_softSuspend = false;

  /* notify any event listeners that we are done */
  m_reOpen = false;
  m_reOpenEvent.Set();
  m_wake.Set();
}

void CSoftAE::ResetEncoder()
{
  if (m_encoder)
    m_encoder->Reset();
  m_encodedBuffer.Empty();
}

bool CSoftAE::SetupEncoder(AEAudioFormat &format)
{
  ResetEncoder();
  delete m_encoder;
  m_encoder = NULL;

  if (!m_transcode)
    return false;

  m_encoder = new CAEEncoderFFmpeg();
  if (m_encoder->Initialize(format))
    return true;

  delete m_encoder;
  m_encoder = NULL;
  return false;
}

void CSoftAE::Shutdown()
{
  Deinitialize();
}

bool CSoftAE::Initialize()
{
  InternalOpenSink();
  m_running = true;
  m_thread  = new CThread(this, "CSoftAE");
  m_thread->Create();
  m_thread->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
  return true;
}

void CSoftAE::OnSettingsChange(const std::string& setting)
{
  if (setting == "audiooutput.passthroughdevice" ||
      setting == "audiooutput.audiodevice"       ||
      setting == "audiooutput.mode"              ||
      setting == "audiooutput.ac3passthrough"    ||
      setting == "audiooutput.dtspassthrough"    ||
      setting == "audiooutput.passthroughaac"    ||
      setting == "audiooutput.truehdpassthrough" ||
      setting == "audiooutput.dtshdpassthrough"  ||
      setting == "audiooutput.channellayout"     ||
      setting == "audiooutput.useexclusivemode"  ||
      setting == "audiooutput.multichannellpcm"  ||
      setting == "audiooutput.stereoupmix")
  {
    OpenSink();
  }

  if (setting == "audiooutput.normalizelevels" || setting == "audiooutput.stereoupmix")
  {
    /* re-init stream reamppers */
    CSingleLock streamLock(m_streamLock);
    for (StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->InitializeRemap();
  }
}

void CSoftAE::LoadSettings()
{
  m_audiophile = g_advancedSettings.m_audioAudiophile;
  if (m_audiophile)
    CLog::Log(LOGINFO, "CSoftAE::LoadSettings - Audiophile switch enabled");

  m_stereoUpmix = g_guiSettings.GetBool("audiooutput.stereoupmix");
  if (m_stereoUpmix)
    CLog::Log(LOGINFO, "CSoftAE::LoadSettings - Stereo upmix is enabled");

  /* load the configuration */
  m_stdChLayout = AE_CH_LAYOUT_2_0;
  switch (g_guiSettings.GetInt("audiooutput.channellayout"))
  {
    default:
    case  0: m_stdChLayout = AE_CH_LAYOUT_2_0; break; /* dont alow 1_0 output */
    case  1: m_stdChLayout = AE_CH_LAYOUT_2_0; break;
    case  2: m_stdChLayout = AE_CH_LAYOUT_2_1; break;
    case  3: m_stdChLayout = AE_CH_LAYOUT_3_0; break;
    case  4: m_stdChLayout = AE_CH_LAYOUT_3_1; break;
    case  5: m_stdChLayout = AE_CH_LAYOUT_4_0; break;
    case  6: m_stdChLayout = AE_CH_LAYOUT_4_1; break;
    case  7: m_stdChLayout = AE_CH_LAYOUT_5_0; break;
    case  8: m_stdChLayout = AE_CH_LAYOUT_5_1; break;
    case  9: m_stdChLayout = AE_CH_LAYOUT_7_0; break;
    case 10: m_stdChLayout = AE_CH_LAYOUT_7_1; break;
  }

  // force optical/coax to 2.0 output channels
  if (!m_rawPassthrough && g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958)
    m_stdChLayout = AE_CH_LAYOUT_2_0;

  /* get the output devices and ensure they exist */
  m_device            = g_guiSettings.GetString("audiooutput.audiodevice");
  m_passthroughDevice = g_guiSettings.GetString("audiooutput.passthroughdevice");
  VerifySoundDevice(m_device           , false);
  VerifySoundDevice(m_passthroughDevice, true );

  m_transcode = (
    g_guiSettings.GetBool("audiooutput.ac3passthrough") /*||
    g_guiSettings.GetBool("audiooutput.dtspassthrough") */
  ) && (
      (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958) ||
      (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_HDMI && !g_guiSettings.GetBool("audiooutput.multichannellpcm"))
  );
}

void CSoftAE::VerifySoundDevice(std::string& device, bool passthrough)
{
  /* check that the specified device exists */
  std::string firstDevice;
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& devInfo = *itt2;
      if (passthrough && devInfo.m_deviceType == AE_DEVTYPE_PCM)
        continue;
      std::string deviceName = sinkInfo.m_sinkName + ":" + devInfo.m_deviceName;

      /* remember the first device so we can default to it if required */
      if (firstDevice.empty())
        firstDevice = deviceName;

      if (device == deviceName)
        return;
    }
  }

  /* if the device wasnt found, set it to the first viable output */
  device = firstDevice;
}

inline void CSoftAE::GetDeviceFriendlyName(std::string &device)
{
  m_deviceFriendlyName = "Device not found";
  /* Match the device and find its friendly name */
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo& devInfo = *itt2;
      if (devInfo.m_deviceName == device)
      {
        m_deviceFriendlyName = devInfo.m_displayName;
        break;
      }
    }
  }
  return;
}

void CSoftAE::Deinitialize()
{
  if (m_thread)
  {
    Stop();
    m_thread->StopThread(true);
    delete m_thread;
    m_thread = NULL;
  }

  if (m_sink)
  {
    /* shutdown the sink */
    m_sink->Deinitialize();
    delete m_sink;
    m_sink = NULL;
  }

  delete m_encoder;
  m_encoder = NULL;
  ResetEncoder();
  m_buffer.DeAlloc();

  _aligned_free(m_converted);
  m_converted = NULL;
  m_convertedSize = 0;
}

void CSoftAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo devInfo = *itt2;
      if (passthrough && devInfo.m_deviceType == AE_DEVTYPE_PCM)
        continue;

      std::string device = sinkInfo.m_sinkName + ":" + devInfo.m_deviceName;

      std::stringstream ss;

      /* add the sink name if we have more then one sink type */
      if (m_sinkInfoList.size() > 1)
        ss << sinkInfo.m_sinkName << ": ";

      ss << devInfo.m_displayName;
      if (!devInfo.m_displayNameExtra.empty())
        ss << ", " << devInfo.m_displayNameExtra;

      devices.push_back(AEDevice(ss.str(), device));
    }
  }
}

std::string CSoftAE::GetDefaultDevice(bool passthrough)
{
  for (AESinkInfoList::iterator itt = m_sinkInfoList.begin(); itt != m_sinkInfoList.end(); ++itt)
  {
    AESinkInfo sinkInfo = *itt;
    for (AEDeviceInfoList::iterator itt2 = sinkInfo.m_deviceInfoList.begin(); itt2 != sinkInfo.m_deviceInfoList.end(); ++itt2)
    {
      CAEDeviceInfo devInfo = *itt2;
      if (passthrough && devInfo.m_deviceType == AE_DEVTYPE_PCM)
        continue;

      std::string device = sinkInfo.m_sinkName + ":" + devInfo.m_deviceName;
      return device;
    }
  }
  return "default";
}

bool CSoftAE::SupportsRaw()
{
  /* CSoftAE supports raw formats */
  return true;
}

void CSoftAE::PauseStream(CSoftAEStream *stream)
{
  CSingleLock streamLock(m_streamLock);
  RemoveStream(m_playingStreams, stream);
  stream->m_paused = true;
  streamLock.Leave();

  OpenSink();
}

void CSoftAE::ResumeStream(CSoftAEStream *stream)
{
  CSingleLock streamLock(m_streamLock);
  m_playingStreams.push_back(stream);
  stream->m_paused = false;
  streamLock.Leave();

  m_streamsPlaying = true;
  OpenSink();
}

void CSoftAE::Stop()
{
  m_running = false;
  m_isSuspended = false;
  m_wake.Set();

  /* wait for the thread to stop */
  CSingleLock lock(m_runningLock);
}

void CSoftAE::SetSoundMode(const int mode)
{
  m_soundMode = mode;

  /* stop all currently playing sounds if they are being turned off */
  if (mode == AE_SOUND_OFF || (mode == AE_SOUND_IDLE && m_streamsPlaying))
    StopAllSounds();
}

IAEStream *CSoftAE::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options/* = 0 */)
{
  CAEChannelInfo channelInfo(channelLayout);
  CLog::Log(LOGINFO, "CSoftAE::MakeStream - %s, %u, %s",
            CAEUtil::DataFormatToStr(dataFormat),
            sampleRate, ((std::string)channelInfo).c_str());

  /* ensure we have the encoded sample rate if the format is RAW */
  if (AE_IS_RAW(dataFormat))
    ASSERT(encodedSampleRate);

  CSingleLock streamLock(m_streamLock);
  CSoftAEStream *stream = new CSoftAEStream(dataFormat, sampleRate, encodedSampleRate, channelLayout, options);
  m_newStreams.push_back(stream);
  streamLock.Leave();

  OpenSink();
  return stream;
}

IAESound *CSoftAE::MakeSound(const std::string& file)
{
  CSingleLock soundLock(m_soundLock);

  CSoftAESound *sound = new CSoftAESound(file);
  if (!sound->Initialize())
  {
    delete sound;
    return NULL;
  }

  m_sounds.push_back(sound);
  return sound;
}

void CSoftAE::PlaySound(IAESound *sound)
{
  if (m_soundMode == AE_SOUND_OFF || (m_soundMode == AE_SOUND_IDLE && m_streamsPlaying))
    return;

  float *samples = ((CSoftAESound*)sound)->GetSamples();
  if (!samples)
    return;

  /* add the sound to the play list */
  CSingleLock soundSampleLock(m_soundSampleLock);
  SoundState ss = {
    ((CSoftAESound*)sound),
    samples,
    ((CSoftAESound*)sound)->GetSampleCount()
  };
  m_playing_sounds.push_back(ss);

  /* wake to play the sound */
  m_softSuspend = false;
  m_wake.Set();
}

void CSoftAE::FreeSound(IAESound *sound)
{
  if (!sound)
    return;

  sound->Stop();
  CSingleLock soundLock(m_soundLock);
  for (SoundList::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }

  delete (CSoftAESound*)sound;
}

void CSoftAE::GarbageCollect()
{
}

unsigned int CSoftAE::GetSampleRate()
{
  if (m_transcode && m_encoder && !m_rawPassthrough)
    return m_encoderFormat.m_sampleRate;

  return m_sinkFormat.m_sampleRate;
}

void CSoftAE::StopSound(IAESound *sound)
{
  CSingleLock lock(m_soundSampleLock);
  for (SoundStateList::iterator itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound)
    {
      (*itt).owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
    }
    else
      ++itt;
  }
}

IAEStream *CSoftAE::FreeStream(IAEStream *stream)
{
  CSingleLock lock(m_streamLock);
  RemoveStream(m_playingStreams, (CSoftAEStream*)stream);
  RemoveStream(m_streams       , (CSoftAEStream*)stream);
  lock.Leave();

  /* if it was the master stream we need to reopen before deletion */
  if (m_masterStream == stream)
    OpenSink();

  delete (CSoftAEStream*)stream;
  return NULL;
}

double CSoftAE::GetDelay()
{
  double delayBuffer = 0.0, delaySink = 0.0, delayTranscoder = 0.0;

  CSharedLock sinkLock(m_sinkLock);
  if (m_sink)
    delaySink = m_sink->GetDelay();
 
  if (m_transcode && m_encoder && !m_rawPassthrough)
  {
    delayBuffer     = (double)m_buffer.Used() * m_encoderInitFrameSizeMul * m_encoderInitSampleRateMul;
    delayTranscoder = m_encoder->GetDelay((double)m_encodedBuffer.Used() * m_encoderFrameSizeMul);
  }
  else
    delayBuffer = (double)m_buffer.Used() * m_sinkFormatFrameSizeMul *m_sinkFormatSampleRateMul;

  return delayBuffer + delaySink + delayTranscoder;
}

double CSoftAE::GetCacheTime()
{
  double timeBuffer = 0.0, timeSink = 0.0, timeTranscoder = 0.0;

  CSharedLock sinkLock(m_sinkLock);
  if (m_sink)
    timeSink = m_sink->GetCacheTime();

  if (m_transcode && m_encoder && !m_rawPassthrough)
  {
    timeBuffer     = (double)m_buffer.Used() * m_encoderInitFrameSizeMul * m_encoderInitSampleRateMul;
    timeTranscoder = m_encoder->GetDelay((double)m_encodedBuffer.Used() * m_encoderFrameSizeMul);
  }
  else
    timeBuffer = (double)m_buffer.Used() * m_sinkFormatFrameSizeMul *m_sinkFormatSampleRateMul;

  return timeBuffer + timeSink + timeTranscoder;
}

double CSoftAE::GetCacheTotal()
{
  double timeBuffer = 0.0, timeSink = 0.0, timeTranscoder = 0.0;

  CSharedLock sinkLock(m_sinkLock);
  if (m_sink)
    timeSink = m_sink->GetCacheTotal();

  if (m_transcode && m_encoder && !m_rawPassthrough)
  {
    timeBuffer     = (double)m_buffer.Size() * m_encoderInitFrameSizeMul * m_encoderInitSampleRateMul;
    timeTranscoder = m_encoder->GetDelay((double)m_encodedBuffer.Size() * m_encoderFrameSizeMul);
  }
  else
    timeBuffer = (double)m_buffer.Size() * m_sinkFormatFrameSizeMul *m_sinkFormatSampleRateMul;

  return timeBuffer + timeSink + timeTranscoder;
}

bool CSoftAE::IsSuspended()
{
  return m_isSuspended;
}

float CSoftAE::GetVolume()
{
  return m_volume;
}

void CSoftAE::SetVolume(float volume)
{
  m_volume = volume;
  if (!m_sinkHandlesVolume)
    return;

  CSharedLock sinkLock(m_sinkLock);
  if (m_sink)
    m_sink->SetVolume(m_volume);
}

void CSoftAE::StopAllSounds()
{
  CSingleLock lock(m_soundSampleLock);
  while (!m_playing_sounds.empty())
  {
    SoundState *ss = &(*m_playing_sounds.begin());
    ss->owner->ReleaseSamples();
    m_playing_sounds.pop_front();
  }
}

bool CSoftAE::Suspend()
{
  CLog::Log(LOGDEBUG, "CSoftAE::Suspend - Suspending AE processing");
  m_isSuspended = true;

  CSingleLock streamLock(m_streamLock);
  
  for (StreamList::iterator itt = m_playingStreams.begin(); itt != m_playingStreams.end(); ++itt)
  {
    CSoftAEStream *stream = *itt;
    stream->Flush();
  }

  return true;
}

bool CSoftAE::Resume()
{
  CLog::Log(LOGDEBUG, "CSoftAE::Resume - Resuming AE processing");
  m_isSuspended = false;
  m_reOpen = true;

  return true;
}

void CSoftAE::Run()
{
  /* we release this when we exit the thread unblocking anyone waiting on "Stop" */
  CSingleLock runningLock(m_runningLock);
  CLog::Log(LOGINFO, "CSoftAE::Run - Thread Started");

  bool hasAudio = false;
  while (m_running)
  {
    bool restart = false;

    if ((this->*m_outputStageFn)(hasAudio) > 0)
      hasAudio = false; /* taken some audio - reset our silence flag */

    /* if we have enough room in the buffer */
    if (m_buffer.Free() >= m_frameSize)
    {
      /* take some data for our use from the buffer */
      uint8_t *out = (uint8_t*)m_buffer.Take(m_frameSize);
      memset(out, 0, m_frameSize);

      /* run the stream stage */
      CSoftAEStream *oldMaster = m_masterStream;
      if ((this->*m_streamStageFn)(m_chLayout.Count(), out, restart) > 0)
        hasAudio = true; /* have some audio */

      /* if in audiophile mode and the master stream has changed, flag for restart */
      if (m_audiophile && oldMaster != m_masterStream)
        restart = true;
    }

    if (m_playingStreams.empty() && m_playing_sounds.empty() && m_streams.empty() && 
       !m_softSuspend && !g_advancedSettings.m_streamSilence)
    {
      m_softSuspend = true;
      m_softSuspendTimer = XbmcThreads::SystemClockMillis() + 10000; //10.0 second delay for softSuspend
    }

    unsigned int curSystemClock = XbmcThreads::SystemClockMillis();

    /* idle while in Suspend() state until Resume() called */
    /* idle if nothing to play and user hasn't enabled     */
    /* continuous streaming (silent stream) in as.xml      */
    while ((m_isSuspended || (m_softSuspend && (curSystemClock > m_softSuspendTimer))) &&
            m_running     && !m_reOpen      && !restart)
    {
      if (m_sink)
      {
        /* take the sink lock */
        CExclusiveLock sinkLock(m_sinkLock);
        //m_sink->Drain(); TODO: implement
        m_sink->Deinitialize();
        delete m_sink;
        m_sink = NULL;
      }
      if (!m_playingStreams.empty() || !m_playing_sounds.empty() || m_sounds.empty())
        m_softSuspend = false;
      m_wake.WaitMSec(SOFTAE_IDLE_WAIT_MSEC);
    }

    /* if we are told to restart */
    if (m_reOpen || restart || !m_sink)
    {
      CLog::Log(LOGDEBUG, "CSoftAE::Run - Sink restart flagged");
      InternalOpenSink();
      m_isSuspended = false; // exit Suspend state
    }
  }
}

void CSoftAE::AllocateConvIfNeeded(size_t convertedSize, bool prezero)
{
  if (m_convertedSize < convertedSize)
  {
    _aligned_free(m_converted);
    m_converted = (uint8_t *)_aligned_malloc(convertedSize, 16);
    m_convertedSize = convertedSize;
  }
  if (prezero)
    memset(m_converted, 0x00, convertedSize);
}

unsigned int CSoftAE::MixSounds(float *buffer, unsigned int samples)
{
  // no point doing anything if we have no sounds,
  // we do not have to take a lock just to check empty
  if (m_playing_sounds.empty())
    return 0;

  SoundStateList::iterator itt;
  unsigned int mixed = 0;
  CSingleLock lock(m_soundSampleLock);
  for (itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt);
    float *out = buffer;

    /* no more frames, so remove it from the list */
    if (ss->sampleCount == 0)
    {
      ss->owner->ReleaseSamples();
      itt = m_playing_sounds.erase(itt);
      continue;
    }

    float volume = ss->owner->GetVolume();
    unsigned int mixSamples = std::min(ss->sampleCount, samples);
    #ifdef __SSE__
      CAEUtil::SSEMulAddArray(out, ss->samples, volume, mixSamples);
    #else
      float *sample_buffer = ss->samples;
      for (unsigned int i = 0; i < mixSamples; ++i)
        *out++ += *sample_buffer++ * volume;
    #endif

    ss->sampleCount -= mixSamples;
    ss->samples     += mixSamples;

    ++itt;
    ++mixed;
  }
  return mixed;
}

bool CSoftAE::FinalizeSamples(float *buffer, unsigned int samples, bool hasAudio)
{
  if (m_soundMode != AE_SOUND_OFF)
    hasAudio |= (MixSounds(buffer, samples) > 0);

  /* no need to process if we don't have audio (buffer is memset to 0) */
  if (!hasAudio)
    return false;

  if (m_muted)
  {
    memset(buffer, 0, samples * sizeof(float));
    return false;
  }

  /* deamplify */
  if (!m_sinkHandlesVolume && m_volume < 1.0)
  {
    #ifdef __SSE__
      CAEUtil::SSEMulArray(buffer, m_volume, samples);
    #else
      float *fbuffer = buffer;
      for (unsigned int i = 0; i < samples; i++)
        *fbuffer++ *= m_volume;
    #endif
  }

  /* check if we need to clamp */
  bool clamp = false;
  float *fbuffer = buffer;
  for (unsigned int i = 0; i < samples; i++, fbuffer++)
  {
    if (*fbuffer < -1.0f || *fbuffer > 1.0f)
    {
      clamp = true;
      break;
    }
  }

  /* if there were no samples outside of the range, dont clamp the buffer */
  if (!clamp)
    return true;

  CLog::Log(LOGDEBUG, "CSoftAE::FinalizeSamples - Clamping buffer of %d samples", samples);
  CAEUtil::ClampArray(buffer, samples);
  return true;
}

int CSoftAE::RunOutputStage(bool hasAudio)
{
  const unsigned int needSamples = m_sinkFormat.m_frames * m_sinkFormat.m_channelLayout.Count();
  const size_t needBytes = needSamples * sizeof(float);
  if (m_buffer.Used() < needBytes)
    return 0;

  void *data = m_buffer.Raw(needBytes);
  hasAudio = FinalizeSamples((float*)data, needSamples, hasAudio);

  int wroteFrames = 0;
  if (m_convertFn)
  {
    const unsigned int convertedBytes = m_sinkFormat.m_frames * m_sinkFormat.m_frameSize;
    AllocateConvIfNeeded(convertedBytes, !hasAudio);
    if (hasAudio)
      m_convertFn((float*)data, needSamples, m_converted);
    data = m_converted;
  }

  /* Output frames to sink */
  if (m_sink)
    wroteFrames = m_sink->AddPackets((uint8_t*)data, m_sinkFormat.m_frames, hasAudio);

  /* Return value of INT_MAX signals error in sink - restart */
  if (wroteFrames == INT_MAX)
  {
    CLog::Log(LOGERROR, "CSoftAE::RunOutputStage - sink error - reinit flagged");
    wroteFrames = 0;
    m_reOpen = true;
  }

  if (wroteFrames)
    m_buffer.Shift(NULL, wroteFrames * m_sinkFormat.m_channelLayout.Count() * sizeof(float));

  return wroteFrames;
}

int CSoftAE::RunRawOutputStage(bool hasAudio)
{
  if(m_buffer.Used() < m_sinkBlockSize)
    return 0;

  void *data = m_buffer.Raw(m_sinkBlockSize);

  if (CAEUtil::S16NeedsByteSwap(AE_FMT_S16NE, m_sinkFormat.m_dataFormat))
  {
    /*
     * It would really be preferable to handle this at packing stage, so that
     * it could byteswap the data efficiently without wasting CPU time on
     * swapping the huge IEC 61937 zero padding between frames (or not
     * byteswap at all, if there are two byteswaps).
     *
     * Unfortunately packing is done on a higher level and we can't easily
     * tell it the needed format from here, so do it here for now (better than
     * nothing)...
     */
    AllocateConvIfNeeded(m_sinkBlockSize, !hasAudio);
    if (hasAudio)
      Endian_Swap16_buf((uint16_t *)m_converted, (uint16_t *)data, m_sinkBlockSize / 2);
    data = m_converted;
  }

  int wroteFrames = 0;
  if (m_sink)
    wroteFrames = m_sink->AddPackets((uint8_t *)data, m_sinkFormat.m_frames, hasAudio);

  /* Return value of INT_MAX signals error in sink - restart */
  if (wroteFrames == INT_MAX)
  {
    CLog::Log(LOGERROR, "CSoftAE::RunRawOutputStage - sink error - reinit flagged");
    wroteFrames = 0;
    m_reOpen = true;
  }

  m_buffer.Shift(NULL, wroteFrames * m_sinkFormat.m_frameSize);
  return wroteFrames;
}

int CSoftAE::RunTranscodeStage(bool hasAudio)
{
  /* if we dont have enough samples to encode yet, return */
  unsigned int block     = m_encoderFormat.m_frames * m_encoderFormat.m_frameSize;
  unsigned int sinkBlock = m_sinkFormat.m_frames    * m_sinkFormat.m_frameSize;

  int encodedFrames = 0;
  if (m_buffer.Used() >= block && m_encodedBuffer.Used() < sinkBlock * 2)
  {
    hasAudio = FinalizeSamples((float*)m_buffer.Raw(block), m_encoderFormat.m_frameSamples, hasAudio);

    void *buffer;
    if (m_convertFn)
    {
      unsigned int newsize = m_encoderFormat.m_frames * m_encoderFormat.m_frameSize;
      AllocateConvIfNeeded(newsize, !hasAudio);
      if (hasAudio)
        m_convertFn((float*)m_buffer.Raw(block),
          m_encoderFormat.m_frames * m_encoderFormat.m_channelLayout.Count(), m_converted);
      buffer = m_converted;
    }
    else
      buffer = m_buffer.Raw(block);

    encodedFrames = m_encoder->Encode((float*)buffer, m_encoderFormat.m_frames);
    m_buffer.Shift(NULL, encodedFrames * m_encoderFormat.m_frameSize);

    uint8_t *packet;
    unsigned int size = m_encoder->GetData(&packet);

    /* if there is not enough space for another encoded packet enlarge the buffer */
    if (m_encodedBuffer.Free() < size)
      m_encodedBuffer.ReAlloc(m_encodedBuffer.Used() + size);

    m_encodedBuffer.Push(packet, size);
  }

  /* if we have enough data to write */
  if (m_encodedBuffer.Used() >= sinkBlock)
  {
    int wroteFrames = m_sink->AddPackets((uint8_t*)m_encodedBuffer.Raw(sinkBlock), m_sinkFormat.m_frames, hasAudio);
    
    /* Return value of INT_MAX signals error in sink - restart */
    if (wroteFrames == INT_MAX)
    {
      CLog::Log(LOGERROR, "CSoftAE::RunTranscodeStage - sink error - reinit flagged");
      wroteFrames = 0;
      m_reOpen = true;
    }

    m_encodedBuffer.Shift(NULL, wroteFrames * m_sinkFormat.m_frameSize);
  }
  return encodedFrames;
}

unsigned int CSoftAE::RunRawStreamStage(unsigned int channelCount, void *out, bool &restart)
{
  StreamList resumeStreams;
  static StreamList::iterator itt;
  CSingleLock streamLock(m_streamLock);

  /* handle playing streams */
  for (itt = m_playingStreams.begin(); itt != m_playingStreams.end(); ++itt)
  {
    CSoftAEStream *sitt = *itt;
    if (sitt == m_masterStream)
      continue;

    /* consume data from streams even though we cant use it */
    uint8_t *frame = sitt->GetFrame();

    /* flag the stream's slave to be resumed if it has drained */
    if (!frame && sitt->IsDrained() && sitt->m_slave && sitt->m_slave->IsPaused())
      resumeStreams.push_back(sitt);
  }

  /* nothing to do if we dont have a master stream */
  if (!m_masterStream)
    return 0;

  /* get the frame and append it to the output */
  uint8_t *frame = m_masterStream->GetFrame();
  unsigned int mixed;
  if (frame)
  {
    mixed = 1;
    memcpy(out, frame, m_sinkFormat.m_frameSize);
  }
  else
  {
    mixed = 0;
    if (m_masterStream->IsDrained() && m_masterStream->m_slave && m_masterStream->m_slave->IsPaused())
      resumeStreams.push_back(m_masterStream);
  }

  ResumeSlaveStreams(resumeStreams);
  return mixed;
}

unsigned int CSoftAE::RunStreamStage(unsigned int channelCount, void *out, bool &restart)
{
  // no point doing anything if we have no streams,
  // we do not have to take a lock just to check empty
  if (m_playingStreams.empty())
    return 0;

  float *dst = (float*)out;
  unsigned int mixed = 0;

  /* identify the master stream */
  CSingleLock streamLock(m_streamLock);

  /* mix in any running streams */
  StreamList resumeStreams;
  for (StreamList::iterator itt = m_playingStreams.begin(); itt != m_playingStreams.end(); ++itt)
  {
    CSoftAEStream *stream = *itt;

    float *frame = (float*)stream->GetFrame();
    if (!frame && stream->IsDrained() && stream->m_slave && stream->m_slave->IsPaused())
      resumeStreams.push_back(stream);

    if (!frame)
      continue;

    float volume = stream->GetVolume() * stream->GetReplayGain();
    #ifdef __SSE__
    if (channelCount > 1)
      CAEUtil::SSEMulAddArray(dst, frame, volume, channelCount);
    else
    #endif
    {
      for (unsigned int i = 0; i < channelCount; ++i)
        *dst++ += *frame++ * volume;
    }

    ++mixed;
  }

  ResumeSlaveStreams(resumeStreams);
  return mixed;
}

inline void CSoftAE::ResumeSlaveStreams(const StreamList &streams)
{
  if (streams.empty())
    return;

  /* resume any streams that need to be */
  for (StreamList::const_iterator itt = streams.begin(); itt != streams.end(); ++itt)
  {
    CSoftAEStream *stream = *itt;
    m_playingStreams.push_back(stream->m_slave);
    stream->m_slave->m_paused = false;
    stream->m_slave = NULL;
  }
}

inline void CSoftAE::RemoveStream(StreamList &streams, CSoftAEStream *stream)
{
  StreamList::iterator f = std::find(streams.begin(), streams.end(), stream);
  if (f != streams.end())
    streams.erase(f);

  if (streams == m_playingStreams)
    m_streamsPlaying = !m_playingStreams.empty();
}

