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
#include "system.h"
#ifdef HAS_ALSA

#include <stdint.h>
#include <limits.h>
#include <sys/utsname.h>
#include <set>
#include <sstream>
#include <string>
#include <algorithm>

#include "AESinkALSA.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Utils/AEELDParser.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/SystemInfo.h"
#include "threads/SingleLock.h"
#include "settings/AdvancedSettings.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif

#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

#define AE_MIN_PERIODSIZE 256

#define ALSA_CHMAP_KERNEL_BLACKLIST

#define ALSA_OPTIONS (SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_RESAMPLE)

#define ALSA_MAX_CHANNELS 16
static enum AEChannel LegacyALSAChannelMap[ALSA_MAX_CHANNELS + 1] = {
  AE_CH_FL      , AE_CH_FR      , AE_CH_BL      , AE_CH_BR      , AE_CH_FC      , AE_CH_LFE     , AE_CH_SL      , AE_CH_SR      ,
  AE_CH_UNKNOWN1, AE_CH_UNKNOWN2, AE_CH_UNKNOWN3, AE_CH_UNKNOWN4, AE_CH_UNKNOWN5, AE_CH_UNKNOWN6, AE_CH_UNKNOWN7, AE_CH_UNKNOWN8, /* for p16v devices */
  AE_CH_NULL
};

static enum AEChannel LegacyALSAChannelMap51Wide[ALSA_MAX_CHANNELS + 1] = {
  AE_CH_FL      , AE_CH_FR      , AE_CH_SL      , AE_CH_SR      , AE_CH_FC      , AE_CH_LFE     , AE_CH_BL      , AE_CH_BR      ,
  AE_CH_UNKNOWN1, AE_CH_UNKNOWN2, AE_CH_UNKNOWN3, AE_CH_UNKNOWN4, AE_CH_UNKNOWN5, AE_CH_UNKNOWN6, AE_CH_UNKNOWN7, AE_CH_UNKNOWN8, /* for p16v devices */
  AE_CH_NULL
};

static enum AEChannel ALSAChannelMapPassthrough[ALSA_MAX_CHANNELS + 1] = {
  AE_CH_RAW     , AE_CH_RAW     , AE_CH_RAW     , AE_CH_RAW     , AE_CH_RAW     , AE_CH_RAW     , AE_CH_RAW      , AE_CH_RAW      ,
  AE_CH_UNKNOWN1, AE_CH_UNKNOWN2, AE_CH_UNKNOWN3, AE_CH_UNKNOWN4, AE_CH_UNKNOWN5, AE_CH_UNKNOWN6, AE_CH_UNKNOWN7, AE_CH_UNKNOWN8, /* for p16v devices */
  AE_CH_NULL
};

static unsigned int ALSASampleRateList[] =
{
  5512,
  8000,
  11025,
  16000,
  22050,
  32000,
  44100,
  48000,
  64000,
  88200,
  96000,
  176400,
  192000,
  384000,
  0
};

CAESinkALSA::CAESinkALSA() :
  m_bufferSize(0),
  m_formatSampleRateMul(0.0),
  m_passthrough(false),
  m_pcm(NULL),
  m_timeout(0),
  m_fragmented(false),
  m_originalPeriodSize(AE_MIN_PERIODSIZE)
{
  /* ensure that ALSA has been initialized */
  if (!snd_config)
    snd_config_update();
}

CAESinkALSA::~CAESinkALSA()
{
  Deinitialize();
}

inline CAEChannelInfo CAESinkALSA::GetChannelLayoutRaw(const AEAudioFormat& format)
{
  unsigned int count = 0;

  switch (format.m_streamInfo.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      count = 8;
      break;
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      count = 2;
      break;
    default:
      count = 0;
      break;
  }

  CAEChannelInfo info;
  for (unsigned int i = 0; i < count; ++i)
    info += ALSAChannelMapPassthrough[i];

  return info;
}

inline CAEChannelInfo CAESinkALSA::GetChannelLayoutLegacy(const AEAudioFormat& format, unsigned int minChannels, unsigned int maxChannels)
{
  enum AEChannel* channelMap = LegacyALSAChannelMap;
  unsigned int count = 0;

  if (format.m_dataFormat == AE_FMT_RAW)
    return GetChannelLayoutRaw(format);

  // According to CEA-861-D only RL and RR are known. In case of a format having SL and SR channels
  // but no BR BL channels, we use the wide map in order to open only the num of channels really
  // needed.
  if (format.m_channelLayout.HasChannel(AE_CH_SL) && !format.m_channelLayout.HasChannel(AE_CH_BL))
  {
    channelMap = LegacyALSAChannelMap51Wide;
  }
  for (unsigned int c = 0; c < 8; ++c)
  {
    for (unsigned int i = 0; i < format.m_channelLayout.Count(); ++i)
    {
      if (format.m_channelLayout[i] == channelMap[c])
      {
        count = c + 1;
        break;
      }
    }
  }
  count = std::max(count, minChannels);
  count = std::min(count, maxChannels);

  CAEChannelInfo info;
  for (unsigned int i = 0; i < count; ++i)
    info += channelMap[i];

  return info;
}

inline CAEChannelInfo CAESinkALSA::GetChannelLayout(const AEAudioFormat& format, unsigned int channels)
{
  CAEChannelInfo info;
  std::string alsaMapStr("none");

  if (format.m_dataFormat == AE_FMT_RAW)
  {
    info = GetChannelLayoutRaw(format);
  }
  else
  {
#ifdef SND_CHMAP_API_VERSION
    /* ask for the actual map */
    snd_pcm_chmap_t* actualMap = NULL;
    if (AllowALSAMaps())
      actualMap = snd_pcm_get_chmap(m_pcm);
    if (actualMap)
    {
      alsaMapStr = ALSAchmapToString(actualMap);

      info = ALSAchmapToAEChannelMap(actualMap);

      /* "fake" a compatible map if it is more suitable for AE */
      if (!info.ContainsChannels(format.m_channelLayout))
      {
        CAEChannelInfo infoAlternate = GetAlternateLayoutForm(info);
        if (infoAlternate.Count())
        {
          std::vector<CAEChannelInfo> alts;
          alts.push_back(info);
          alts.push_back(infoAlternate);
          if (format.m_channelLayout.BestMatch(alts) == 1)
            info = infoAlternate;
        }
      }

      /* add empty channels as needed (with e.g. FL,FR,LFE in 4ch) */
      while (info.Count() < channels)
        info += AE_CH_UNKNOWN1;

      free(actualMap);
    }
    else
#endif
    {
      info = GetChannelLayoutLegacy(format, channels, channels);
    }
  }

  CLog::Log(LOGDEBUG, "CAESinkALSA::GetChannelLayout - Input Channel Count: %d Output Channel Count: %d", format.m_channelLayout.Count(), info.Count());
  CLog::Log(LOGDEBUG, "CAESinkALSA::GetChannelLayout - Requested Layout: %s", std::string(format.m_channelLayout).c_str());
  CLog::Log(LOGDEBUG, "CAESinkALSA::GetChannelLayout - Got Layout: %s (ALSA: %s)", std::string(info).c_str(), alsaMapStr.c_str());

  return info;
}

#ifdef SND_CHMAP_API_VERSION

bool CAESinkALSA::AllowALSAMaps()
{
  /*
   * Some older kernels had various bugs in the HDA HDMI channel mapping, so just
   * blanket blacklist them for now to avoid any breakage.
   * Should be reasonably safe but still blacklisted:
   * 3.10.20+, 3.11.9+, 3.12.1+
   * Safe:
   * 3.12.15+, 3.13+
   */
#ifdef ALSA_CHMAP_KERNEL_BLACKLIST
  static bool checked = false;
  static bool allowed;

  if (!checked)
    allowed = strverscmp(g_sysinfo.GetKernelVersionFull().c_str(), "3.12.15") >= 0;
  checked = true;

  return allowed;
#else
  return true;
#endif
}

AEChannel CAESinkALSA::ALSAChannelToAEChannel(unsigned int alsaChannel)
{
  AEChannel aeChannel;
  switch (alsaChannel)
  {
    case SND_CHMAP_FL:   aeChannel = AE_CH_FL; break;
    case SND_CHMAP_FR:   aeChannel = AE_CH_FR; break;
    case SND_CHMAP_FC:   aeChannel = AE_CH_FC; break;
    case SND_CHMAP_LFE:  aeChannel = AE_CH_LFE; break;
    case SND_CHMAP_RL:   aeChannel = AE_CH_BL; break;
    case SND_CHMAP_RR:   aeChannel = AE_CH_BR; break;
    case SND_CHMAP_FLC:  aeChannel = AE_CH_FLOC; break;
    case SND_CHMAP_FRC:  aeChannel = AE_CH_FROC; break;
    case SND_CHMAP_RC:   aeChannel = AE_CH_BC; break;
    case SND_CHMAP_SL:   aeChannel = AE_CH_SL; break;
    case SND_CHMAP_SR:   aeChannel = AE_CH_SR; break;
    case SND_CHMAP_TFL:  aeChannel = AE_CH_TFL; break;
    case SND_CHMAP_TFR:  aeChannel = AE_CH_TFR; break;
    case SND_CHMAP_TFC:  aeChannel = AE_CH_TFC; break;
    case SND_CHMAP_TC:   aeChannel = AE_CH_TC; break;
    case SND_CHMAP_TRL:  aeChannel = AE_CH_TBL; break;
    case SND_CHMAP_TRR:  aeChannel = AE_CH_TBR; break;
    case SND_CHMAP_TRC:  aeChannel = AE_CH_TBC; break;
    case SND_CHMAP_RLC:  aeChannel = AE_CH_BLOC; break;
    case SND_CHMAP_RRC:  aeChannel = AE_CH_BROC; break;
    default:             aeChannel = AE_CH_UNKNOWN1; break;
  }
  return aeChannel;
}

unsigned int CAESinkALSA::AEChannelToALSAChannel(AEChannel aeChannel)
{
  unsigned int alsaChannel;
  switch (aeChannel)
  {
    case AE_CH_FL:    alsaChannel = SND_CHMAP_FL; break;
    case AE_CH_FR:    alsaChannel = SND_CHMAP_FR; break;
    case AE_CH_FC:    alsaChannel = SND_CHMAP_FC; break;
    case AE_CH_LFE:   alsaChannel = SND_CHMAP_LFE; break;
    case AE_CH_BL:    alsaChannel = SND_CHMAP_RL; break;
    case AE_CH_BR:    alsaChannel = SND_CHMAP_RR; break;
    case AE_CH_FLOC:  alsaChannel = SND_CHMAP_FLC; break;
    case AE_CH_FROC:  alsaChannel = SND_CHMAP_FRC; break;
    case AE_CH_BC:    alsaChannel = SND_CHMAP_RC; break;
    case AE_CH_SL:    alsaChannel = SND_CHMAP_SL; break;
    case AE_CH_SR:    alsaChannel = SND_CHMAP_SR; break;
    case AE_CH_TFL:   alsaChannel = SND_CHMAP_TFL; break;
    case AE_CH_TFR:   alsaChannel = SND_CHMAP_TFR; break;
    case AE_CH_TFC:   alsaChannel = SND_CHMAP_TFC; break;
    case AE_CH_TC:    alsaChannel = SND_CHMAP_TC; break;
    case AE_CH_TBL:   alsaChannel = SND_CHMAP_TRL; break;
    case AE_CH_TBR:   alsaChannel = SND_CHMAP_TRR; break;
    case AE_CH_TBC:   alsaChannel = SND_CHMAP_TRC; break;
    case AE_CH_BLOC:  alsaChannel = SND_CHMAP_RLC; break;
    case AE_CH_BROC:  alsaChannel = SND_CHMAP_RRC; break;
    default:          alsaChannel = SND_CHMAP_UNKNOWN; break;
  }
  return alsaChannel;
}

CAEChannelInfo CAESinkALSA::ALSAchmapToAEChannelMap(snd_pcm_chmap_t* alsaMap)
{
  CAEChannelInfo info;

  for (unsigned int i = 0; i < alsaMap->channels; i++)
    info += ALSAChannelToAEChannel(alsaMap->pos[i]);

  return info;
}

snd_pcm_chmap_t* CAESinkALSA::AEChannelMapToALSAchmap(const CAEChannelInfo& info)
{
  int AECount = info.Count();
  snd_pcm_chmap_t* alsaMap = (snd_pcm_chmap_t*)malloc(sizeof(snd_pcm_chmap_t) + AECount * sizeof(int));

  alsaMap->channels = AECount;

  for (int i = 0; i < AECount; i++)
    alsaMap->pos[i] = AEChannelToALSAChannel(info[i]);

  return alsaMap;
}

snd_pcm_chmap_t* CAESinkALSA::CopyALSAchmap(snd_pcm_chmap_t* alsaMap)
{
  snd_pcm_chmap_t* copyMap = (snd_pcm_chmap_t*)malloc(sizeof(snd_pcm_chmap_t) + alsaMap->channels * sizeof(int));

  copyMap->channels = alsaMap->channels;
  memcpy(copyMap->pos, alsaMap->pos, alsaMap->channels * sizeof(int));

  return copyMap;
}

std::string CAESinkALSA::ALSAchmapToString(snd_pcm_chmap_t* alsaMap)
{
  char buf[128] = { 0 };
  //! @bug ALSA bug - buffer overflow by a factor of 2 is possible
  //! http://mailman.alsa-project.org/pipermail/alsa-devel/2014-December/085815.html
  int err = snd_pcm_chmap_print(alsaMap, sizeof(buf) / 2, buf);
  if (err < 0)
    return "Error";
  return std::string(buf);
}

CAEChannelInfo CAESinkALSA::GetAlternateLayoutForm(const CAEChannelInfo& info)
{
  CAEChannelInfo altLayout;

  /* only handle symmetrical layouts */
  if (info.HasChannel(AE_CH_BL) == info.HasChannel(AE_CH_BR) &&
      info.HasChannel(AE_CH_SL) == info.HasChannel(AE_CH_SR) &&
      info.HasChannel(AE_CH_BLOC) == info.HasChannel(AE_CH_BROC))
  {
    /* CEA-861-D used by HDMI 1.x has 7.1 as back+back-x-of-center, not
     * side+back. Mangle it here. */
    if (info.HasChannel(AE_CH_SL) && info.HasChannel(AE_CH_BL) && !info.HasChannel(AE_CH_BLOC))
    {
      altLayout = info;
      altLayout.ReplaceChannel(AE_CH_BL, AE_CH_BLOC);
      altLayout.ReplaceChannel(AE_CH_BR, AE_CH_BROC);
      altLayout.ReplaceChannel(AE_CH_SL, AE_CH_BL);
      altLayout.ReplaceChannel(AE_CH_SR, AE_CH_BR);
    }
    /* same in reverse */
    else if (!info.HasChannel(AE_CH_SL) && info.HasChannel(AE_CH_BL) && info.HasChannel(AE_CH_BLOC))
    {
      altLayout = info;
      altLayout.ReplaceChannel(AE_CH_BL, AE_CH_SL);
      altLayout.ReplaceChannel(AE_CH_BR, AE_CH_SR);
      altLayout.ReplaceChannel(AE_CH_BLOC, AE_CH_BL);
      altLayout.ReplaceChannel(AE_CH_BROC, AE_CH_BR);
    }
    /* We have side speakers but no back speakers, allow map to back
     * speakers. */
    else if (info.HasChannel(AE_CH_SL) && !info.HasChannel(AE_CH_BL))
    {
      altLayout = info;
      altLayout.ReplaceChannel(AE_CH_SL, AE_CH_BL);
      altLayout.ReplaceChannel(AE_CH_SR, AE_CH_BR);
    }
    /* reverse */
    else if (!info.HasChannel(AE_CH_SL) && info.HasChannel(AE_CH_BL))
    {
      altLayout = info;
      altLayout.ReplaceChannel(AE_CH_BL, AE_CH_SL);
      altLayout.ReplaceChannel(AE_CH_BR, AE_CH_SR);
    }
  }
  return altLayout;
}

snd_pcm_chmap_t* CAESinkALSA::SelectALSAChannelMap(const CAEChannelInfo& info)
{
  snd_pcm_chmap_t* chmap = NULL;
  snd_pcm_chmap_query_t** supportedMaps;

  supportedMaps = snd_pcm_query_chmaps(m_pcm);

  if (!supportedMaps)
    return NULL;

  CAEChannelInfo infoAlternate = GetAlternateLayoutForm(info);

  /* for efficiency, first try to find an exact match, and only then fallback
   * to searching for less perfect matches */
  int i = 0;
  for (snd_pcm_chmap_query_t* supportedMap = supportedMaps[i++];
       supportedMap; supportedMap = supportedMaps[i++])
  {
    if (supportedMap->map.channels == info.Count())
    {
      CAEChannelInfo candidate = ALSAchmapToAEChannelMap(&supportedMap->map);
      const CAEChannelInfo* selectedInfo = &info;

      if (!candidate.ContainsChannels(info) || !info.ContainsChannels(candidate))
      {
        selectedInfo = &infoAlternate;
        if (!candidate.ContainsChannels(infoAlternate) || !infoAlternate.ContainsChannels(candidate))
          continue;
      }

      if (supportedMap->type == SND_CHMAP_TYPE_VAR)
      {
        /* device supports the AE map directly */
        chmap = AEChannelMapToALSAchmap(*selectedInfo);
        break;
      }
      else
      {
        /* device needs 1:1 remapping */
        chmap = CopyALSAchmap(&supportedMap->map);
        break;
      }
    }
  }

  /* if no exact chmap was found, fallback to best-effort */
  if (!chmap)
  {
    CAEChannelInfo allChannels;
    std::vector<CAEChannelInfo> supportedMapsAE;

    /* Convert the ALSA maps to AE maps. */
    int i = 0;
    for (snd_pcm_chmap_query_t* supportedMap = supportedMaps[i++];
        supportedMap; supportedMap = supportedMaps[i++])
      supportedMapsAE.push_back(ALSAchmapToAEChannelMap(&supportedMap->map));

    int score = 0;
    int best = info.BestMatch(supportedMapsAE, &score);

    /* see if we find a better result with the alternate form */
    if (infoAlternate.Count() && score < 0)
    {
      int scoreAlt = 0;
      int bestAlt = infoAlternate.BestMatch(supportedMapsAE, &scoreAlt);
      if (scoreAlt > score)
        best = bestAlt;
    }

    if (best > 0)
      chmap = CopyALSAchmap(&supportedMaps[best]->map);
  }

  if (chmap && g_advancedSettings.CanLogComponent(LOGAUDIO))
    CLog::Log(LOGDEBUG, "CAESinkALSA::SelectALSAChannelMap - Selected ALSA map \"%s\"", ALSAchmapToString(chmap).c_str());

  snd_pcm_free_chmaps(supportedMaps);
  return chmap;
}

#endif // SND_CHMAP_API_VERSION

void CAESinkALSA::GetAESParams(const AEAudioFormat& format, std::string& params)
{
  if (m_passthrough)
    params = "AES0=0x06";
  else
    params = "AES0=0x04";

  params += ",AES1=0x82,AES2=0x00";

  if (m_passthrough && format.m_channelLayout.Count() == 8) params += ",AES3=0x09";
  else if (format.m_sampleRate == 192000) params += ",AES3=0x0e";
  else if (format.m_sampleRate == 176400) params += ",AES3=0x0c";
  else if (format.m_sampleRate ==  96000) params += ",AES3=0x0a";
  else if (format.m_sampleRate ==  88200) params += ",AES3=0x08";
  else if (format.m_sampleRate ==  48000) params += ",AES3=0x02";
  else if (format.m_sampleRate ==  44100) params += ",AES3=0x00";
  else if (format.m_sampleRate ==  32000) params += ",AES3=0x03";
  else params += ",AES3=0x01";
}

bool CAESinkALSA::Initialize(AEAudioFormat &format, std::string &device)
{
  m_initDevice = device;
  m_initFormat = format;
  ALSAConfig inconfig, outconfig;
  inconfig.format = format.m_dataFormat;
  inconfig.sampleRate = format.m_sampleRate;

  /*
   * We can't use the better GetChannelLayout() at this point as the device
   * is not opened yet, and we need inconfig.channels to select the correct
   * device... Legacy layouts should be accurate enough for device selection
   * in all cases, though.
   */
  inconfig.channels = GetChannelLayoutLegacy(format, 2, 8).Count();

  /* if we are raw, correct the data format */
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    inconfig.format   = AE_FMT_S16NE;
    m_passthrough     = true;
  }
  else
  {
    m_passthrough   = false;
  }
#if defined(HAS_LIBAMCODEC)
  if (aml_present())
  {
    aml_set_audio_passthrough(m_passthrough);
    device = "default";
  }
#endif

  if (inconfig.channels == 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - Unable to open the requested channel layout");
    return false;
  }

  AEDeviceType devType = AEDeviceTypeFromName(device);

  std::string AESParams;
  /* digital interfaces should have AESx set, though in practice most
   * receivers don't care */
  if (m_passthrough || devType == AE_DEVTYPE_HDMI || devType == AE_DEVTYPE_IEC958)
    GetAESParams(format, AESParams);

  CLog::Log(LOGINFO, "CAESinkALSA::Initialize - Attempting to open device \"%s\"", device.c_str());

  /* get the sound config */
  snd_config_t *config;
  snd_config_copy(&config, snd_config);

  if (!OpenPCMDevice(device, AESParams, inconfig.channels, &m_pcm, config))
  {
    CLog::Log(LOGERROR, "CAESinkALSA::Initialize - failed to initialize device \"%s\"", device.c_str());
    snd_config_delete(config);
    return false;
  }

  /* get the actual device name that was used */
  device = snd_pcm_name(m_pcm);
  m_device = device;

  CLog::Log(LOGINFO, "CAESinkALSA::Initialize - Opened device \"%s\"", device.c_str());

  /* free the sound config */
  snd_config_delete(config);

#ifdef SND_CHMAP_API_VERSION
  snd_pcm_chmap_t* selectedChmap = NULL;
  if (!m_passthrough && AllowALSAMaps())
  {
    selectedChmap = SelectALSAChannelMap(format.m_channelLayout);
    if (selectedChmap)
    {
      /* update wanted channel count according to the selected map */
      inconfig.channels = selectedChmap->channels;
    }
  }
#endif

  if (!InitializeHW(inconfig, outconfig) || !InitializeSW(outconfig))
  {
#ifdef SND_CHMAP_API_VERSION
    free(selectedChmap);
#endif
    return false;
  }

#ifdef SND_CHMAP_API_VERSION
  if (selectedChmap)
  {
    /* failure is OK, that likely just means the selected chmap is fixed already */
    snd_pcm_set_chmap(m_pcm, selectedChmap);
    free(selectedChmap);
  }
#endif

  // we want it blocking
  snd_pcm_nonblock(m_pcm, 0);
  snd_pcm_prepare (m_pcm);

  if (m_passthrough && inconfig.channels != outconfig.channels)
  {
    CLog::Log(LOGINFO, "CAESinkALSA::Initialize - could not open required number of channels");
    return false;
  }
  // adjust format to the configuration we got
  format.m_channelLayout = GetChannelLayout(format, outconfig.channels);
  format.m_sampleRate = outconfig.sampleRate;
  format.m_frames = outconfig.periodSize;
  format.m_frameSize = outconfig.frameSize;
  format.m_dataFormat = outconfig.format;

  m_format              = format;
  m_formatSampleRateMul = 1.0 / (double)m_format.m_sampleRate;

  return true;
}

snd_pcm_format_t CAESinkALSA::AEFormatToALSAFormat(const enum AEDataFormat format)
{
  if (format == AE_FMT_RAW)
    return SND_PCM_FORMAT_S16;

  switch (format)
  {
    case AE_FMT_U8    : return SND_PCM_FORMAT_U8;
    case AE_FMT_S16NE : return SND_PCM_FORMAT_S16;
    case AE_FMT_S16LE : return SND_PCM_FORMAT_S16_LE;
    case AE_FMT_S16BE : return SND_PCM_FORMAT_S16_BE;
    case AE_FMT_S24NE4: return SND_PCM_FORMAT_S24;
#ifdef __BIG_ENDIAN__
    case AE_FMT_S24NE3: return SND_PCM_FORMAT_S24_3BE;
#else
    case AE_FMT_S24NE3: return SND_PCM_FORMAT_S24_3LE;
#endif
    case AE_FMT_S32NE : return SND_PCM_FORMAT_S32;
    case AE_FMT_FLOAT : return SND_PCM_FORMAT_FLOAT;

    default:
      return SND_PCM_FORMAT_UNKNOWN;
  }
}

bool CAESinkALSA::InitializeHW(const ALSAConfig &inconfig, ALSAConfig &outconfig)
{
  snd_pcm_hw_params_t *hw_params;

  snd_pcm_hw_params_alloca(&hw_params);
  memset(hw_params, 0, snd_pcm_hw_params_sizeof());

  snd_pcm_hw_params_any(m_pcm, hw_params);
  snd_pcm_hw_params_set_access(m_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

  unsigned int sampleRate   = inconfig.sampleRate;
  snd_pcm_hw_params_set_rate_near    (m_pcm, hw_params, &sampleRate, NULL);

  unsigned int channelCount = inconfig.channels;
  /* select a channel count >=wanted, or otherwise the highest available */
  if (snd_pcm_hw_params_set_channels_min(m_pcm, hw_params, &channelCount) == 0)
    snd_pcm_hw_params_set_channels_first(m_pcm, hw_params, &channelCount);
  else
    snd_pcm_hw_params_set_channels_last(m_pcm, hw_params, &channelCount);

  /* ensure we opened X channels or more */
  if (inconfig.channels > channelCount)
  {
    CLog::Log(LOGINFO, "CAESinkALSA::InitializeHW - Unable to open the required number of channels");
  }

  /* update outconfig */
  outconfig.channels = channelCount;

  snd_pcm_format_t fmt = AEFormatToALSAFormat(inconfig.format);
  outconfig.format = inconfig.format;

  if (fmt == SND_PCM_FORMAT_UNKNOWN)
  {
    /* if we dont support the requested format, fallback to float */
    fmt = SND_PCM_FORMAT_FLOAT;
    outconfig.format = AE_FMT_FLOAT;
  }

  /* try the data format */
  if (snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0)
  {
    /* if the chosen format is not supported, try each one in decending order */
    CLog::Log(LOGINFO, "CAESinkALSA::InitializeHW - Your hardware does not support %s, trying other formats", CAEUtil::DataFormatToStr(outconfig.format));
    for (enum AEDataFormat i = AE_FMT_MAX; i > AE_FMT_INVALID; i = (enum AEDataFormat)((int)i - 1))
    {
      if (i == AE_FMT_RAW || i == AE_FMT_MAX)
        continue;

      if (m_passthrough && i != AE_FMT_S16BE && i != AE_FMT_S16LE)
	continue;

      fmt = AEFormatToALSAFormat(i);

      if (fmt == SND_PCM_FORMAT_UNKNOWN || snd_pcm_hw_params_set_format(m_pcm, hw_params, fmt) < 0)
      {
        fmt = SND_PCM_FORMAT_UNKNOWN;
        continue;
      }

      int fmtBits = CAEUtil::DataFormatToBits(i);
      int bits    = snd_pcm_hw_params_get_sbits(hw_params);
      if (bits != fmtBits)
      {
        /* if we opened in 32bit and only have 24bits, signal it accordingly */
        if (fmtBits == 32 && bits == 24)
          i = AE_FMT_S24NE4MSB;
        else
          continue;
      }

      /* record that the format fell back to X */
      outconfig.format = i;
      CLog::Log(LOGINFO, "CAESinkALSA::InitializeHW - Using data format %s", CAEUtil::DataFormatToStr(outconfig.format));
      break;
    }

    /* if we failed to find a valid output format */
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
    {
      CLog::Log(LOGERROR, "CAESinkALSA::InitializeHW - Unable to find a suitable output format");
      return false;
    }
  }

  snd_pcm_uframes_t periodSize, bufferSize;
  snd_pcm_hw_params_get_buffer_size_max(hw_params, &bufferSize);
  snd_pcm_hw_params_get_period_size_max(hw_params, &periodSize, NULL);

  /* 
   We want to make sure, that we have max 200 ms Buffer with 
   a periodSize of approx 50 ms. Choosing a higher bufferSize
   will cause problems with menu sounds. Buffer will be increased
   after those are fixed.
  */
  periodSize  = std::min(periodSize, (snd_pcm_uframes_t) sampleRate / 20);
  bufferSize  = std::min(bufferSize, (snd_pcm_uframes_t) sampleRate / 5);
  
  /* 
   According to upstream we should set buffer size first - so make sure it is always at least
   4x period size to not get underruns (some systems seem to have issues with only 2 periods)
  */
  periodSize = std::min(periodSize, bufferSize / 4);

  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Request: periodSize %lu, bufferSize %lu", periodSize, bufferSize);

  snd_pcm_hw_params_t *hw_params_copy;
  snd_pcm_hw_params_alloca(&hw_params_copy);
  snd_pcm_hw_params_copy(hw_params_copy, hw_params); // copy what we have and is already working

  // Make sure to not initialize too large to not cause underruns
  snd_pcm_uframes_t periodSizeMax = bufferSize / 3;
  if(snd_pcm_hw_params_set_period_size_max(m_pcm, hw_params_copy, &periodSizeMax, NULL) != 0)
  {
    snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
    CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Request: Failed to limit periodSize to %lu", periodSizeMax);
  }
  
  // first trying bufferSize, PeriodSize
  // for more info see here:
  // http://mailman.alsa-project.org/pipermail/alsa-devel/2009-September/021069.html
  // the last three tries are done as within pulseaudio

  // backup periodSize and bufferSize first. Restore them after every failed try
  snd_pcm_uframes_t periodSizeTemp, bufferSizeTemp;
  periodSizeTemp = periodSize;
  bufferSizeTemp = bufferSize;
  if (snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
    || snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
    || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
  {
    bufferSize = bufferSizeTemp;
    periodSize = periodSizeTemp;
    // retry with PeriodSize, bufferSize
    snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
    if (snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0
      || snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
      || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
    {
      // try only periodSize
      periodSize = periodSizeTemp;
      snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restore working copy
      if(snd_pcm_hw_params_set_period_size_near(m_pcm, hw_params_copy, &periodSize, NULL) != 0 
        || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
      {
        // try only BufferSize
        bufferSize = bufferSizeTemp;
        snd_pcm_hw_params_copy(hw_params_copy, hw_params); // restory working copy
        if (snd_pcm_hw_params_set_buffer_size_near(m_pcm, hw_params_copy, &bufferSize) != 0
          || snd_pcm_hw_params(m_pcm, hw_params_copy) != 0)
        {
          // set default that Alsa would choose
          CLog::Log(LOGWARNING, "CAESinkAlsa::IntializeHW - Using default alsa values - set failed");
          if (snd_pcm_hw_params(m_pcm, hw_params) != 0)
          {
            CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Could not init a valid sink");
            return false;
          }
        }
      }
      // reread values when alsa default was kept
      snd_pcm_get_params(m_pcm, &bufferSize, &periodSize);
    }
  }
  
  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Got: periodSize %lu, bufferSize %lu", periodSize, bufferSize);

  /* set the format parameters */
  outconfig.sampleRate   = sampleRate;

  /* if periodSize is too small Audio Engine might starve */
  m_fragmented = false;
  unsigned int fragments = 1;
  if (periodSize < AE_MIN_PERIODSIZE)
  {
    fragments = std::ceil((double) AE_MIN_PERIODSIZE / periodSize);
    CLog::Log(LOGDEBUG, "Audio Driver reports too low periodSize %d - will use %d fragments", (int) periodSize, (int) fragments);
    m_fragmented = true;
  }

  m_originalPeriodSize   = periodSize;
  outconfig.periodSize   = fragments * periodSize;
  outconfig.frameSize    = snd_pcm_frames_to_bytes(m_pcm, 1);

  m_bufferSize = (unsigned int)bufferSize;
  m_timeout    = std::ceil((double)(bufferSize * 1000) / (double)sampleRate);

  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Setting timeout to %d ms", m_timeout);

  return true;
}

bool CAESinkALSA::InitializeSW(const ALSAConfig &inconfig)
{
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_uframes_t boundary;

  snd_pcm_sw_params_alloca(&sw_params);
  memset(sw_params, 0, snd_pcm_sw_params_sizeof());

  snd_pcm_sw_params_current              (m_pcm, sw_params);
  snd_pcm_sw_params_set_start_threshold  (m_pcm, sw_params, INT_MAX);
  snd_pcm_sw_params_set_silence_threshold(m_pcm, sw_params, 0);
  snd_pcm_sw_params_get_boundary         (sw_params, &boundary);
  snd_pcm_sw_params_set_silence_size     (m_pcm, sw_params, boundary);
  snd_pcm_sw_params_set_avail_min        (m_pcm, sw_params, inconfig.periodSize);

  if (snd_pcm_sw_params(m_pcm, sw_params) < 0)
  {
    CLog::Log(LOGERROR, "CAESinkALSA::InitializeSW - Failed to set the parameters");
    return false;
  }

  return true;
}

void CAESinkALSA::Deinitialize()
{
  if (m_pcm)
  {
    Stop();
    snd_pcm_close(m_pcm);
    m_pcm = NULL;
  }
}

void CAESinkALSA::Stop()
{
  if (!m_pcm)
    return;
  snd_pcm_drop(m_pcm);
}

void CAESinkALSA::GetDelay(AEDelayStatus& status)
{
  if (!m_pcm)
  {
    status.SetDelay(0);
    return;
  }
  snd_pcm_sframes_t frames = 0;
  snd_pcm_delay(m_pcm, &frames);

  if (frames < 0)
  {
#if SND_LIB_VERSION >= 0x000901 /* snd_pcm_forward() exists since 0.9.0rc8 */
    snd_pcm_forward(m_pcm, -frames);
#endif
    frames = 0;
  }

  status.SetDelay((double)frames * m_formatSampleRateMul);
}

double CAESinkALSA::GetCacheTotal()
{
  return (double)m_bufferSize * m_formatSampleRateMul;
}

unsigned int CAESinkALSA::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_pcm)
  {
    CLog::Log(LOGERROR, "CAESinkALSA - Tried to add packets without a sink");
    return INT_MAX;
  }

  void *buffer = data[0]+offset*m_format.m_frameSize;
  unsigned int amount = 0;
  int64_t data_left = (int64_t) frames;
  int frames_written = 0;

  while (data_left > 0)
  {
    if (m_fragmented)
      amount = std::min((unsigned int) data_left, m_originalPeriodSize);
    else // take care as we can come here a second time if the sink does not eat all data
      amount = (unsigned int) data_left;

    int ret = snd_pcm_writei(m_pcm, buffer, amount);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CAESinkALSA - snd_pcm_writei(%d) %s - trying to recover", ret, snd_strerror(ret));
      ret = snd_pcm_recover(m_pcm, ret, 1);
      if(ret < 0)
      {
        HandleError("snd_pcm_writei(1)", ret);
        ret = snd_pcm_writei(m_pcm, buffer, amount);
        if (ret < 0)
        {
          HandleError("snd_pcm_writei(2)", ret);
          ret = 0;
        }
      }
    }

    if ( ret > 0 && snd_pcm_state(m_pcm) == SND_PCM_STATE_PREPARED)
      snd_pcm_start(m_pcm);

    if (ret <= 0)
      break;

    frames_written += ret;
    data_left -= ret;
    buffer = data[0]+offset*m_format.m_frameSize + frames_written*m_format.m_frameSize;
  }
  return frames_written;
}

void CAESinkALSA::HandleError(const char* name, int err)
{
  switch(err)
  {
    case -EPIPE:
      CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - underrun", name);
      if ((err = snd_pcm_prepare(m_pcm)) < 0)
        CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - snd_pcm_prepare returned %d (%s)", name, err, snd_strerror(err));
      break;

    case -ESTRPIPE:
      CLog::Log(LOGINFO, "CAESinkALSA::HandleError(%s) - Resuming after suspend", name);

      /* try to resume the stream */
      while((err = snd_pcm_resume(m_pcm)) == -EAGAIN)
        Sleep(1);

      /* if the hardware doesnt support resume, prepare the stream */
      if (err == -ENOSYS)
        if ((err = snd_pcm_prepare(m_pcm)) < 0)
          CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - snd_pcm_prepare returned %d (%s)", name, err, snd_strerror(err));
      break;

    default:
      CLog::Log(LOGERROR, "CAESinkALSA::HandleError(%s) - snd_pcm_writei returned %d (%s)", name, err, snd_strerror(err));
      break;
  }
}

void CAESinkALSA::Drain()
{
  if (!m_pcm)
    return;

  snd_pcm_drain(m_pcm);
  snd_pcm_prepare(m_pcm);
}

void CAESinkALSA::AppendParams(std::string &device, const std::string &params)
{
  /* Note: escaping, e.g. "plug:'something:X=y'" isn't handled,
   * but it is not normally encountered at this point. */

  device += (device.find(':') == std::string::npos) ? ':' : ',';
  device += params;
}

bool CAESinkALSA::TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf)
{
  /* Check if this device was already open (e.g. when checking for supported
   * channel count in EnumerateDevice()) */
  if (*pcmp)
  {
    if (name == snd_pcm_name(*pcmp))
      return true;

    snd_pcm_close(*pcmp);
    *pcmp = NULL;
  }

  int err = snd_pcm_open_lconf(pcmp, name.c_str(), SND_PCM_STREAM_PLAYBACK, ALSA_OPTIONS, lconf);
  if (err < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - Unable to open device \"%s\" for playback", name.c_str());
  }

  return err == 0;
}

bool CAESinkALSA::TryDeviceWithParams(const std::string &name, const std::string &params, snd_pcm_t **pcmp, snd_config_t *lconf)
{
  if (!params.empty())
  {
    std::string nameWithParams = name;
    AppendParams(nameWithParams, params);
    if (TryDevice(nameWithParams, pcmp, lconf))
      return true;
  }

  /* Try the variant without extra parameters.
   * Custom devices often do not take the AESx parameters, for example.
   */
  return TryDevice(name, pcmp, lconf);
}

bool CAESinkALSA::OpenPCMDevice(const std::string &name, const std::string &params, int channels, snd_pcm_t **pcmp, snd_config_t *lconf)
{
 /* Special name denoting surroundXX mangling. This is needed for some
   * devices for multichannel to work. */
  if (name == "@" || name.substr(0, 2) == "@:")
  {
    std::string openName = name.substr(1);

    /* These device names allow alsa-lib to perform special routing if needed
     * for multichannel to work with the audio hardware.
     * Fall through in switch() so that devices with more channels are
     * added as fallback. */
    switch (channels)
    {
      case 3:
      case 4:
        if (TryDeviceWithParams("surround40" + openName, params, pcmp, lconf))
          return true;
      case 5:
      case 6:
        if (TryDeviceWithParams("surround51" + openName, params, pcmp, lconf))
          return true;
      case 7:
      case 8:
        if (TryDeviceWithParams("surround71" + openName, params, pcmp, lconf))
          return true;
    }

    /* Try "sysdefault" and "default" (they provide dmix if needed, and route
     * audio to all extra channels on subdeviced cards),
     * unless the selected devices is not DEV=0 of the card, in which case
     * "sysdefault" and "default" would point to another device.
     * "sysdefault" is a newish device name that won't be overwritten in case
     * system configuration redefines "default". "default" is still tried
     * because "sysdefault" is rather new. */
    size_t devPos = openName.find(",DEV=");
    if (devPos == std::string::npos || (devPos + 5 < openName.size() && openName[devPos+5] == '0'))
    {
      /* "sysdefault" and "default" do not have "DEV=0", drop it */
      std::string nameWithoutDev = openName;
      if (devPos != std::string::npos)
        nameWithoutDev.erase(nameWithoutDev.begin() + devPos, nameWithoutDev.begin() + devPos + 6);

      if (TryDeviceWithParams("sysdefault" + nameWithoutDev, params, pcmp, lconf)
          || TryDeviceWithParams("default" + nameWithoutDev, params, pcmp, lconf))
        return true;
    }

    /* Try "front" (no dmix, no audio in other channels on subdeviced cards) */
    if (TryDeviceWithParams("front" + openName, params, pcmp, lconf))
      return true;

  }
  else
  {
    /* Non-surroundXX device, just add it */
    if (TryDeviceWithParams(name, params, pcmp, lconf))
      return true;
  }

  return false;
}

void CAESinkALSA::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
#if HAVE_LIBUDEV
  m_deviceMonitor.Start();
#endif

  /* ensure that ALSA has been initialized */
  snd_lib_error_set_handler(sndLibErrorHandler);
  if(!snd_config || force)
  {
    if(force)
      snd_config_update_free_global();

    snd_config_update();
  }

  snd_config_t *config;
  snd_config_copy(&config, snd_config);

  m_controlMonitor.Clear();

  /* Always enumerate the default device.
   * Note: If "default" is a stereo device, EnumerateDevice()
   * will automatically add "@" instead to enable surroundXX mangling.
   * We don't want to do that if "default" can handle multichannel
   * itself (e.g. in case of a pulseaudio server). */
  EnumerateDevice(list, "default", "", config);

  void **hints;

  if (snd_device_name_hint(-1, "pcm", &hints) < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - Unable to get a list of devices");
    return;
  }

  std::string defaultDescription;

  for (void** hint = hints; *hint != NULL; ++hint)
  {
    char *io = snd_device_name_get_hint(*hint, "IOID");
    char *name = snd_device_name_get_hint(*hint, "NAME");
    char *desc = snd_device_name_get_hint(*hint, "DESC");
    if ((!io || strcmp(io, "Output") == 0) && name
        && strcmp(name, "null") != 0)
    {
      std::string baseName = std::string(name);
      baseName = baseName.substr(0, baseName.find(':'));

      if (strcmp(name, "default") == 0)
      {
        /* added already, but lets get the description if we have one */
        if (desc)
          defaultDescription = desc;
      }
      else if (baseName == "front")
      {
        /* Enumerate using the surroundXX mangling */
        /* do not enumerate basic "front", it is already handled
         * by the default "@" entry added in the very beginning */
        if (strcmp(name, "front") != 0)
          EnumerateDevice(list, std::string("@") + (name+5), desc ? desc : name, config);
      }

      /* Do not enumerate "default", it is already enumerated above. */

      /* Do not enumerate the sysdefault or surroundXX devices, those are
       * always accompanied with a "front" device and it is handled above
       * as "@". The below devices will be automatically used if available
       * for a "@" device. */

      /* Ubuntu has patched their alsa-lib so that "defaults.namehint.extended"
       * defaults to "on" instead of upstream "off", causing lots of unwanted
       * extra devices (many of which are not actually routed properly) to be
       * found by the enumeration process. Skip them as well ("hw", "dmix",
       * "plughw", "dsnoop"). */

      else if (baseName != "default"
            && baseName != "sysdefault"
            && baseName != "surround40"
            && baseName != "surround41"
            && baseName != "surround50"
            && baseName != "surround51"
            && baseName != "surround71"
            && baseName != "hw"
            && baseName != "dmix"
            && baseName != "plughw"
            && baseName != "dsnoop")
      {
        EnumerateDevice(list, name, desc ? desc : name, config);
      }
    }
    free(io);
    free(name);
    free(desc);
  }
  snd_device_name_free_hint(hints);

  m_controlMonitor.Start();

  /* set the displayname for default device */
  if (!list.empty() && list[0].m_deviceName == "default")
  {
    /* If we have one from a hint (DESC), use it */
    if (!defaultDescription.empty())
      list[0].m_displayName = defaultDescription;
    /* Otherwise use the discovered name or (unlikely) "Default" */
    else if (list[0].m_displayName.empty())
      list[0].m_displayName = "Default";
  }

  /* lets check uniqueness, we may need to append DEV or CARD to DisplayName */
  /* If even a single device of card/dev X clashes with Y, add suffixes to
   * all devices of both them, for clarity. */

  /* clashing card names, e.g. "NVidia", "NVidia_2" */
  std::set<std::string> cardsToAppend;

  /* clashing basename + cardname combinations, e.g. ("hdmi","Nvidia") */
  std::set<std::pair<std::string, std::string> > devsToAppend;

  for (AEDeviceInfoList::iterator it1 = list.begin(); it1 != list.end(); ++it1)
  {
    for (AEDeviceInfoList::iterator it2 = it1+1; it2 != list.end(); ++it2)
    {
      if (it1->m_displayName == it2->m_displayName
       && it1->m_displayNameExtra == it2->m_displayNameExtra)
      {
        /* something needs to be done */
        std::string cardString1 = GetParamFromName(it1->m_deviceName, "CARD");
        std::string cardString2 = GetParamFromName(it2->m_deviceName, "CARD");

        if (cardString1 != cardString2)
        {
          /* card name differs, add identifiers to all devices */
          cardsToAppend.insert(cardString1);
          cardsToAppend.insert(cardString2);
          continue;
        }

        std::string devString1 = GetParamFromName(it1->m_deviceName, "DEV");
        std::string devString2 = GetParamFromName(it2->m_deviceName, "DEV");

        if (devString1 != devString2)
        {
          /* device number differs, add identifiers to all such devices */
          devsToAppend.insert(std::make_pair(it1->m_deviceName.substr(0, it1->m_deviceName.find(':')), cardString1));
          devsToAppend.insert(std::make_pair(it2->m_deviceName.substr(0, it2->m_deviceName.find(':')), cardString2));
          continue;
        }

        /* if we got here, the configuration is really weird, just append the whole device string */
        it1->m_displayName += " (" + it1->m_deviceName + ")";
        it2->m_displayName += " (" + it2->m_deviceName + ")";
      }
    }
  }

  for (std::set<std::string>::iterator it = cardsToAppend.begin();
       it != cardsToAppend.end(); ++it)
  {
    for (AEDeviceInfoList::iterator itl = list.begin(); itl != list.end(); ++itl)
    {
      std::string cardString = GetParamFromName(itl->m_deviceName, "CARD");
      if (cardString == *it)
        /* "HDA NVidia (NVidia)", "HDA NVidia (NVidia_2)", ... */
        itl->m_displayName += " (" + cardString + ")";
    }
  }

  for (std::set<std::pair<std::string, std::string> >::iterator it = devsToAppend.begin();
       it != devsToAppend.end(); ++it)
  {
    for (AEDeviceInfoList::iterator itl = list.begin(); itl != list.end(); ++itl)
    {
      std::string baseName = itl->m_deviceName.substr(0, itl->m_deviceName.find(':'));
      std::string cardString = GetParamFromName(itl->m_deviceName, "CARD");
      if (baseName == it->first && cardString == it->second)
      {
        std::string devString = GetParamFromName(itl->m_deviceName, "DEV");
        /* "HDMI #0", "HDMI #1" ... */
        itl->m_displayNameExtra += " #" + devString;
      }
    }
  }
}

AEDeviceType CAESinkALSA::AEDeviceTypeFromName(const std::string &name)
{
  if (name.substr(0, 4) == "hdmi")
    return AE_DEVTYPE_HDMI;
  else if (name.substr(0, 6) == "iec958" || name.substr(0, 5) == "spdif")
    return AE_DEVTYPE_IEC958;

  return AE_DEVTYPE_PCM;
}

std::string CAESinkALSA::GetParamFromName(const std::string &name, const std::string &param)
{
  /* name = "hdmi:CARD=x,DEV=y" param = "CARD" => return "x" */
  size_t parPos = name.find(param + '=');
  if (parPos != std::string::npos)
  {
    parPos += param.size() + 1;
    return name.substr(parPos, name.find_first_of(",'\"", parPos)-parPos);
  }

  return "";
}

void CAESinkALSA::EnumerateDevice(AEDeviceInfoList &list, const std::string &device, const std::string &description, snd_config_t *config)
{
  snd_pcm_t *pcmhandle = NULL;
  if (!OpenPCMDevice(device, "", ALSA_MAX_CHANNELS, &pcmhandle, config))
    return;

  snd_pcm_info_t *pcminfo;
  snd_pcm_info_alloca(&pcminfo);
  memset(pcminfo, 0, snd_pcm_info_sizeof());

  int err = snd_pcm_info(pcmhandle, pcminfo);
  if (err < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - Unable to get pcm_info for \"%s\"", device.c_str());
    snd_pcm_close(pcmhandle);
  }

  int cardNr = snd_pcm_info_get_card(pcminfo);

  CAEDeviceInfo info;
  info.m_deviceName = device;
  info.m_deviceType = AEDeviceTypeFromName(device);

  if (cardNr >= 0)
  {
    /* "HDA NVidia", "HDA Intel", "HDA ATI HDMI", "SB Live! 24-bit External", ... */
    char *cardName;
    if (snd_card_get_name(cardNr, &cardName) == 0)
      info.m_displayName = cardName;

    if (info.m_deviceType == AE_DEVTYPE_HDMI && info.m_displayName.size() > 5 &&
        info.m_displayName.substr(info.m_displayName.size()-5) == " HDMI")
    {
      /* We already know this is HDMI, strip it */
      info.m_displayName.erase(info.m_displayName.size()-5);
    }

    /* "CONEXANT Analog", "USB Audio", "HDMI 0", "ALC889 Digital" ... */
    std::string pcminfoName = snd_pcm_info_get_name(pcminfo);

    /*
     * Filter "USB Audio", in those cases snd_card_get_name() is more
     * meaningful already
     */
    if (pcminfoName != "USB Audio")
      info.m_displayNameExtra = pcminfoName;

    if (info.m_deviceType == AE_DEVTYPE_HDMI)
    {
      /* replace, this was likely "HDMI 0" */
      info.m_displayNameExtra = "HDMI";

      int dev = snd_pcm_info_get_device(pcminfo);

      if (dev >= 0)
      {
        /* lets see if we can get ELD info */

        snd_ctl_t *ctlhandle;
        std::stringstream sstr;
        sstr << "hw:" << cardNr;
        std::string strHwName = sstr.str();

        if (snd_ctl_open_lconf(&ctlhandle, strHwName.c_str(), 0, config) == 0)
        {
          snd_hctl_t *hctl;
          if (snd_hctl_open_ctl(&hctl, ctlhandle) == 0)
          {
            snd_hctl_load(hctl);
            bool badHDMI = false;

            /* add ELD to monitoring */
            m_controlMonitor.Add(strHwName, SND_CTL_ELEM_IFACE_PCM, dev, "ELD");

            if (!GetELD(hctl, dev, info, badHDMI))
              CLog::Log(LOGDEBUG, "CAESinkALSA - Unable to obtain ELD information for device \"%s\" (not supported by device, or kernel older than 3.2)",
                        device.c_str());

            /* snd_hctl_close also closes ctlhandle */
            snd_hctl_close(hctl);

            if (badHDMI)
            {
              /* 
               * Warn about disconnected devices, but keep them enabled 
               * Detection can go wrong on Intel, Nvidia and on all 
               * AMD (fglrx) hardware, so it is not safe to close those
               * handles
               */
              CLog::Log(LOGDEBUG, "CAESinkALSA - HDMI device \"%s\" may be unconnected (no ELD data)", device.c_str());
            }
          }
          else
          {
            snd_ctl_close(ctlhandle);
          }
        }
      }
    }
    else if (info.m_deviceType == AE_DEVTYPE_IEC958)
    {
      /* append instead of replace, pcminfoName is useful for S/PDIF */
      if (!info.m_displayNameExtra.empty())
        info.m_displayNameExtra += ' ';
      info.m_displayNameExtra += "S/PDIF";

      info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
      info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
      info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
      info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
      info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
      info.m_dataFormats.push_back(AE_FMT_RAW);
    }
    else if (info.m_displayNameExtra.empty())
    {
      /* for USB audio, it gets a bit confusing as there is
       * - "SB Live! 24-bit External"
       * - "SB Live! 24-bit External, S/PDIF"
       * so add "Analog" qualifier to the first one */
      info.m_displayNameExtra = "Analog";
    }

    /* "default" is a device that will be used for all inputs, while
     * "@" will be mangled to front/default/surroundXX as necessary */
    if (device == "@" || device == "default")
    {
      /* Make it "Default (whatever)" */
      info.m_displayName = "Default (" + info.m_displayName + (info.m_displayNameExtra.empty() ? "" : " " + info.m_displayNameExtra + ")");
      info.m_displayNameExtra = "";
    }

  }
  else
  {
    /* virtual devices: "default", "pulse", ... */
    /* description can be e.g. "PulseAudio Sound Server" - for hw devices it is
     * normally uninteresting, like "HDMI Audio Output" or "Default Audio Device",
     * so we only use it for virtual devices that have no better display name */
    info.m_displayName = description;
  }

  snd_pcm_hw_params_t *hwparams;
  snd_pcm_hw_params_alloca(&hwparams);
  memset(hwparams, 0, snd_pcm_hw_params_sizeof());

  /* ensure we can get a playback configuration for the device */
  if (snd_pcm_hw_params_any(pcmhandle, hwparams) < 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - No playback configurations available for device \"%s\"", device.c_str());
    snd_pcm_close(pcmhandle);
    return;
  }

  /* detect the available sample rates */
  for (unsigned int *rate = ALSASampleRateList; *rate != 0; ++rate)
    if (snd_pcm_hw_params_test_rate(pcmhandle, hwparams, *rate, 0) >= 0)
      info.m_sampleRates.push_back(*rate);

  /* detect the channels available */
  int channels = 0;
  for (int i = ALSA_MAX_CHANNELS; i >= 1; --i)
  {
    /* Reopen the device if needed on the special "surroundXX" cases */
    if (info.m_deviceType == AE_DEVTYPE_PCM && (i == 8 || i == 6 || i == 4))
      OpenPCMDevice(device, "", i, &pcmhandle, config);

    if (snd_pcm_hw_params_test_channels(pcmhandle, hwparams, i) >= 0)
    {
      channels = i;
      break;
    }
  }

  if (device == "default" && channels == 2)
  {
    /* This looks like the ALSA standard default stereo dmix device, we
     * probably want to use "@" instead to get surroundXX. */
    snd_pcm_close(pcmhandle);
    EnumerateDevice(list, "@", description, config);
    return;
  }

  CAEChannelInfo alsaChannels;
#ifdef SND_CHMAP_API_VERSION
  snd_pcm_chmap_query_t** alsaMaps = NULL;
  if (AllowALSAMaps())
    alsaMaps = snd_pcm_query_chmaps(pcmhandle);
  bool useEldChannels = (info.m_channels.Count() > 0);
  if (alsaMaps)
  {
    int i = 0;
    for (snd_pcm_chmap_query_t* alsaMap = alsaMaps[i++];
        alsaMap; alsaMap = alsaMaps[i++])
    {
      CAEChannelInfo AEmap = ALSAchmapToAEChannelMap(&alsaMap->map);
      alsaChannels.AddMissingChannels(AEmap);
      if (!useEldChannels)
        info.m_channels.AddMissingChannels(AEmap);
    }
    snd_pcm_free_chmaps(alsaMaps);
  }
  else
#endif
  {
    for (int i = 0; i < channels; ++i)
    {
      if (!info.m_channels.HasChannel(LegacyALSAChannelMap[i]))
        info.m_channels += LegacyALSAChannelMap[i];
      alsaChannels += LegacyALSAChannelMap[i];
    }
  }

  /* remove the channels from m_channels that we cant use */
  info.m_channels.ResolveChannels(alsaChannels);

  /* detect the PCM sample formats that are available */
  for (enum AEDataFormat i = AE_FMT_MAX; i > AE_FMT_INVALID; i = (enum AEDataFormat)((int)i - 1))
  {
    if (i == AE_FMT_RAW || i == AE_FMT_MAX)
      continue;
    snd_pcm_format_t fmt = AEFormatToALSAFormat(i);
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
      continue;

    if (snd_pcm_hw_params_test_format(pcmhandle, hwparams, fmt) >= 0)
      info.m_dataFormats.push_back(i);
  }

  if (info.m_deviceType == AE_DEVTYPE_HDMI)
  {
    // we don't trust ELD information and push back our supported formats explicitely
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
    info.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);

    // indicate that we can do AE_FMT_RAW
    info.m_dataFormats.push_back(AE_FMT_RAW);
  }

  snd_pcm_close(pcmhandle);
  info.m_wantsIECPassthrough = true;
  list.push_back(info);
}

bool CAESinkALSA::GetELD(snd_hctl_t *hctl, int device, CAEDeviceInfo& info, bool& badHDMI)
{
  badHDMI = false;

  snd_ctl_elem_id_t    *id;
  snd_ctl_elem_info_t  *einfo;
  snd_ctl_elem_value_t *control;
  snd_hctl_elem_t      *elem;

  snd_ctl_elem_id_alloca(&id);
  memset(id, 0, snd_ctl_elem_id_sizeof());

  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_PCM);
  snd_ctl_elem_id_set_name     (id, "ELD" );
  snd_ctl_elem_id_set_device   (id, device);
  elem = snd_hctl_find_elem(hctl, id);
  if (!elem)
    return false;

  snd_ctl_elem_info_alloca(&einfo);
  memset(einfo, 0, snd_ctl_elem_info_sizeof());

  if (snd_hctl_elem_info(elem, einfo) < 0)
    return false;

  if (!snd_ctl_elem_info_is_readable(einfo))
    return false;

  if (snd_ctl_elem_info_get_type(einfo) != SND_CTL_ELEM_TYPE_BYTES)
    return false;

  snd_ctl_elem_value_alloca(&control);
  memset(control, 0, snd_ctl_elem_value_sizeof());

  if (snd_hctl_elem_read(elem, control) < 0)
    return false;

  int dataLength = snd_ctl_elem_info_get_count(einfo);
  /* if there is no ELD data, then its a bad HDMI device, either nothing attached OR an invalid nVidia HDMI device
   * OR the driver doesn't properly support ELD (notably ATI/AMD, 2012-05) */
  if (!dataLength)
    badHDMI = true;
  else
    CAEELDParser::Parse(
      (const uint8_t*)snd_ctl_elem_value_get_bytes(control),
      dataLength,
      info
    );

  info.m_deviceType = AE_DEVTYPE_HDMI;
  return true;
}

void CAESinkALSA::sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
  if(!g_advancedSettings.CanLogComponent(LOGAUDIO))
    return;

  va_list arg;
  va_start(arg, fmt);
  char *errorStr;
  if (vasprintf(&errorStr, fmt, arg) >= 0)
  {
    CLog::Log(LOGINFO, "CAESinkALSA - ALSA: %s:%d:(%s) %s%s%s",
              file, line, function, errorStr, err ? ": " : "", err ? snd_strerror(err) : "");
    free(errorStr);
  }
  va_end(arg);
}

#if HAVE_LIBUDEV
CALSADeviceMonitor CAESinkALSA::m_deviceMonitor; // ARGH
#endif
CALSAHControlMonitor CAESinkALSA::m_controlMonitor; // ARGH

#endif
