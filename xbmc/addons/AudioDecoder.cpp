/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioDecoder.h"

#include "cores/AudioEngine/Utils/AEUtil.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/TagLoaderTagLib.h"

namespace ADDON
{

CAudioDecoder::CAudioDecoder(const BinaryAddonBasePtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_AUDIODECODER, addonInfo)
{
  m_CodecName = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@name").asString();
  m_strExt = m_CodecName + "stream";
  m_hasTags = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@tags").asBoolean();
  m_struct = {{ 0 }};
}

CAudioDecoder::~CAudioDecoder()
{
  DestroyInstance();
}

bool CAudioDecoder::CreateDecoder()
{
  m_struct.toKodi.kodiInstance = this;
  return CreateInstance(&m_struct) == ADDON_STATUS_OK;
}

bool CAudioDecoder::Init(const CFileItem& file, unsigned int filecache)
{
  if (!m_struct.toAddon.init)
    return false;

  // for replaygain
  CTagLoaderTagLib tag;
  tag.Load(file.GetDynPath(), XFILE::CMusicFileDirectory::m_tag, NULL);

  int channels;
  int sampleRate;

 bool ret = m_struct.toAddon.init(&m_struct, file.GetDynPath().c_str(), filecache,
                                  &channels, &sampleRate,
                                  &m_bitsPerSample, &m_TotalTime,
                                  &m_bitRate, &m_format.m_dataFormat, &m_channel);

  m_format.m_sampleRate = sampleRate;
  if (m_channel)
    m_format.m_channelLayout = CAEChannelInfo(m_channel);
  else
    m_format.m_channelLayout = CAEUtil::GuessChLayout(channels);

  return ret;
}

int CAudioDecoder::ReadPCM(uint8_t* buffer, int size, int* actualsize)
{
  if (!m_struct.toAddon.read_pcm)
    return 0;

  return m_struct.toAddon.read_pcm(&m_struct, buffer, size, actualsize);
}

bool CAudioDecoder::Seek(int64_t time)
{
  if (!m_struct.toAddon.seek)
    return false;

  m_struct.toAddon.seek(&m_struct, time);
  return true;
}

bool CAudioDecoder::Load(const std::string& fileName,
                         MUSIC_INFO::CMusicInfoTag& tag,
                         EmbeddedArt* art)
{
  if (!m_struct.toAddon.read_tag)
    return false;

  char title[256] = { 0 };
  char artist[256] = { 0 };
  int length;
  if (m_struct.toAddon.read_tag(&m_struct, fileName.c_str(), title, artist, &length))
  {
    tag.SetTitle(title);
    tag.SetArtist(artist);
    tag.SetDuration(length);
    return true;
  }

  return false;
}

int CAudioDecoder::GetTrackCount(const std::string& strPath)
{
  if (!m_struct.toAddon.track_count)
    return 0;

  int result = m_struct.toAddon.track_count(&m_struct, strPath.c_str());

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


} /*namespace ADDON*/
