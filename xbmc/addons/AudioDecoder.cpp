/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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

#include "AudioDecoder.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/TagLoaderTagLib.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

namespace ADDON
{

CAudioDecoder::CAudioDecoder(AddonInfoPtr addonInfo)
  : IAddonInstanceHandler(ADDON_AUDIODECODER, addonInfo)
{
  m_extension = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@extension").asString();
  m_mimetype = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@mimetype").asString();
  m_tags = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@tags").asBoolean();
  m_tracks = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@tracks").asBoolean();
  m_CodecName = addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@name").asString();
  m_strExt = m_CodecName + "stream";
}

CAudioDecoder::~CAudioDecoder()
{
  DestroyInstance();
}

bool CAudioDecoder::Create()
{
  m_struct.toKodi.kodiInstance = this;
  if (!CreateInstance(ADDON_INSTANCE_AUDIODECODER, &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance)))
    return false;

  return true;
}

bool CAudioDecoder::Init(const CFileItem& file, unsigned int filecache)
{
  if (!m_struct.toAddon.Init)
    return false;

  // for replaygain
  CTagLoaderTagLib tag;
  tag.Load(file.GetPath(), XFILE::CMusicFileDirectory::m_tag, nullptr);

  int channels;
  int sampleRate;

  bool ret = m_struct.toAddon.Init(m_addonInstance,
                                   file.GetPath().c_str(), filecache,
                                   &channels, &sampleRate,
                                   &m_bitsPerSample, &m_TotalTime,
                                   &m_bitRate, &m_format.m_dataFormat, &m_channel);

  if (ret)
  {
    m_format.m_sampleRate = sampleRate;
    if (m_channel)
      m_format.m_channelLayout = CAEChannelInfo(m_channel);
    else
      m_format.m_channelLayout = CAEUtil::GuessChLayout(channels);
  }

  return ret;
}

int CAudioDecoder::ReadPCM(uint8_t* buffer, int size, int* actualsize)
{
  if (!m_struct.toAddon.ReadPCM)
    return 0;

  return m_struct.toAddon.ReadPCM(m_addonInstance, buffer, size, actualsize);
}

bool CAudioDecoder::Seek(int64_t time)
{
  if (!m_struct.toAddon.Seek)
    return false;

  m_struct.toAddon.Seek(m_addonInstance, time);
  return true;
}

bool CAudioDecoder::Load(const std::string& fileName,
                         MUSIC_INFO::CMusicInfoTag& tag,
                         MUSIC_INFO::EmbeddedArt* art)
{
  char title[256];
  char artist[256];
  int length;

  if (!m_struct.toAddon.ReadTag ||
      !m_struct.toAddon.ReadTag(m_addonInstance, fileName.c_str(), title, artist, &length))
    return false;

  tag.SetTitle(title);
  tag.SetArtist(artist);
  tag.SetDuration(length);
  return true;
}

int CAudioDecoder::GetTrackCount(const std::string& strPath)
{
  if (!m_struct.toAddon.TrackCount)
    return 0;

  int result = m_struct.toAddon.TrackCount(m_addonInstance, strPath.c_str());

  if (result > 1 && !Load(strPath, XFILE::CMusicFileDirectory::m_tag, nullptr))
    return 0;

  XFILE::CMusicFileDirectory::m_tag.SetLoaded(true);
  return result;
}

} /* namespace ADDON */
