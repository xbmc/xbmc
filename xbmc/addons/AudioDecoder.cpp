/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioDecoder.h"

#include "addons/interfaces/AudioEngine.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/TagLoaderTagLib.h"
#include "utils/log.h"

namespace ADDON
{

CAudioDecoder::CAudioDecoder(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_AUDIODECODER, addonInfo)
{
  m_CodecName = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@name").asString();
  m_strExt = m_CodecName + "stream";
  m_hasTags = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@tags").asBoolean();

  // Create all interface parts independent to make API changes easier if
  // something is added
  m_struct.toKodi = new AddonToKodiFuncTable_AudioDecoder();
  m_struct.toAddon = new KodiToAddonFuncTable_AudioDecoder();
}

CAudioDecoder::~CAudioDecoder()
{
  DestroyInstance();

  delete m_struct.toKodi;
  delete m_struct.toAddon;
}

bool CAudioDecoder::CreateDecoder()
{
  m_struct.toKodi->kodiInstance = this;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK)
    return false;

  m_addonInstance = m_struct.toAddon->addonInstance;
  return true;
}

bool CAudioDecoder::Init(const CFileItem& file, unsigned int filecache)
{
  if (!m_struct.toAddon->init)
    return false;

  /// for replaygain
  /// @todo About audio decoder in most cases Kodi's one not work, add fallback
  /// to use addon if this fails. Need API change about addons music info tag!
  CTagLoaderTagLib tag;
  tag.Load(file.GetDynPath(), XFILE::CMusicFileDirectory::m_tag, NULL);

  int channels = -1;
  int sampleRate = -1;
  AudioEngineDataFormat addonFormat = AUDIOENGINE_FMT_INVALID;

  bool ret = m_struct.toAddon->init(m_addonInstance, file.GetDynPath().c_str(), filecache,
                                    &channels, &sampleRate, &m_bitsPerSample, &m_TotalTime,
                                    &m_bitRate, &addonFormat, &m_channel);
  if (ret)
  {
    if (channels <= 0 || sampleRate <= 0 || addonFormat == AUDIOENGINE_FMT_INVALID)
    {
      CLog::Log(LOGERROR,
                "CAudioDecoder::{} - Addon '{}' returned true without set of needed values",
                __func__, ID());
      return false;
    }

    m_format.m_dataFormat = Interface_AudioEngine::TranslateAEFormatToKodi(addonFormat);
    m_format.m_sampleRate = sampleRate;
    if (m_channel)
    {
      CAEChannelInfo layout;
      for (unsigned int ch = 0; ch < AUDIOENGINE_CH_MAX; ++ch)
      {
        if (m_channel[ch] == AUDIOENGINE_CH_NULL)
          break;
        layout += Interface_AudioEngine::TranslateAEChannelToKodi(m_channel[ch]);
      }

      m_format.m_channelLayout = layout;
    }
    else
      m_format.m_channelLayout = CAEUtil::GuessChLayout(channels);
  }

  return ret;
}

int CAudioDecoder::ReadPCM(uint8_t* buffer, int size, int* actualsize)
{
  if (!m_struct.toAddon->read_pcm)
    return 0;

  return m_struct.toAddon->read_pcm(m_addonInstance, buffer, size, actualsize);
}

bool CAudioDecoder::Seek(int64_t time)
{
  if (!m_struct.toAddon->seek)
    return false;

  m_struct.toAddon->seek(m_addonInstance, time);
  return true;
}

bool CAudioDecoder::Load(const std::string& fileName,
                         MUSIC_INFO::CMusicInfoTag& tag,
                         EmbeddedArt* art)
{
  if (!m_struct.toAddon->read_tag)
    return false;

  KODI_ADDON_AUDIODECODER_INFO_TAG ifcTag = {};
  bool ret = m_struct.toAddon->read_tag(m_addonInstance, fileName.c_str(), &ifcTag);
  if (ret)
  {
    if (ifcTag.title)
    {
      tag.SetTitle(ifcTag.title);
      free(ifcTag.title);
    }
    if (ifcTag.artist)
    {
      tag.SetArtist(ifcTag.artist);
      free(ifcTag.artist);
    }
    if (ifcTag.album)
    {
      tag.SetAlbum(ifcTag.album);
      free(ifcTag.album);
    }
    if (ifcTag.album_artist)
    {
      tag.SetAlbumArtist(ifcTag.album_artist);
      free(ifcTag.album_artist);
    }
    if (ifcTag.media_type)
    {
      tag.SetType(ifcTag.media_type);
      free(ifcTag.media_type);
    }
    if (ifcTag.genre)
    {
      tag.SetGenre(ifcTag.genre);
      free(ifcTag.genre);
    }
    tag.SetDuration(ifcTag.duration);
    tag.SetTrackNumber(ifcTag.track);
    tag.SetDiscNumber(ifcTag.disc);
    if (ifcTag.disc_subtitle)
    {
      tag.SetDiscSubtitle(ifcTag.disc_subtitle);
      free(ifcTag.disc_subtitle);
    }
    tag.SetTotalDiscs(ifcTag.disc_total);
    if (ifcTag.release_date)
    {
      tag.SetReleaseDate(ifcTag.release_date);
      free(ifcTag.release_date);
    }
    if (ifcTag.lyrics)
    {
      tag.SetLyrics(ifcTag.lyrics);
      free(ifcTag.lyrics);
    }
    tag.SetSampleRate(ifcTag.samplerate);
    tag.SetNoOfChannels(ifcTag.channels);
    tag.SetBitRate(ifcTag.bitrate);
    if (ifcTag.comment)
    {
      tag.SetComment(ifcTag.comment);
      free(ifcTag.comment);
    }
    tag.SetLoaded(true);
  }

  return ret;
}

int CAudioDecoder::GetTrackCount(const std::string& strPath)
{
  if (!m_struct.toAddon->track_count)
    return 0;

  int result = m_struct.toAddon->track_count(m_addonInstance, strPath.c_str());

  if (result > 1)
  {
    if (m_hasTags)
    {
      if (!Load(strPath, XFILE::CMusicFileDirectory::m_tag, nullptr))
        return 0;
    }
    else
      XFILE::CMusicFileDirectory::m_tag.SetTitle(CURL(strPath).GetFileNameWithoutPath());
    XFILE::CMusicFileDirectory::m_tag.SetLoaded(true);
  }

  return result;
}

} /* namespace ADDON */
